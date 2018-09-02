#include "paging.h"

#include <stdint.h>
#include "kheap.h"
#include <kassert.h>
#include <string.h>
#include "interrupts.h"
#include "kheap.h"
#include <bitset.h>
#include "reflection.h"
#include "screen.h"
#include "synchro.h"

using namespace os::Paging;

struct os::Paging::PageDirectory {
  std::array<PageDirectoryEntry, 1024> entries;

  PageDirectoryEntry& operator[](size_t idx) {
    return entries[idx];
  }

  const PageDirectoryEntry& operator[](size_t idx) const {
    return entries[idx];
  }

  PageTable& getTable(size_t idx) {
    assert(idx < 1024);
    assert(entries[idx].present);

    uintptr_t addr = entries[idx].addr << 12;
    return *((PageTable*)addr);
  }
};

static PageDirectory* kernel_directory;
static PageDirectory* current_directory;

// Phyisical memory bitmap
static os::bitset<>* memoryMap;

static uintptr_t heapStart;
static size_t heapSize = 0;

static os::Spinlock spinlock;

bool os::Paging::heapActive() {
  return memoryMap != nullptr && current_directory != nullptr;
}

static void pageFaultHandler(os::Interrupts::Registers* regs) {
  uintptr_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

  // The error code gives us details of what happened.
  int present = regs->err_code & 0x1;  // Page not present
  int rw = regs->err_code & 0x2;       // Write operation?
  int us = regs->err_code & 0x4;       // Processor was in user-mode?
  int reserved = regs->err_code & 0x8; // Overwritten CPU-reserved bits of page entry?

  //uint32_t pde = (faulting_address & 0xFFC00000) >> 22;
  //uint32_t pte = (faulting_address & 0x003FF000) >> 12;
  
  os::Screen::getInstance().write("Page fault (% % % %) at % from %\n",
    present ? "present" : "",
    rw ? "readonly" : "",
    us ? "usermode" : "",
    reserved ? "reserved" : "",
    (void*)faulting_address, (void*)regs->eip);
  while(true) {}
}

static void loadPageDirectory(PageDirectory *dir) {
  assert(!((uintptr_t)dir & 0x00000FFF));
  asm volatile("mov %0, %%cr3":: "r"(dir));
}

static void enablePaging() {
  uint32_t cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000; // Enable paging!
  asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void os::Paging::switchDirectory(PageDirectory* dir) {
  loadPageDirectory(dir);
  current_directory = dir;
}

void os::Paging::switchDirectory() {
  loadPageDirectory(kernel_directory);
  current_directory = kernel_directory;
}

// This function should not be used once the heap is working
static void identity_map(uintptr_t start, uintptr_t end, bool rw = true, bool user = false) {
  assert(start < end);

  auto& dir = *kernel_directory;

  for(uintptr_t i = start; i < end; i += 0x1000) {
    uint32_t pde = (i & 0xFFC00000) >> 22;
    uint32_t pte = (i & 0x003FF000) >> 12;

    PageTable* table = nullptr;

    if(!dir[pde].present) {
      dir[pde].present = 1;
      dir[pde].rw = rw ? 1 : 0;
      dir[pde].user = user ? 1 : 0;

      table = (PageTable*)kmalloc_align(sizeof(PageTable));
      memset(table, 0, sizeof(PageTable));
      dir[pde].addr = (uintptr_t)table / 0x1000;
    } else {
      table = (PageTable*)(dir[pde].addr * 0x1000);
    }

    assert(table != nullptr);

    auto& table_ref = *table;

    if(!table_ref[pte].present) {
      table_ref[pte].present = 1;
      table_ref[pte].rw = 1;
      table_ref[pte].addr = i / 0x1000;
    }
  }
}

void os::Paging::init(multiboot_memory_map_t* map, size_t mapLength) {
  kernel_directory = (PageDirectory*)kmalloc_align(sizeof(*kernel_directory));
  memset(kernel_directory, 0, sizeof(PageDirectory));
  current_directory = kernel_directory;

  // Identity-map all the memory from GRUB, also search for the largest chunk
  multiboot_memory_map_t* mmap = map;
  while((uintptr_t)mmap < (uintptr_t)map + mapLength) {
    if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
      identity_map(mmap->addr, mmap->addr + mmap->len);
      if(mmap->len > heapSize) {
        heapSize = mmap->len;
        heapStart = mmap->addr;
        if(heapStart & 0x00000FFF) {
          size_t tmp = heapStart;
          heapStart &= 0x00000FFF;
          heapStart += 0x1000;
          heapSize -= (heapStart - tmp);
        }
      }
    }
    mmap = (multiboot_memory_map_t*) ( (uintptr_t)mmap + mmap->size + sizeof(mmap->size) );
  }
  // Make sure the kernel's code is read-only
  identity_map(Reflection::getKernelStart(), Reflection::getKernelEnd(), false);

  // If the largest chunks contains part of the kernel, skip that part
  if(heapStart < Reflection::getKernelEnd()) {
    uintptr_t nextPage = Reflection::getKernelEnd();
    if(nextPage & 0x00000FFF) {
      nextPage &= 0xFFFFF000;
      nextPage += 0x1000;
    }

    size_t tmp = heapStart;
    heapStart = nextPage;
    heapSize -= (heapStart - tmp);
  }

  memoryMap = new os::bitset<>((heapSize - 1) / 0x1000 + 1);

  os::Interrupts::registerInterruptHandler(14, pageFaultHandler);

  loadPageDirectory(kernel_directory);
  enablePaging();
}

