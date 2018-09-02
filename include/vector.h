#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kassert.h>
#include <stdlib.h>

namespace os {
  namespace std {
    template<typename T>
    class vector {
    public:
      using value_type = T;
      using reference = T&;
      using const_reference = const T&;
      using pointer = T*;
      using const_pointer = const T*;
      using size_type = size_t;
      using difference_type = ptrdiff_t;
      using iterator = T*;
      using const_iterator = const T*;

      reference operator[](size_t i) {
        return m_buffer[i];
      }

      const_reference operator[](size_t i) const {
        return m_buffer[i];
      }

      reference at(size_t i) {
        assert(i < m_size);
        return m_buffer[i];
      }

      const_reference at(size_t i) const {
        assert(i < m_size);
        return m_buffer[i];
      }

      reference front() {
        return *m_buffer;
      }

      const_reference front() const {
        return *m_buffer;
      }

      reference back() {
        return *(m_buffer + m_size - 1);
      }

      const_reference back() const {
        return *(m_buffer + m_size - 1);
      }

      pointer data() {
        return m_buffer;
      }

      const_pointer data() const {
        return m_buffer;
      }

      iterator begin() {
        return m_buffer;
      }

      const_iterator begin() const {
        return m_buffer;
      }

      const_iterator cbegin() const {
        return m_buffer;
      }

      iterator end() {
        return m_buffer + m_size;
      }

      const_iterator end() const {
        return m_buffer + m_size;
      }

      const_iterator cend() const {
        return m_buffer + m_size;
      }

      bool empty() const {
        return m_size == 0;
      }

      size_t size() const {
        return m_size;
      }

      size_t max_size() const {
        return (size_t)-1;
      }

      size_t capacity() const {
        return m_capacity;
      }
      
      void push_back(const T& v) {
        if(m_capacity == 0) {
          m_capacity = 16;
          m_buffer = (T*)malloc(sizeof(T) * 16);
        } else if(m_capacity < m_size + 1) {
          m_capacity *= 2;
          m_buffer = (T*)realloc(m_buffer, sizeof(T) * m_capacity);
        }
        m_buffer[m_size++] = v;
      }

      void pop_back() {
        m_size--;
        m_buffer[m_size].~T();
      }
    private:
      T* m_buffer;
      size_t m_size;
      size_t m_capacity;
    };
  }
}