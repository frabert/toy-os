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

    struct PageDirectory;
    using PageTable = std::array<PageTableEntry, 1024>;

    /**
     * \brief Initializes paging
     * 
     * \param map GRUB's memory map
     * \param mapLength Memory map length
     */
    void init(multiboot_memory_map_t* map, size_t mapLength);

    std::pair<uintptr_t, bool> translate(uintptr_t virtAddr);
    inline std::pair<void*, bool> translate(void* virtAddr) {
      auto p = translate((uintptr_t)virtAddr);
      return {(void*)p.first, p.second};
    }

    /**
     * \brief Returns the number of bytes found in the map
     */
    size_t getHeapSize();

    /**
     * \brief Returns the number of bytes in the map that are not allocated yet
     */
    size_t getFreeHeap();

    /**
     * \brief Changes the current page directory
     * 
     * \param dir 
     */
    void switchDirectory(PageDirectory* dir);

    void switchDirectory();

    PageDirectory* currentDirectory();

    struct ThreadData {
      PageDirectory* directory;
      uintptr_t virtualStackStart;
      uintptr_t physicalStackStart;
    };

    /**
     * \brief Allocates a page for the stack of a new thread
     * 
     * \return A pair of the new directory and a pointer to the beginning of the stack 
     */
    ThreadData makeThread();

    /**
     * \brief Whether the heap is active or not
     */
    bool heapActive();
  }
}