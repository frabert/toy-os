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
#include "liballoc.h"
#include "synchro.h"

using namespace os::Paging;

static PageDirectory* kernel_directory;
static PageDirectory* current_directory;

// Phyisical memory bitmap
static os::bitset<>* memoryMap;

// Virtual memory bitmap
static os::bitset<>* vmemoryMap;
constexpr static uintptr_t vmemStart = 0xFF000000;

static uintptr_t heapStart;
static size_t heapSize = 0;

static os::Spinlock spinlock;

bool os::Paging::heapActive() {
  return memoryMap != nullptr && vmemoryMap != nullptr;
}

static void pageFaultHandler(os::Interrupts::Registers* regs) {
  uintptr_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

  // The error code gives us details of what happened.
  int present = regs->err_code & 0x1;  // Page not present
  int rw = regs->err_code & 0x2;       // Write operation?
  int us = regs->err_code & 0x4;       // Processor was in user-mode?
  int reserved = regs->err_code & 0x8; // Overwritten CPU-reserved bits of page entry?

  uint32_t pde = (faulting_address & 0xFFC00000) >> 22;
  uint32_t pte = (faulting_address & 0x003FF000) >> 12;
  
  os::Screen::getInstance().write("Page fault (% % % %) at %\n",
    present ? "present" : "",
    rw ? "readonly" : "",
    us ? "usermode" : "",
    reserved ? "reserved" : "",
    (void*)faulting_address);
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
}

// This function should not be used once the heap is working
static void identity_map(uintptr_t start, uintptr_t end, bool rw = true, bool user = false) {
  assert(start < end);

  auto& dir = *kernel_directory;

  for(uintptr_t i = start; i < end; i += 0x1000) {
    uint32_t pde = (i & 0xFFC00000) >> 22;
    uint32_t pte = (i & 0x003FF000) >> 12;
    uint32_t frame = i & 0x00000FFF;

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
  kernel_directory->fill({0});
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

  auto& dir = *kernel_directory;

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

  memoryMap = new os::bitset<>(heapSize / 0x1000);
  vmemoryMap = new os::bitset<>(heapSize / 0x1000);

  os::Interrupts::registerInterruptHandler(14, pageFaultHandler);

  loadPageDirectory(kernel_directory);
  enablePaging();
}

os::std::pair<uintptr_t, bool> os::Paging::translate(uintptr_t virtAddr) {
  assert(kernel_directory != nullptr);
  uint32_t pde = (virtAddr & 0xFFC00000) >> 22;
  uint32_t pte = (virtAddr & 0x003FF000) >> 12;
  uint32_t offs = virtAddr & 0x00000FFF;

  auto& dir = *kernel_directory;
  if(dir[pde].present) {
    auto& table = *((PageTable*)(dir[pde].addr * 0x1000));
    if(table[pte].present) {
      return {(table[pte].addr * 0x1000) | offs, true};
    }
  }
  return {0, false};
}

int liballoc_lock() {
  spinlock.acquire();
  return 0;
}

int liballoc_unlock() {
  spinlock.release();
  return 0;
}

static uintptr_t allocPage() {
  size_t frame = memoryMap->firstFree();
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

void* liballoc_alloc(int n) {
  assert(kernel_directory != nullptr);
  size_t vframeStart = vmemoryMap->freeSpan(n);

  auto& dir = *kernel_directory;

  for(int i = 0; i < n; i++) {
    vmemoryMap->set(vframeStart + i);
    uintptr_t virtAddr = vmemStart + (vframeStart + i) * 0x1000;
    size_t pde_idx = (virtAddr & 0xFFC00000) >> 22;
    size_t pte_idx = (virtAddr & 0x003FF000) >> 12;

    auto& pde = dir[pde_idx];
    PageTable* table_ptr = nullptr;
    if(!pde.present) {
      pde.present = 1;
      pde.rw = 1;
      table_ptr = allocTable();
      pde.addr = (uintptr_t)table_ptr >> 12;
    } else {
      table_ptr = (PageTable*)(pde.addr << 12);
    }

    auto& table = *table_ptr;
    if(!table[pte_idx].present) {
      auto& pte = table[pte_idx];
      pte.present = 1;
      pte.rw = 1;
      uintptr_t page = allocPage();
      pte.addr = page >> 12;
    }
  }

  loadPageDirectory(kernel_directory);
  uintptr_t res = vmemStart + vframeStart * 0x1000;
  return (void*)res;
}

int liballoc_free(void* addr, int n) {
  assert(kernel_directory != nullptr);
  uintptr_t vframeStart = ((uintptr_t)addr - vmemStart) / 0x1000;

  auto& dir = *kernel_directory;

  for(int i = 0; i < n; i++) {
    vmemoryMap->unset(vframeStart + i);
    uintptr_t virtAddr = (uintptr_t)addr + i * 0x1000;
    size_t pde_idx = (virtAddr & 0xFFC00000) >> 22;
    size_t pte_idx = (virtAddr & 0x003FF000) >> 12;

    auto& pde = dir[pde_idx];
    PageTable* table_ptr = nullptr;
    if(pde.present) {
      table_ptr = (PageTable*)(pde.addr << 12);

      auto& table = *table_ptr;
      if(table[pte_idx].present) {
        auto& pte = table[pte_idx];
        pte.present = 0;
        pte.rw = 0;
        freePage(pte.addr << 12);
      }
    }
  }

  loadPageDirectory(current_directory);
}

size_t os::Paging::getHeapSize() {
  return heapSize;
}

size_t os::Paging::getFreeHeap() {
  return memoryMap->free_slots() * 0x1000;
}

os::std::pair<PageDirectory*, uintptr_t> os::Paging::makeThread() {
  spinlock.acquire();
  uintptr_t stackPage = allocPage();

  PageTable* stackTable = allocTable();
  PageDirectory* dir = allocDirectory();
  spinlock.release();

  auto& lastTableEntry = (*stackTable)[1023];
  lastTableEntry.addr = stackPage / 0x1000;
  lastTableEntry.present = 1;
  lastTableEntry.rw = 1;

  auto& lastDirEntry = (*dir)[1023];
  lastDirEntry.addr = (uintptr_t)stackTable / 0x1000;
  lastDirEntry.present = 1;
  lastDirEntry.rw = 1;
  
  for(size_t i = 0; i < 1024; i++) {
    auto pde = (*kernel_directory)[i];
    if(pde.present) {
      (*dir)[i] = (*kernel_directory)[i];
    }
  }

  return {dir, 0xFFFFF00};
}