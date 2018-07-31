#include "descriptor_tables.h"

#include <array.h>
#include <string.h>
#include "debug.h"
#include "ports.h"

using os::std::array;
using namespace os;

enum class PrivilegeLevel : uint8_t {
  Ring0 = 0,
  Ring1 = 1,
  Ring2 = 2,
  Ring3 = 3
};

enum class Granularity : uint8_t {
  Byte     = 0 << 7,
  Kilobyte = 1 << 7
};

// Data segment

enum class DataAccess : uint8_t{
  ReadOnly  = 0 << 1,
  ReadWrite = 1 << 1
};

enum class ExpansionDirection : uint8_t {
  ExpandUp   = 0 << 2,
  ExpandDown = 1 << 2
};

// Code segment

enum class ReadEnable : uint8_t {
  ExecuteOnly = 0 << 1,
  ReadExecute = 1 << 1
};

enum class Conformance : uint8_t {
  Nonconforming = 0 << 2,
  Conforming    = 1 << 2
};

#pragma pack(push, 1)
struct GdtEntry {
  uint16_t limit_low;           // The lower 16 bits of the limit.
  uint16_t base_low;            // The lower 16 bits of the base.
  uint8_t  base_middle;         // The next 8 bits of the base.

  uint8_t access;              // Access flags, determine what ring this segment can be used in.
  uint8_t granularity;

  uint8_t  base_high;

  static GdtEntry makeDataSegment(uintptr_t base, uint32_t limit,
      PrivilegeLevel level, DataAccess access,
      ExpansionDirection direction = ExpansionDirection::ExpandUp,
      Granularity gran = Granularity::Kilobyte) {

    GdtEntry entry;
    
    entry.base_low    = (base & 0xFFFF);
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high   = (base >> 24) & 0xFF;
    entry.limit_low   = (limit & 0xFFFF);
    entry.granularity = (limit >> 16) & 0x0F;

    uint8_t g = static_cast<uint8_t>(gran);
    uint8_t pvl = static_cast<uint8_t>(level) << 5;
    uint8_t acc = static_cast<uint8_t>(access);
    uint8_t dir = static_cast<uint8_t>(direction);

    entry.granularity |= (g | 0x40);
    entry.access      = (pvl | acc | dir | 0x90);

    return entry;
  }

  static GdtEntry makeCodeSegment(uintptr_t base, uint32_t limit,
      PrivilegeLevel level, ReadEnable access,
      Conformance conf = Conformance::Nonconforming,
      Granularity gran = Granularity::Kilobyte) {

    GdtEntry entry;
    
    entry.base_low    = (base & 0xFFFF);
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high   = (base >> 24) & 0xFF;
    entry.limit_low   = (limit & 0xFFFF);
    entry.granularity = (limit >> 16) & 0x0F;

    uint8_t g = static_cast<uint8_t>(gran);
    uint8_t pvl = static_cast<uint8_t>(level) << 5;
    uint8_t acc = static_cast<uint8_t>(access);
    uint8_t c = static_cast<uint8_t>(conf);

    entry.granularity |= (g | 0x40);
    entry.access      = (pvl | acc | c | 0x98);

    return entry;
  }
};

struct GdtPointer {
  uint16_t limit;
  uintptr_t base;

  template<size_t N>
  static inline GdtPointer makePointer(const array<GdtEntry, N>& entries) {
    return {
      sizeof(entries) - 1,
      (uintptr_t)&entries
    };
  }
};

struct IdtEntry {
  uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
  uint16_t sel;                 // Kernel segment selector.
  uint8_t  always0;             // This must always be zero.
  uint8_t  flags;               // More flags. See documentation.
  uint16_t base_hi;             // The upper 16 bits of the address to jump to.

  using interrupt_handler = void(void);

  static IdtEntry makeEntry(interrupt_handler handler, uint8_t selector, PrivilegeLevel level) {
    uintptr_t addr = (uintptr_t)handler;
    uint8_t lev = static_cast<uint8_t>(level) << 5;

    return {
      addr & 0xFFFF,
      selector,
      0,
      lev | 0x8E,
      (addr >> 16) & 0xFFFF
    };
  }
};

struct IdtPointer {
  uint16_t limit;
  uintptr_t base;

  template<size_t N>
  static inline IdtPointer makePointer(const array<IdtEntry, N>& entries) {
    return {
      sizeof(entries) - 1,
      (uintptr_t)&entries
    };
  }
};
#pragma pack(pop)

// Lets us access our ASM functions from our C code.
extern "C" {
  void gdt_flush(GdtPointer*);
  void idt_flush(IdtPointer*);
}

extern "C" IdtEntry::interrupt_handler *isr_handlers[];
extern "C" uint32_t isr_handlers_count;

array<GdtEntry, 5> gdt_entries;
GdtPointer gdt_ptr;
array<IdtEntry, 256> idt_entries;
IdtPointer idt_ptr;

static void init_gdt()
{
  gdt_entries[0] = {0};
  gdt_entries[1] = GdtEntry::makeCodeSegment(0, 0xFFFFFFFF, PrivilegeLevel::Ring0, ReadEnable::ReadExecute);
  gdt_entries[2] = GdtEntry::makeDataSegment(0, 0xFFFFFFFF, PrivilegeLevel::Ring0, DataAccess::ReadWrite);
  gdt_entries[3] = GdtEntry::makeCodeSegment(0, 0xFFFFFFFF, PrivilegeLevel::Ring3, ReadEnable::ReadExecute);
  gdt_entries[4] = GdtEntry::makeDataSegment(0, 0xFFFFFFFF, PrivilegeLevel::Ring3, DataAccess::ReadWrite);

  gdt_ptr = GdtPointer::makePointer(gdt_entries);

  gdt_flush(&gdt_ptr);
}

static void init_idt()
{
  // Remap the PIC
  outb(0x20, 0x11);
  outb(0xA0, 0x11);
  outb(0x21, 0x20);
  outb(0xA1, 0x28);
  outb(0x21, 0x04);
  outb(0xA1, 0x02);
  outb(0x21, 0x01);
  outb(0xA1, 0x01);
  outb(0x21, 0x00);
  outb(0xA1, 0x00);

  memset(&idt_entries, 0, sizeof(IdtEntry)*256);

  for(uint32_t i = 0; i < isr_handlers_count; i++) {
    idt_entries[i] = IdtEntry::makeEntry(isr_handlers[i], 0x08, PrivilegeLevel::Ring0);
  }

  idt_ptr = IdtPointer::makePointer(idt_entries);

  idt_flush(&idt_ptr);
}

void os::DescriptorTables::init() {
  init_gdt();
  init_idt();
}