#include "kheap.h"
#include <kassert.h>
#include "paging.h"
#include <stdlib.h>

extern uint8_t end;
uintptr_t placement_address = (uintptr_t)&end;

void* kmalloc(size_t size) {
  if (os::Paging::heapActive()) {
    return malloc(size); 
  } else {
    uintptr_t tmp = placement_address;
    placement_address += ((size / 4) + 1) * 4; // Ensure that all addresses are 4-byte aligned
    return (void*)tmp;
  }
}

void* kmalloc_align(size_t size, void** physical) {
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

void kfree(void* p) {
  assert(os::Paging::heapActive());
  free(p);
}

void *krealloc(void* p, size_t s) {
  assert(os::Paging::heapActive());
  return realloc(p, s);
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
 
void operator delete(void *p, size_t)
{
  kfree(p);
}
 
void operator delete[](void *p, size_t)
{
  kfree(p);
}