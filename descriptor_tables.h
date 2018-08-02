#pragma once
#include <stdint.h>

namespace os {
  namespace DescriptorTables {
    /**
     * \brief Initializes the GDT and IDT
     * 
     */
    void init();
  }
}