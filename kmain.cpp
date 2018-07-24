#include "multiboot.h"
#include "screen.h"
#include "utils.h"
#include "descriptor_tables.h"
#include "ports.h"
#include "debug.h"
#include "paging.h"
#include <stddef.h>

using os::Screen;

template<typename Function>
void test(Function f) {
  f(1);
  f(2);
  f(3);
}

void func(int x) {
  Screen::getInstance().write("world: %\n", x);
}

extern "C" int kmain(multiboot_info_t *mboot_ptr)
{
  Screen::init();
  Screen& screen = Screen::getInstance();
  screen.setColors(Screen::Color::Gray, Screen::Color::Blue);
  screen.clear();

  screen.write("Toy OS booting\n");

  os::DescriptorTables::init();
  screen.write("Descriptor tables initialized\n");
  os::Paging::init();
  screen.write("Paging initialized\n");

  test([&](int x) {
    screen.write("hello: %\n", x);
  });

  test(func);

  return 0;
}