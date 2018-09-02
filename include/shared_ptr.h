#pragma once

#include <stddef.h>
#include <stdint.h>
#include <kassert.h>
#include <function_objects.h>

namespace os {
  namespace std {
    template<typename T>
    class shared_ptr {
    public:
      using nullptr_t = decltype(nullptr);

      shared_ptr() : m_data(nullptr) {}
      shared_ptr(nullptr_t) : m_data(nullptr) {}
      explicit shared_ptr(T* ptr) {
        if(ptr != nullptr) {
          m_data = new shared_data();
          m_data->ptr = ptr;
          m_data->refcount = 1;
        } else {
          m_data = nullptr;
        }
      }

      shared_ptr(const shared_ptr& p) {
        if(p) {
          m_data = p.m_data;
          m_data->refcount++;
        } else {
          m_data = nullptr;
        }
      }

      ~shared_ptr() {
        if(m_data != nullptr) {
          m_data->refcount--;
          if(m_data->refcount == 0) {
            delete m_data->ptr;
            delete m_data;
          }
        }
      }

      shared_ptr& operator=(const shared_ptr& p) {
        if(m_data != nullptr && m_data != p.m_data) {
          m_data->refcount--;
          if(m_data->refcount == 0) {
            delete m_data->ptr;
            delete m_data;
          }
        }

        m_data = p.m_data;
        if(m_data != nullptr) m_data->refcount++;

        return *this;
      }

      T* get() const {
        if(m_data) {
          return m_data->ptr;
        }
        return nullptr;
      }

      T& operator*() const {
        assert(m_data != nullptr);
        return *(m_data->ptr);
      }

      T* operator->() const {
        assert(m_data != nullptr);
        return get();
      }

      size_t use_count() const {
        if(m_data == nullptr) return 0;
        return m_data->refcount;
      }

      explicit operator bool() const {
        return m_data != nullptr;
      }

    private:
      struct shared_data {
        T* ptr;
        size_t refcount;
      };

      shared_data* m_data;
    };

    template<typename T, typename... Args>
    shared_ptr<T> make_shared(Args&&... args) {
      return shared_ptr<T>(new T(args...));
    }

    template<typename T, typename U>
    bool operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) {
      return lhs.get() == rhs.get();
    }

    template<typename T>
    bool operator==(const shared_ptr<T>& lhs, nullptr_t) {
      return !lhs;
    }

    template<typename T, typename U>
    bool operator!=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) {
      return !(lhs == rhs);
    }

    template<typename T>
    bool operator!=(const shared_ptr<T>& lhs, nullptr_t) {
      return lhs;
    }

    template<typename T>
    bool operator<(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) {
      return less<T*>()(lhs.get(), rhs.get());
    }

    template<typename T>
    bool operator<=(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) {
      return less_equal<T*>()(lhs.get(), rhs.get());
    }

    template<typename T>
    bool operator>(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) {
      return greater<T*>()(lhs.get(), rhs.get());
    }

    template<typename T>
    bool operator>=(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) {
      return greater_equal<T>()(lhs.get(), rhs.get());
    }
  }
}
