#pragma once
#include <stdint.h>

namespace os {
  namespace Interrupts {
    constexpr int IRQ0 = 32;
    constexpr int IRQ1 = 33;
    constexpr int IRQ2 = 34;
    constexpr int IRQ3 = 35;
    constexpr int IRQ4 = 36;
    constexpr int IRQ5 = 37;
    constexpr int IRQ6 = 38;
    constexpr int IRQ7 = 39;
    constexpr int IRQ8 = 40;
    constexpr int IRQ9 = 41;
    constexpr int IRQ10 = 42;
    constexpr int IRQ11 = 43;
    constexpr int IRQ12 = 44;
    constexpr int IRQ13 = 45;
    constexpr int IRQ14 = 46;
    constexpr int IRQ15 = 47;

    struct Registers {
      uint32_t ds;                  // Data segment selector
      uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
      uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
      uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
    };

    using InterruptServiceRoutine = void(Registers);
    void register_interrupt_handler(uint8_t n, InterruptServiceRoutine* handler);
  }
}