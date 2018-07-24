#include "debug.h"
#include "ports.h"

void debug_write(const char *c) {
  while(*c) {
    port_write<uint8_t>(0xE9, *c);
    c++;
  }
}

void debug_break() {
  port_write<uint16_t>(0x8A00, 0x8A00);
  port_write<uint16_t>(0x8A00, 0x08AE0);
}