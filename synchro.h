#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void spinlock_acquire(volatile int* spinlock);
void spinlock_release(volatile int* spinlock);

#ifdef __cplusplus
}
#endif