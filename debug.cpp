#include "debug.h"
#include "ports.h"
#include "screen.h"
#include "reflection.h"

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

// From https://wiki.osdev.org/Stack_Trace
void debug_stacktrace(unsigned int MaxFrames) {
  // Stack contains:
  //  Second function argument
  //  First function argument (MaxFrames)
  //  Return address in calling function
  //  EBP of calling function (pointed to by current EBP)
  unsigned int * ebp = &MaxFrames - 2;
  os::Screen::getInstance().write("Stack trace:\n");
  for(unsigned int frame = 0; frame < MaxFrames; ++frame) {
    unsigned int eip = ebp[1];
    if(eip == 0)
        // No caller on stack
        break;
    // Unwind to previous stack frame
    ebp = reinterpret_cast<unsigned int *>(ebp[0]);
    //unsigned int * arguments = &ebp[2];
    auto names = os::Reflection::getSymbolName(eip);
    os::Screen::getInstance().write("  % (% + %)\n", (void*)eip, names.first, (void*)names.second);
  }
}