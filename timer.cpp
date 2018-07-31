#include "timer.h"
#include "interrupts.h"
#include "ports.h"
#include "screen.h"
#include "debug.h"

static void timer_callback(os::Interrupts::Registers regs) {
}

void os::Timer::init(uint32_t frequency) {
  // Firstly, register our timer callback.
  registerInterruptHandler(os::Interrupts::IRQ0, timer_callback);

  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  uint32_t divisor = 1193180 / frequency;

  // Send the command byte.
  outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8_t l = (uint8_t)(divisor & 0xFF);
  uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);
}