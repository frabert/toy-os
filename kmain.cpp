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

#include <priority_queue.h>

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
  os::Timer::init(100);

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
  auto t1 = os::Tasking::Task::start(&func);

  auto t2 = os::Tasking::Task::start(&func3);
  auto t3 = os::Tasking::Task::start(&func4);

  t2->wait();
  screen.write("t2 end\n");
  t3->wait();
  screen.write("t3 end\n");

  os::std::priority_queue<int> pq;
  pq.push(4);
  pq.push(10);
  pq.push(3);
  pq.push(5);
  pq.push(1);

  while(!pq.empty()) {
    screen.write("% ", pq.top());
    pq.pop();
  }
  screen.write("\n");

  while(true) {
    os::Screen::getInstance().write("foobar!\n");
    for(size_t i = 0; i < 0xFFFFFF1; i++) {

    }
    t1->wait();
  }
  return 0;
}

void func() {
  os::Tasking::unlock_scheduler();
  auto t = os::Tasking::Task::start(&func2);
  Screen::getInstance().write("foo!\n");
  t->wait();
  for(size_t i = 0; i < 0xFFFFFFF; i++) { }
}

void func2() {
  os::Tasking::unlock_scheduler();
  Screen::getInstance().write("bar!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  Screen::getInstance().write("baz\n");
}

void func3() {
  os::Tasking::unlock_scheduler();
  Screen::getInstance().write("barbaz fast!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  Screen::getInstance().write("barbaz fast end!\n");
}

void func4() {
  os::Tasking::unlock_scheduler();
  Screen::getInstance().write("barbaz slow!\n");
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  for(size_t i = 0; i < 0xFFFFFF1; i++) { }
  Screen::getInstance().write("barbaz slow end!\n");
}