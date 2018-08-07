#include "multiboot.h"
#include "screen.h"
#include "descriptor_tables.h"
#include "paging.h"
#include "timer.h"
#include "reflection.h"
#include <stddef.h>
#include <kassert.h>
#include <stdlib.h>
#include "tasking.h"
#include "debug.h"

using os::Screen;

static void func(void);
static void func2(void);
static void func3(void);
static void func4(void);

extern "C" int kmain(multiboot_info_t*);

int kmain(multiboot_info_t *mboot_ptr) {
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

  screen.write("Physical memory size: %KB \nFree memory: %KB\n",
    os::Paging::getHeapSize() >> 10,
    os::Paging::getFreeHeap() >> 10);

  os::Tasking::init();
  os::Tasking::Thread* t1 = os::Tasking::Thread::start(&func);

  os::Tasking::Thread* t2 = os::Tasking::Thread::start(&func3);
  os::Tasking::Thread* t3 = os::Tasking::Thread::start(&func4);

  os::std::array<os::Tasking::Waitable*, 2> ts;
  ts[0] = t2;
  ts[1] = t3;

  auto t4 = os::Tasking::Waitable::wait_one(ts.data(), 2);
  t4->wait();

  while(true) {
    os::Screen::getInstance().write("foobar!\n");
    for(size_t i = 0; i < 0xFFFFFF1; i++) {

    }
    t1->wait();
  }
  return 0;
}

void func() {
  os::Tasking::Thread* t = os::Tasking::Thread::start(&func2);
  os::Screen::getInstance().write("foo!\n");
  t->wait();
  for(size_t i = 0; i < 0xFFFFFFF; i++) { }
}


void func2() {
  os::Screen::getInstance().write("bar!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  os::Screen::getInstance().write("baz\n");
}

void func3() {
  os::Screen::getInstance().write("barbaz fast!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
}

void func4() {
  os::Screen::getInstance().write("barbaz slow!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
}