#pragma once

#include <stdint.h>
#include <array.h>
#include <pair.h>
#include "multiboot.h"

namespace os {
  namespace Paging {
    struct PageDirectoryEntry {
      uint32_t present       : 1;
      uint32_t rw            : 1;
      uint32_t user          : 1;
      uint32_t writeThrough  : 1;
      uint32_t cacheDisabled : 1;
      uint32_t accessed      : 1;
      uint32_t ignore        : 1;
      uint32_t size          : 1;
      uint32_t unused        : 4;
      uint32_t addr          : 20;
    };

    struct PageTableEntry {
      uint32_t present       : 1;
      uint32_t rw            : 1;
      uint32_t user          : 1;
      uint32_t writeThrough  : 1;
      uint32_t cacheDisabled : 1;
      uint32_t accessed      : 1;
      uint32_t dirty         : 1;
      uint32_t zero          : 1;
      uint32_t global        : 1;
      uint32_t unused        : 3;
      uint32_t addr          : 20;
    };

    using PageDirectory = std::array<PageDirectoryEntry, 1024>;
    using PageTable = std::array<PageTableEntry, 1024>;

    void init(multiboot_memory_map_t* map, size_t mapLength);

    void identity_map(uintptr_t start, uintptr_t end, bool rw = true, bool user = false);

    std::pair<uintptr_t, bool> translate(uintptr_t virtAddr);
    inline std::pair<void*, bool> translate(void* virtAddr) {
      auto p = translate((uintptr_t)virtAddr);
      return {(void*)p.first, p.second};
    }
  }
}