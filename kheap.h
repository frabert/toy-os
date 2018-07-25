#pragma once

#include <stdint.h>
#include <stddef.h>
#include "ordered_array.h"

void* kmalloc(size_t size);

void* kmalloc_align(size_t size, void** physical);
inline void* kmalloc_align(size_t size) {
  void* physical;
  return kmalloc_align(size, &physical);
}

void kfree(void *p);

namespace os {
  template <class T>
  struct KernelAllocator {
    typedef T value_type;
    KernelAllocator() = default;
    template <class U> constexpr KernelAllocator(const KernelAllocator<U>&) noexcept {}
    T* allocate(size_t n) {
      KASSERT_MSG(!(n > (size_t)-1 / sizeof(T)), "Bad allocation");
      if(auto p = static_cast<T*>(kmalloc(n*sizeof(T)))) return p;
      KASSERT_MSG(0, "Bad allocation");
    }
    void deallocate(T* p, size_t) noexcept { kfree(p); }
  };
  template <class T, class U>
  bool operator==(const KernelAllocator<T>&, const KernelAllocator<U>&) { return true; }
  template <class T, class U>
  bool operator!=(const KernelAllocator<T>&, const KernelAllocator<U>&) { return false; }

  template <class T>
  struct AlignedKernelAllocator {
    typedef T value_type;
    AlignedKernelAllocator() = default;
    template <class U> constexpr AlignedKernelAllocator(const KernelAllocator<U>&) noexcept {}
    T* allocate(size_t n) {
      KASSERT_MSG(!(n > (size_t)-1 / sizeof(T)), "Bad allocation");
      if(auto p = static_cast<T*>(kmalloc_align(n*sizeof(T)))) return p;
      KASSERT_MSG(0, "Bad allocation");
    }
    void deallocate(T* p, size_t) noexcept { kfree(p); }
  };
  template <class T, class U>
  bool operator==(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return true; }
  template <class T, class U>
  bool operator!=(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return false; }

  constexpr uintptr_t KHEAP_START = 0xC0000000;
  constexpr size_t KHEAP_INITIAL_SIZE = 0x100000;
  constexpr size_t HEAP_INDEX_SIZE = 0x20000;
  constexpr uint32_t HEAP_MAGIC = 0x123890AB;
  constexpr size_t HEAP_MIN_SIZE = 0x70000;

  struct Header {
    uint32_t magic;   // Magic number, used for error checking and identification.
    bool is_hole;   // 1 if this is a hole. 0 if this is a block.
    size_t size;    // size of the block, including the end footer.

    bool operator <(const Header& x) const {
      return size < x.size;
    }
  };

  class Heap {
  public:
    using container_type = os::OrderedArray<Header*, KernelAllocator<Header*>>;

    Heap(uintptr_t start, uintptr_t end, uintptr_t max, bool supervisor, bool readonly);

  void* alloc(size_t size, bool align = false);
  void free(void* ptr);

  private:
    container_type m_index;
    uintptr_t m_start_address; // The start of our allocated space.
    uintptr_t m_end_address;   // The end of our allocated space. May be expanded up to max_address.
    uintptr_t m_max_address;   // The maximum address the heap can be expanded to.
    bool m_supervisor;     // Should extra pages requested by us be mapped as supervisor-only?
    bool m_readonly;       // Should extra pages requested by us be mapped as read-only?

    container_type::iterator findSmallestHole(size_t size, bool page_align);
    void expand(size_t newSize);
    int32_t contract(size_t newSize);
  };
}