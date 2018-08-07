#include <stdlib.h>
#include <kassert.h>
#include "../kheap.h"

namespace os {
  template <class T>
  struct KernelAllocator {
    typedef T value_type;
    KernelAllocator() = default;
    template <class U> constexpr KernelAllocator(const KernelAllocator<U>&) noexcept {}
    T* allocate(size_t n) {
      if(n > (size_t)-1 / sizeof(T)) panic("Bad allocation");
      if(auto p = static_cast<T*>(kmalloc(n*sizeof(T)))) return p;
      panic("Bad allocation");
      return nullptr;
    }
    void deallocate(T* p, size_t) noexcept { kfree(p); }
  };
  template <class T, class U>
  bool operator==(const KernelAllocator<T>&, const KernelAllocator<U>&) { return true; }
  template <class T, class U>
  bool operator!=(const KernelAllocator<T>&, const KernelAllocator<U>&) { return false; }
}