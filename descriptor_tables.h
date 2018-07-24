#pragma once
#include <stdint.h>

namespace os {
  namespace DescriptorTables {
    #pragma pack(push, 1)
    struct GdtEntry {
      uint16_t limit_low;           // The lower 16 bits of the limit.
      uint16_t base_low;            // The lower 16 bits of the base.
      uint8_t  base_middle;         // The next 8 bits of the base.

      uint8_t access;              // Access flags, determine what ring this segment can be used in.
      uint8_t granularity;

      uint8_t  base_high;
    };

    struct GdtPointer {
      uint16_t limit;
      uint32_t base;
    };

    struct IdtEntry {
      uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
      uint16_t sel;                 // Kernel segment selector.
      uint8_t  always0;             // This must always be zero.
      uint8_t  flags;               // More flags. See documentation.
      uint16_t base_hi;             // The upper 16 bits of the address to jump to.
    };

    struct IdtPointer {
      uint16_t limit;
      uint32_t base;
    };
    #pragma pack(pop)

    void init();
  }
}