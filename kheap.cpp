#include "kheap.h"
#include <kassert.h>
#include "paging.h"

extern uint8_t end;
uintptr_t placement_address = (uintptr_t)&end;

void* kmalloc(size_t size) {
  /*if (kernel_heap != nullptr) {
    return kernel_heap->alloc(size, false); 
  } else */{
    uintptr_t tmp = placement_address;
    placement_address += ((size / 4) + 1) * 4; // Ensure that all addresses are 4-byte aligned
    return (void*)tmp;
  }
}

void* kmalloc_align(size_t size, void** physical) {
  /*if (kernel_heap != nullptr) {
    void *addr = kernel_heap->alloc(size, true);
    auto page = os::Paging::get_page((uintptr_t)addr, 0, kernel_directory);
    *physical = (void*)(page->frame*0x1000 + ((uintptr_t)addr&0xFFF));
    return addr;
  } else */{
    if (placement_address & 0xFFFFF000) {
      // Align the placement address;
      placement_address &= 0xFFFFF000;
      placement_address += 0x1000;
    }

    *physical = (void*)placement_address;
    uintptr_t tmp = placement_address;
    placement_address += ((size / 4) + 1) * 4; // Ensure that all addresses are 4-byte aligned
    return (void*)tmp;
  }
}

void kfree(void* p) {
  /*KASSERT(kernel_heap != nullptr);
  kernel_heap->free(p);*/
  panic("Cannot free");
}

void *krealloc(void* p, size_t s) {
  /*KASSERT(kernel_heap != nullptr);
  return kernel_heap->realloc(p, s);*/
  panic("Cannot realloc");
}

void *operator new(size_t size)
{
  return kmalloc(size);
}
 
void *operator new[](size_t size)
{
  return kmalloc(size);
}
 
void operator delete(void *p)
{
  kfree(p);
}
 
void operator delete[](void *p)
{
  kfree(p);
}
 
void operator delete(void *p, size_t s)
{
  kfree(p);
}
 
void operator delete[](void *p, size_t s)
{
  kfree(p);
}