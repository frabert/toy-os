#include "multiboot.h"
#include "screen.h"
#include "utils.h"
#include "descriptor_tables.h"
#include "ports.h"
#include "debug.h"
#include "paging.h"
#include "kheap.h"
#include <stddef.h>

using os::Screen;

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

  return 0;
}