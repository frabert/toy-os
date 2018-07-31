#pragma once
#include <stddef.h>
#include <kassert.h>

namespace os {
  namespace std {
    template<typename T, size_t N>
    class array {
    public:
      using value_type = T;
      using size_type = size_t;
      using difference_type = ptrdiff_t;
      using reference = value_type&;
      using const_reference = const value_type&;
      using pointer = value_type*;
      using const_pointer = const value_type*;
      using iterator = pointer;
      using const_iterator = const_pointer;

      constexpr bool empty() const noexcept {
        return N == 0;
      }

      constexpr size_type size() const noexcept {
        return N;
      }

      constexpr size_type max_size() const noexcept {
        return N;
      }

      constexpr const_reference front() const {
        return *m_data;
      }

      constexpr reference front() {
        return *m_data;
      }

      constexpr const_iterator begin() const {
        return m_data;
      }

      constexpr iterator begin() {
        return m_data;
      }

      constexpr const_iterator end() const {
        return m_data + N;
      }

      constexpr iterator end() {
        return m_data + N;
      }

      constexpr const_pointer data() const noexcept {
        return m_data;
      }

      constexpr pointer data() noexcept {
        return m_data;
      }

      constexpr reference at(size_type pos) {
        assert(pos < N);
        return m_data[pos];
      }

      constexpr const_reference at(size_type pos) const {
        assert(pos < N);
        return m_data[pos];
      }

      constexpr reference operator[](size_type pos) {
        return m_data[pos];
      }

      constexpr const_reference operator[](size_type pos) const {
        return m_data[pos];
      }

      void fill(const_reference val) {
        for(size_t i = 0; i < N; i++) {
          m_data[i] = val;
        }
      }

      constexpr const_iterator cbegin() const noexcept {
        return m_data;
      }

      constexpr const_iterator cend() const noexcept {
        return begin() + N;
      }
    private:
      T m_data[N];
    };
  }
}
