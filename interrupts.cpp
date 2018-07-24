#include "interrupts.h"

#include "screen.h"
#include "ports.h"

using namespace os::Interrupts;
using os::Screen;

InterruptServiceRoutine* interrupt_handlers[256];

void os::Interrupts::register_interrupt_handler(uint8_t n, InterruptServiceRoutine* handler)
{
  interrupt_handlers[n] = handler;
}

extern "C" void isr_handler(Registers regs) {
  auto handler = interrupt_handlers[regs.int_no];

  if(handler != nullptr) {
    interrupt_handlers[regs.int_no](regs);
  } else {
    Screen::getInstance().write("\nUnhandled interrupt %\n", regs.int_no);
  }
}

extern "C" void irq_handler(Registers regs) {
  // Send an EOI (end of interrupt) signal to the PICs.
  // If this interrupt involved the slave.
  if (regs.int_no >= 40) {
    // Send reset signal to slave.
    port_write<uint8_t>(0xA0, 0x20);
  }
  // Send reset signal to master. (As well as slave, if necessary).
  port_write<uint8_t>(0x20, 0x20);

  if (interrupt_handlers[regs.int_no] != 0) {
    auto handler = interrupt_handlers[regs.int_no];
    if(handler != nullptr) handler(regs);
  }
}