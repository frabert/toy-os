#include "multiboot.h"
#include "screen.h"
#include "descriptor_tables.h"
#include "paging.h"
#include "timer.h"
#include "reflection.h"
#include <stddef.h>
#include <kassert.h>
#include <stdlib.h>

using os::Screen;

extern "C" int kmain(multiboot_info_t *mboot_ptr) {
  assert(mboot_ptr->flags & (1 << 5));
  os::Reflection::init(mboot_ptr->u.elf_sec);

  Screen::init();
  Screen& screen = Screen::getInstance();
  screen.setColors(Screen::Color::Gray, Screen::Color::Blue);
  screen.clear();

  screen.write("Toy OS booting\n");

  os::DescriptorTables::init();
  screen.write("Descriptor tables initialized\n");
  asm volatile("sti");
  os::Timer::init(50);

  multiboot_memory_map_t* mmap;
  if(mboot_ptr->flags & (1 << 6)) {
    mmap = (multiboot_memory_map_t*)mboot_ptr->mmap_addr;
  } else {
    panic("No memory map found");
  }

  os::Paging::init(mmap, mboot_ptr->mmap_length);
  screen.write("Paging initialized\n");

  int* a = (int*)malloc(sizeof(int) * 64);
  int* b = (int*)os::Paging::translate(a).first;
  screen.write("a: %\n", a);
  for(int i = 0; i < 64; i++) {
    a[i] = i;
  }
 
  for(int i = 0; i < 64; i++) {
    screen.write("a[%] = %\n", i, a[i]);
    screen.write("b[%] = %\n", i, b[i]);
  }

  int* c = (int*)malloc(sizeof(int));
  int* d = (int*)malloc(sizeof(int));
  free(c);
  int* e = (int*)malloc(sizeof(int));
  screen.write("c: %\nd: %\ne: %\n", c, d, e);

  while(true) {;}
  return 0;
}