os::std::pair<uintptr_t, bool> os::Paging::translate(uintptr_t virtAddr) {
  assert(current_directory != nullptr);
  uint32_t pde_idx = (virtAddr & 0xFFC00000) >> 22;
  uint32_t pte_idx = (virtAddr & 0x003FF000) >> 12;
  uint32_t offs = virtAddr & 0x00000FFF;

  auto& dir = *current_directory;
  auto& pde = dir[pde_idx];
  if(pde.present) {
    auto& table = *((PageTable*)(pde.addr * 0x1000));
    auto& pte = table[pte_idx];
    if(pte.present) {
      return {(pte.addr * 0x1000) | offs, true};
    }
  }
  return {0, false};
}

static uintptr_t allocPage() {
  size_t frame = memoryMap->firstFree();
  if(frame == (size_t)-1) return 0;
  memoryMap->set(frame);

  return (uintptr_t)(heapStart + frame * 0x1000);
}

static void freePage(uintptr_t page) {
  size_t frame = page >> 12;
  memoryMap->unset(frame);
}

static PageDirectory* allocDirectory() {
  PageDirectory* dir = (PageDirectory*)allocPage();
  memset(dir, 0, sizeof(PageDirectory));
  return dir;
}

static void freeDirectory(PageDirectory* dir) {
  freePage((uintptr_t)dir);
}

static PageTable* allocTable() {
  PageTable* table = (PageTable*)allocPage();
  memset(table, 0, sizeof(PageTable));
  return table;
}

static void freeTable(PageTable* table) {
  freePage((uintptr_t)table);
}

struct HeapHeader {
  uint32_t magic;
  size_t chunkSize;

  static constexpr uint32_t magic_value = 0xDEADBEEF;
};

void *malloc(size_t s) {
  spinlock.acquire();
  size_t neededMemory = s + sizeof(HeapHeader);
  size_t numPages = (neededMemory - 1) / 0x1000 + 1;
  size_t addrSlot = memoryMap->freeSpan(numPages);
  if(addrSlot == (size_t)-1) return nullptr;
  for(size_t i = 0; i < numPages; i++) {
    memoryMap->set(addrSlot + i);
  }
  spinlock.release();

  uintptr_t addrStart = heapStart + (addrSlot << 12);

  void* actualStart = (void*)(addrStart + sizeof(HeapHeader));

  HeapHeader* header = (HeapHeader*)addrStart;
  header->chunkSize = s;
  header->magic = HeapHeader::magic_value;

  return actualStart;
}

void *calloc(size_t n, size_t s) {
  void* addr = malloc(n * s);
  memset(addr, 0, n * s);
  return addr;
}

