#include "paging.h"

#include <stdint.h>
#include "kmalloc.h"
#include "kassert.h"
#include "utils.h"
#include "interrupts.h"

using namespace os::Paging;

extern uintptr_t placement_address;

// A bitset of frames - used or free.
uint32_t *frames;
uint32_t nframes;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr/0x1000;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr/0x1000;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static uint32_t test_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr/0x1000;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static uint32_t first_frame() {
  uint32_t i, j;
  for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
    if (frames[i] != 0xFFFFFFFF) {
      // at least one bit is free here.
      for (j = 0; j < 32; j++) {
        uint32_t toTest = 0x1 << j;
        if ( !(frames[i]&toTest) ) {
          return i*4*8+j;
        }
      }
    }
  }
}

// The kernel's page directory
static PageDirectory *kernel_directory = nullptr;

// The current page directory;
static PageDirectory *current_directory = nullptr;

static void alloc_frame(Page* page, bool is_kernel, bool is_writeable) {
  if (page->frame != 0) {
    return; // Frame was already allocated, return straight away.
  } else {
    uint32_t idx = first_frame(); // idx is now the index of the first free frame.
    KASSERT_MSG(idx != (uint32_t)-1, "No free frames");
    set_frame(idx * 0x1000); // this frame is now ours!
    page->present = 1; // Mark it as present.
    page->rw = (is_writeable)?1:0; // Should the page be writeable?
    page->user = (is_kernel)?0:1; // Should the page be user-mode?
    page->frame = idx;
  }
}

static void free_frame(Page *page)
{
  uint32_t frame;
  if (!(frame=page->frame)) {
    return; // The given page didn't actually have an allocated frame!
  } else {
    clear_frame(frame); // Frame is now free again.
    page->frame = 0x0; // Page now doesn't have a frame.
  }
}

static void page_fault(os::Interrupts::Registers regs)
{
   // A page fault has occurred.
   // The faulting address is stored in the CR2 register.
   uint32_t faulting_address;
   asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

   // The error code gives us details of what happened.
   int present   = !(regs.err_code & 0x1); // Page not present
   int rw = regs.err_code & 0x2;           // Write operation?
   int us = regs.err_code & 0x4;           // Processor was in user-mode?
   int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
   int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

   // Output an error message.

  KPANIC("Page fault (present % read-only % user-mode % reserved %) at %\n", present, rw, us, reserved, (void*)faulting_address);
} 

void os::Paging::init()
{
   // The size of physical memory. For the moment we
   // assume it is 16MB big.
   uint32_t mem_end_page = 0x1000000;

   nframes = mem_end_page / 0x1000;
   frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes));
   memset(frames, 0, INDEX_FROM_BIT(nframes));

   // Let's make a page directory.
   kernel_directory = (PageDirectory*)kmalloc_align(sizeof(PageDirectory));
   memset(kernel_directory, 0, sizeof(PageDirectory));
   current_directory = kernel_directory;

   // We need to identity map (phys addr = virt addr) from
   // 0x0 to the end of used memory, so we can access this
   // transparently, as if paging wasn't enabled.
   // NOTE that we use a while loop here deliberately.
   // inside the loop body we actually change placement_address
   // by calling kmalloc(). A while loop causes this to be
   // computed on-the-fly rather than once at the start.
   uintptr_t i = 0;
   while (i < placement_address)
   {
       // Kernel code is readable but not writeable from userspace.
       alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
       i += 0x1000;
   }
   // Before we enable paging, we must register our page fault handler.
   os::Interrupts::register_interrupt_handler(14, page_fault);

   // Now, enable paging!
   switch_page_directory(kernel_directory);
}

void os::Paging::switch_page_directory(PageDirectory *dir)
{
   current_directory = dir;
   uintptr_t addr = (uintptr_t)&dir->tablesPhysical;
   asm volatile("mov %0, %%cr3":: "r"(addr));
   uint32_t cr0;
   asm volatile("mov %%cr0, %0": "=r"(cr0));
   cr0 |= 0x80000000; // Enable paging!
   asm volatile("mov %0, %%cr0":: "r"(cr0));
}

Page *os::Paging::get_page(uintptr_t addr, bool make, PageDirectory *dir)
{
  size_t address = (size_t)addr;
  // Turn the address into an index.
  address /= 0x1000;
  // Find the page table containing this address.
  size_t table_idx = address / 1024;
  if (dir->tables[table_idx] != nullptr) {
      return &dir->tables[table_idx]->pages[address%1024];
  } else if(make) {
    void* tmp;
    dir->tables[table_idx] = (PageTable*)kmalloc_align(sizeof(PageTable), &tmp);
    memset(dir->tables[table_idx], 0, 0x1000);
    dir->tablesPhysical[table_idx] = (uintptr_t)tmp | 0x7; // PRESENT, RW, US.
    return &dir->tables[table_idx]->pages[address % 1024];
  } else {
    return 0;
  }
}