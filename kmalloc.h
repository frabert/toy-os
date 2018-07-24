#pragma once

#include <stddef.h>
#include <stdint.h>
#include "kassert.h"

void* kmalloc(size_t size);

void* kmalloc_align(size_t size, void** physical);
inline void* kmalloc_align(size_t size) {
  void* physical;
  return kmalloc_align(size, &physical);
}

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
    void deallocate(T* p, size_t) noexcept { }
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
    void deallocate(T* p, size_t) noexcept { }
  };
  template <class T, class U>
  bool operator==(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return true; }
  template <class T, class U>
  bool operator!=(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return false; }
}