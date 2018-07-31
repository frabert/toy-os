#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kallocator.h>
#include <kassert.h>

namespace os {
  template<typename T = uint32_t, typename Allocator = os::KernelAllocator<T>>
  class bitset {
  public:
    bitset(size_t size) 
        : m_alloc(Allocator())
        , m_size(size)
        , m_numValues(size / (sizeof(T) * 8))
        , m_values(m_alloc.allocate(m_numValues)) {
      for(size_t i = 0; i < m_numValues; i++) {
        m_values[i] = 0;
      }
    }

    ~bitset() {
      m_alloc.deallocate(m_values, size / (sizeof(T) * 8));
    }

    size_t size() const {
      return m_size;
    }

    size_t firstFree() const {
      for(size_t i = 0; i < m_numValues; i++) {
        T val = m_values[i];
        if(val == (T)-1) continue;
        for(size_t j = 0; j < sizeof(T) * 8; j++) {
          T mask = 1 << j;
          if(mask & val) continue;
          return i * sizeof(T) * 8 + j;
        }
      }
      return (T)-1;
    }

    void set(size_t i) {
      assert(i < m_size);
      size_t idx = i / (sizeof(T) * 8);
      size_t offs = i % (sizeof(T) * 8);

      m_values[idx] |= 1 << offs;
    }

    bool test(size_t i) const {
      assert(i < m_size);
      size_t idx = i / (sizeof(T) * 8);
      size_t offs = i % (sizeof(T) * 8);

      return m_values[idx] & (1 << offs);
    }

    void unset(size_t i) {
      assert(i < m_size);
      size_t idx = i / (sizeof(T) * 8);
      size_t offs = i % (sizeof(T) * 8);

      m_values[idx] &= ~(1 << offs);
    }

  private:
    Allocator m_alloc;
    size_t m_size;
    size_t m_numValues;
    T* m_values;
  };
};