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
        , m_numValues((size - 1) / (sizeof(T) * 8) + 1)
        , m_freeSlots(m_size)
        , m_values(m_alloc.allocate(m_numValues)) {
      for(size_t i = 0; i < m_numValues; i++) {
        m_values[i] = 0;
      }
    }

    size_t size() const {
      return m_size;
    }

    size_t free_slots() const {
      return m_freeSlots;
    }

    size_t free_slots_after(size_t n) const {
      size_t count = 0;
      size_t i = n + 1;
      while(i < m_size) {
        if(test(i++)) return count;
      }
      return count;
    }

    size_t firstFree() const {
      if(m_freeSlots == 0) return (T)-1;

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

    size_t freeSpan(size_t n) const {
      assert(n < sizeof(T) * 8);
      if(m_freeSlots < n) return (T)-1;

      T mask = 0;
      for(size_t i = 0; i < n; i++) {
        mask |= 1 << i;
      }

      for(size_t i = 0; i < m_numValues; i++) {
        T val = m_values[i];
        if(val == (T)-1) continue;
        for(size_t j = 0; j < sizeof(T) * 8; j++) {
          if(!(val & (mask << j))) {
            return i * sizeof(T) * 8 + j;
          }
        }
      }
      return (T)-1;
    }

    void set(size_t i) {
      assert(i < m_size);
      size_t idx = i / (sizeof(T) * 8);
      size_t offs = i % (sizeof(T) * 8);

      if(!test(i)) m_freeSlots--;

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

      if(test(i)) m_freeSlots++;

      m_values[idx] &= ~(1 << offs);
    }

  private:
    Allocator m_alloc;
    size_t m_size;
    size_t m_numValues;
    size_t m_freeSlots;
    T* m_values;
  };
}