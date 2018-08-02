#include "synchro.h"

extern "C" void spinlock_acquire(volatile int* spinlock);
extern "C" void spinlock_release(volatile int* spinlock);

void os::Spinlock::acquire() {
  spinlock_acquire(&m_val);
}

void os::Spinlock::release() {
  spinlock_release(&m_val);
}