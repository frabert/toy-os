#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kassert.h>

void* kmalloc(size_t size);

void* kmalloc_align(size_t size, void** physical);
inline void* kmalloc_align(size_t size) {
  void* physical;
  return kmalloc_align(size, &physical);
}

void kfree(void *p);

void* krealloc(void* ptr, size_t s);

namespace os {
  template <class T>
  struct AlignedKernelAllocator {
    typedef T value_type;
    AlignedKernelAllocator() = default;
    template <class U> constexpr AlignedKernelAllocator(const AlignedKernelAllocator<U>&) noexcept {}
    T* allocate(size_t n) {
      if((n > (size_t)-1 / sizeof(T))) { panic("Bad allocation"); }
      if(auto p = static_cast<T*>(kmalloc_align(n*sizeof(T)))) return p;
      panic("Bad allocation");
    }
    void deallocate(T* p, size_t) noexcept { kfree(p); }
  };
  template <class T, class U>
  bool operator==(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return true; }
  template <class T, class U>
  bool operator!=(const AlignedKernelAllocator<T>&, const AlignedKernelAllocator<U>&) { return false; }
}