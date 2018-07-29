#include "debug.h"
#include "ports.h"

void debug_write(const char *c) {
  while(*c) {
    outb(0xE9, *c);
    c++;
  }
}

void debug_break() {
  outw(0x8A00, 0x8A00);
  outw(0x8A00, 0x08AE0);
}