#pragma once

namespace os {
  class Spinlock {
  public:
    void acquire();
    void release();
  private:
    volatile int m_val = 0;
  };
};