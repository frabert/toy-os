#pragma once

#include <stddef.h>
#include <function_objects.h>
#include "kmalloc.h"

namespace os {
  template<typename T, typename Compare = os::std::less<T>, typename Allocator = os::KernelAllocator<T>>
  class OrderedArray {
  public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using difference_type = ptrdiff_t;
    using size_type = size_t;

    OrderedArray(size_t size)
      : m_size(size)
      , m_alloc(Allocator())
      , m_data(m_alloc.allocate(size)) {}

  iterator begin() { return m_data; }
  iterator end() { return m_data + m_size; }
  const_iterator cbegin() const { return static_cast<const_iterator>(m_data); }
  const_iterator cend() const { return static_cast<const_iterator>(m_data + m_size); }

  size_type size() const { return m_size; }
  bool empty() const { return m_size == 0; }

  bool operator==(const OrderedArray<T, Compare, Allocator>& x) {
    if(x.size() == size()) {
      auto j = cbegin();
      for(auto i = x.cbegin(); i != x.cend(); i++) {
        if(os::std::not_equal_to(*i, *j)) return false;
        j++;
      }
      return true;
    } else {
      return false;
    }
  }

  bool operator!=(const OrderedArray<T, Compare, Allocator>& x) {
    return !(*this==x);
  }

  private:
    size_t m_size;
    Allocator m_alloc;
    T* m_data;
  };
}
