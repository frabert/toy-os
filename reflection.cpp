#include "reflection.h"
#include <kassert.h>
#include <stddef.h>

#define SHF_WRITE              0x1
#define SHF_ALLOC              0x2
#define SHF_EXECINSTR          0x4
#define SHF_MASKPROC    0xf0000000

enum class SectionType : uint32_t {
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_LOPROC = 0x70000000,
  SHT_HIPROC = 0x7fffffff,
  SHT_LOUSER = 0x80000000,
  SHT_HIUSER = 0xffffffff
};

struct Elf32_Shdr {
  uint32_t sh_name;
  SectionType sh_type;
  uint32_t sh_flags;
  uintptr_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
};

struct Elf32_Sym {
  uint32_t  st_name;                /* Symbol name (string tbl index) */
  uintptr_t st_value;               /* Symbol value */
  uint32_t  st_size;                /* Symbol size */
  unsigned char st_info;            /* Symbol type and binding */
  unsigned char st_other;           /* Symbol visibility */
  uint16_t st_shndx;                /* Section index */
};

static Elf32_Shdr* headers = nullptr;

static Elf32_Shdr* symtab_hdr = nullptr;
static size_t symtab_entries = 0;

static Elf32_Shdr* shstrtab_hdr = nullptr;
static Elf32_Shdr* strtab_hdr = nullptr;

static Elf32_Sym* symtab = nullptr;
static const char* strtab = nullptr; 
static const char* shstrtab = nullptr; 

static uintptr_t kernelStart = (uintptr_t)-1;
static uintptr_t kernelEnd = 0;

void os::Reflection::init(multiboot_elf_section_header_table_t elf) {
  Elf32_Shdr* hdrs = (Elf32_Shdr*)elf.addr;
  headers = hdrs;
  shstrtab_hdr = hdrs + elf.shndx;
  shstrtab = (const char*)shstrtab_hdr->sh_addr;

  for(size_t i = 0; i < elf.num; i++) {
    Elf32_Shdr* hdr = &hdrs[i];
    if(hdr->sh_addr < kernelStart) {
      kernelStart = hdr->sh_addr;
    }

    if(hdr->sh_addr + hdr->sh_size > kernelEnd) {
      kernelEnd = hdr->sh_addr + hdr->sh_size;
    }

    // There are generally two string tables, one is for section names,
    // the other is for symbol names. We need to find symbol table, we already
    // have the section table.
    if(hdr->sh_type == SectionType::SHT_STRTAB && i != elf.shndx) {
      strtab_hdr = hdr;
    } else if(hdr->sh_type == SectionType::SHT_SYMTAB) {
      symtab_hdr = hdr;
      symtab_entries = hdr->sh_size / sizeof(Elf32_Sym);
    }
  }

  assert(strtab_hdr != nullptr);
  assert(symtab_hdr != nullptr);

  symtab = (Elf32_Sym*)symtab_hdr->sh_addr;
  strtab = (const char*)strtab_hdr->sh_addr;
}

uintptr_t os::Reflection::getKernelEnd() {
  return kernelEnd;
}

uintptr_t os::Reflection::getKernelStart() {
  return kernelStart;
}

os::std::pair<const char*, size_t> os::Reflection::getSymbolName(uintptr_t addr) {
  if(strtab_hdr == nullptr) return {"", 0};

  uintptr_t actual_addr = 0;
  size_t idx = -1;

  // We are searching for the largest address that's smaller than the target
  for(size_t i = 0; i < symtab_entries; i++) {
    if(symtab[i].st_value < addr && symtab[i].st_value > actual_addr) {
      idx = i;
      actual_addr = symtab[i].st_value;
    }
  }

  if(idx == (size_t)-1)
    return {"", 0};

  Elf32_Sym sym = symtab[idx];
  const char* name = &strtab[sym.st_name];
  
  return {name, addr - sym.st_value};
}