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

static PageDirectory* directory;
static os::bitset<>* memoryMap;
static uintptr_t heapStart;
static size_t heapSize = 0;

static size_t firstPde = 0;

static int volatile liballoc_spinlock = 0;

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

void os::Paging::identity_map(uintptr_t start, uintptr_t end, bool rw, bool user) {
  assert(start < end);

  auto& dir = *directory;

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
  directory = (PageDirectory*)kmalloc_align(sizeof(*directory));
  directory->fill({0});

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
  identity_map(Reflection::getKernelStart(), Reflection::getKernelEnd(), false);

  auto& dir = *directory;

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
  firstPde = 1024 - heapSize / (0x1000 * 1024);

  os::Interrupts::registerInterruptHandler(14, pageFaultHandler);

  loadPageDirectory(directory);
  enablePaging();
}

os::std::pair<uintptr_t, bool> os::Paging::translate(uintptr_t virtAddr) {
  assert(directory != nullptr);
  uint32_t pde = (virtAddr & 0xFFC00000) >> 22;
  uint32_t pte = (virtAddr & 0x003FF000) >> 12;
  uint32_t offs = virtAddr & 0x00000FFF;

  auto& dir = *directory;
  if(dir[pde].present) {
    auto& table = *((PageTable*)(dir[pde].addr * 0x1000));
    if(table[pte].present) {
      return {(table[pte].addr * 0x1000) | offs, true};
    }
  }
  return {0, false};
}

int liballoc_lock() {
  spinlock_acquire(&liballoc_spinlock);
  return 0;
}

int liballoc_unlock() {
  spinlock_release(&liballoc_spinlock);
  return 0;
}

void* liballoc_alloc(int n) {
  assert(directory != nullptr);
  uintptr_t res = 0;
  auto& dir = *directory;
  for(int i = 0; i < n; i++) {
    size_t freeFrame = memoryMap->firstFree();
    size_t pde = freeFrame / 1024 + firstPde;
    size_t pte = freeFrame % 1024;

    if(res == 0) {
      res = (pde << 22) | (pte << 12);
    }
    
    PageTable* table = nullptr;

    if(!dir[pde].present) {
      dir[pde].present = 1;
      dir[pde].rw = 1;
      
      table = (PageTable*)kmalloc_align(sizeof(PageTable));
      memset(table, 0, sizeof(PageTable));
      dir[pde].addr = (uintptr_t)table / 0x1000;
    } else {
      table = (PageTable*)(dir[pde].addr * 0x1000);
    }

    assert(table != nullptr);

    auto& table_ref = *table;

    assert(!((heapStart + freeFrame * 0x1000) & 0x00000FFF));

    if(!table_ref[pte].present) {
      table_ref[pte].present = 1;
      table_ref[pte].rw = 1;
      table_ref[pte].addr = (heapStart + freeFrame * 0x1000) / 0x1000;
    }

    memoryMap->set(freeFrame);
  }
  loadPageDirectory(directory);
  return (void*)res;
}

int liballoc_free(void* addr, int n) {
  assert(directory != nullptr);
  auto& dir = *directory;
  size_t frame = (uintptr_t)addr / 0x1000;

  for(int i = 0; i < n; i++) {
    size_t pde = frame / 1024 - firstPde;
    size_t pte = frame % 1024;

    assert(dir[pde].present);

    auto& table = *((PageTable*)(dir[pde].addr * 0x1000));

    assert(table[pte].present);

    table[pte].present = 0;
    memoryMap->unset(frame);

    frame++;
  }
}