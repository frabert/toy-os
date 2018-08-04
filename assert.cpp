#include <kassert.h>
#include "screen.h"
#include "debug.h"

void panic_raw(const char *c) {
  asm volatile("cli");
  os::Screen::getInstance().write(c);
  debug_stacktrace(1000);
  debug_break();
  while(1) {;}
}