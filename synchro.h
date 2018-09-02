#pragma once

namespace os {
  class Spinlock {
  public:
    void acquire();
    void release();
  private:
    volatile int m_val = 0;
  };

  template<typename T>
  class scoped_lock {
  public:
    scoped_lock(T& l)
        : m_lock(l) {
      m_lock.acquire();
    }

    ~scoped_lock() {
      m_lock.release();
    }

  private:
    T& m_lock;
  };
}