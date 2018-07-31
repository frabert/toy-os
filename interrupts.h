#pragma once
#include <stdint.h>

namespace os {
  namespace Interrupts {
    constexpr uint8_t IRQ0 = 32;
    constexpr uint8_t IRQ1 = 33;
    constexpr uint8_t IRQ2 = 34;
    constexpr uint8_t IRQ3 = 35;
    constexpr uint8_t IRQ4 = 36;
    constexpr uint8_t IRQ5 = 37;
    constexpr uint8_t IRQ6 = 38;
    constexpr uint8_t IRQ7 = 39;
    constexpr uint8_t IRQ8 = 40;
    constexpr uint8_t IRQ9 = 41;
    constexpr uint8_t IRQ10 = 42;
    constexpr uint8_t IRQ11 = 43;
    constexpr uint8_t IRQ12 = 44;
    constexpr uint8_t IRQ13 = 45;
    constexpr uint8_t IRQ14 = 46;
    constexpr uint8_t IRQ15 = 47;

    struct Registers {
      uint32_t ds; // Data segment selector
      uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
      uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
      uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
    };

    using InterruptServiceRoutine = void(Registers*);
    void registerInterruptHandler(uint8_t n, InterruptServiceRoutine* handler);
  }
}