void *realloc(void *ptr, size_t s) {
  if(s == 0) {
    free(ptr);
    return nullptr;
  }

  if(ptr == nullptr) {
    return malloc(s);
  }

  uintptr_t addr = (uintptr_t)ptr;
  size_t pageNum = (addr - heapStart) >> 12;

  uintptr_t pageAddr = heapStart + (pageNum << 12);
  HeapHeader* hdr = (HeapHeader*)pageAddr;
  assert(hdr->magic == HeapHeader::magic_value);

  size_t curNumPages = ((hdr->chunkSize + sizeof(HeapHeader)) - 1) / 0x1000 + 1;
  size_t targetNumPages = ((s + sizeof(HeapHeader)) - 1) / 0x1000 + 1;

  if(curNumPages > targetNumPages) {
    os::scoped_lock l(spinlock);
    size_t to_dealloc = curNumPages - targetNumPages;
    size_t lastPage = pageAddr + curNumPages - 1;
    for(size_t i = 0; i < to_dealloc; i++) {
      memoryMap->unset(lastPage - i);
    }
    return ptr;
  } else if(curNumPages < targetNumPages) {
    os::scoped_lock l(spinlock);
    size_t to_alloc = targetNumPages - curNumPages;
    if(memoryMap->free_slots_after(pageAddr) >= to_alloc) {
      for(size_t i = 0; i < to_alloc; i++) {
        memoryMap->set(pageAddr + i + 1);
      }
      hdr->chunkSize = s;
      return ptr;
    } else {
      // Need to move block
      panic("UNIMPLEMENTED");
      return nullptr;
    }
  } else {
    hdr->chunkSize = s;
    return ptr;
  }
}

void free(void* ptr) {
  uintptr_t addr = (uintptr_t)ptr;
  size_t pageNum = (addr - heapStart) >> 12;

  uintptr_t pageAddr = heapStart + (pageNum << 12);
  HeapHeader* hdr = (HeapHeader*)pageAddr;
  assert(hdr->magic == HeapHeader::magic_value);
  size_t numPagesToFree = ((hdr->chunkSize + sizeof(HeapHeader)) - 1) / 0x1000 + 1;

  spinlock.acquire();
  for(size_t i = 0; i < numPagesToFree; i++) {
    memoryMap->unset(pageNum + i);
  }
  spinlock.release();
}

size_t os::Paging::getHeapSize() {
  return heapSize;
}

size_t os::Paging::getFreeHeap() {
  spinlock.acquire();
  size_t v = memoryMap->free_slots() << 12;
  spinlock.release();
  return v;
}

static PageTable* cloneTable(PageTable* orig) {
  PageTable* res = allocTable();
  memcpy(res, orig, sizeof(PageTable));
  return res;
}

static PageDirectory* cloneDirectory(PageDirectory& dir) {
  PageDirectory* clone_ptr = allocDirectory();

  auto& clone = *clone_ptr;
  for(int i = 0; i < 1024; i++) {
    if(dir[i].present) {
      clone[i] = dir[i];
      PageTable* tbl = (PageTable*)(dir[i].addr << 12);
      clone[i].addr = (uintptr_t)cloneTable(tbl) >> 12;
    }
  }

  return clone_ptr;
}

ThreadData os::Paging::makeThread() {
  spinlock.acquire();
  uintptr_t stackPage = allocPage();
  memset((void*)stackPage, 0, 0x1000);

  PageTable* stackTable = allocTable();
  PageDirectory* dir = cloneDirectory(*kernel_directory);
  spinlock.release();

  auto& lastTableEntry = (*stackTable)[1023];
  lastTableEntry.addr = stackPage / 0x1000;
  lastTableEntry.present = 1;
  lastTableEntry.rw = 1;

  auto& lastDirEntry = (*dir)[1023];
  lastDirEntry.addr = (uintptr_t)stackTable / 0x1000;
  lastDirEntry.present = 1;
  lastDirEntry.rw = 1;

  return {dir, 0xFFFFFFFF, stackPage + 0xFFF};
}

void os::Paging::freeThread(const ThreadData& data) {
  os::scoped_lock l(spinlock);

  auto& lastDirEntry = (*data.directory)[1023];
  PageTable* stackTable = (PageTable*)(lastDirEntry.addr << 12);
  freeTable(stackTable);
  freeDirectory(data.directory);
}

PageDirectory* os::Paging::currentDirectory() {
  return current_directory;
}