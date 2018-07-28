#pragma once

#include <stddef.h>
#include <function_objects.h>
#include "kassert.h"
#include <string.h>
#include <kallocator.h>

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

    OrderedArray(size_t size) {
      m_size = 0;
      m_maxsize = size;
      m_data = m_alloc.allocate(m_maxsize);
      memset(m_data, 0, sizeof(T) * m_maxsize);
    }

    OrderedArray(T* place, size_t size) {
      m_size = 0;
      m_maxsize = size;
      m_data = place;
      memset(m_data, 0, sizeof(T) * m_maxsize);
    }

    ~OrderedArray() {
      m_alloc.deallocate(m_data, m_maxsize);
    }

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

    constexpr reference at(size_type pos) {
      KASSERT_MSG(pos < m_size, "Out of bounds access");
      return m_data[pos];
    }

    constexpr const_reference at(size_type pos) const {
      KASSERT_MSG(pos < m_size, "Out of bounds access");
      return m_data[pos];
    }

    constexpr reference operator[]( size_type pos ) {
      return m_data[pos];
    }

    constexpr const_reference operator[]( size_type pos ) const {
      return m_data[pos];
    }

    constexpr reference front() {
      return *m_data;
    }

    constexpr const_reference front() const {
      return *m_data;
    }

    constexpr reference back() {
      return m_data[m_size - 1];
    }

    constexpr const_reference back() const {
      return m_data[m_size - 1];
    }

    constexpr T* data() noexcept {
      return m_data;
    }

    constexpr const T* data() const noexcept {
      return m_data;
    }

    iterator insert(value_type val) {
      KASSERT_MSG(m_size < m_maxsize, "Out of bounds insertion");

      iterator i = begin();
      while(i != end() && m_comp(*i, val)) {
        i++;
      }

      if(i == end()) {
        m_data[m_size] = val;
        m_size++;
        return end() - 1;
      } else {
        value_type tmp = *i;
        *i = val;
        while (i != end()) {
            i++;
            value_type tmp2 = *i;
            *i = tmp;
            tmp = tmp2;
        }
        m_size++;
        return end() - 1;
      }
    }

    void remove(iterator pos) {
      KASSERT_MSG(pos < end(), "Out of bounds access");
      while (pos < end()) {
        *pos = *(pos + 1);
        pos++;
      }
      m_size--;
    }

  private:
    size_t m_size;
    size_t m_maxsize;
    Allocator m_alloc;
    Compare m_comp;
    T* m_data;
  };
}
