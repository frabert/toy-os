#pragma once

#include <pair.h>
#include "multiboot.h"

namespace os {
  namespace Reflection {
    /**
     * \brief Initializes kernel reflection
     * 
     * \param elf Multiboot ELF header
     */
    void init(multiboot_elf_section_header_table_t elf);

    /**
     * \brief Returns the name of the symbol at address \p addr
     * 
     * \param addr The address of the symbol
     * \return A pair containing the name of the symbol and the name of the 
     *         section in which the symbol is located
     */
    std::pair<const char*, const char*> getSymbolName(uintptr_t addr);

    uintptr_t getKernelStart();
    uintptr_t getKernelEnd();
  }
}