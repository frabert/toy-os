#include "kmalloc.h"

#include <stdint.h>

extern uint8_t end;
uintptr_t placement_address = (uintptr_t)&end;

void* kmalloc(size_t size) {
  void* addr = (void*)placement_address;
  placement_address += size;
  return addr;
}

void* kmalloc_align(size_t size, void** physical) {
  if ((size_t)placement_address & 0xFFFFF000) {
    placement_address = placement_address & 0xFFFFF000;
    placement_address += 0x1000;
  }

  *physical = (void*)placement_address;
  void* addr = (void*)placement_address;
  placement_address += size;
  return addr;
}