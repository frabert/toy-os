#include "timer.h"
#include "interrupts.h"
#include "ports.h"
#include "screen.h"
#include "debug.h"
#include "tasking.h"
#include "time.h"

using namespace os::Time;

void timer_handler(os::Interrupts::Registers*);

static os::Time::TimeSpan timeSinceBoot{0};
static os::Time::TimeSpan timerPeriod{0};

void timer_handler(os::Interrupts::Registers*) {
  timeSinceBoot += timerPeriod;
  os::Tasking::timer_tick(timeSinceBoot);
  //os::Tasking::switchTasks();
}

os::Time::TimeSpan os::Time::since_boot() {
  return timeSinceBoot;
}

void os::Timer::init(uint32_t frequency) {
  timerPeriod = 1_s / frequency;
  // Firstly, register our timer callback.
  registerInterruptHandler(os::Interrupts::IRQ0, timer_handler);

  // The value we send to the PIT is the value to divide it's input clock
  // (1193180 Hz) by, to get our required frequency. Important to note is
  // that the divisor must be small enough to fit into 16-bits.
  uint16_t divisor = 1193180 / frequency;

  // Send the command byte.
  outb(0x43, 0x36);

  // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
  uint8_t l = (uint8_t)(divisor & 0xFF);
  uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

  // Send the frequency divisor.
  outb(0x40, l);
  outb(0x40, h);
}