#pragma once

#include <stdint.h>

namespace os {
  namespace Paging {
    struct Page {
      uint32_t present    : 1;   // Page present in memory
      uint32_t rw         : 1;   // Read-only if clear, readwrite if set
      uint32_t user       : 1;   // Supervisor level only if clear
      uint32_t accessed   : 1;   // Has the page been accessed since last refresh?
      uint32_t dirty      : 1;   // Has the page been written to since last refresh?
      uint32_t unused     : 7;   // Amalgamation of unused and reserved bits
      uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
    };

    struct PageTable {
      Page pages[1024];
    };

    struct PageDirectory {
      /**
      Array of pointers to pagetables.
      **/
      PageTable *tables[1024];
      /**
         Array of pointers to the pagetables above, but gives their *physical*
         location, for loading into the CR3 register.
      **/
      uintptr_t tablesPhysical[1024];
      /**
         The physical address of tablesPhysical. This comes into play
         when we get our kernel heap allocated and the directory
         may be in a different location in virtual memory.
      **/
      uintptr_t physicalAddr;
    };

    void init();

    void switch_page_directory(PageDirectory *page);

    Page *get_page(uintptr_t address, bool make, PageDirectory *dir);
  }
}