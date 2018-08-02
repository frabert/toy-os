#include <stddef.h>
#include <stdint.h>
#include <kassert.h>
#include "synchro.h"

void *__dso_handle;
void *__gxx_personality_v0;

static os::Spinlock spinlock;

// Defined in the linker.
extern uintptr_t start_ctors;
extern uintptr_t end_ctors;

/// Required for G++ to compile code.
extern "C" void __cxa_atexit(void (*f)(void *), void *p, void *d)
{
}

/// Called by G++ if a pure virtual function is called. Bad Thing, should never happen!
extern "C" void __cxa_pure_virtual()
{
  panic("Pure virtual function call made");
}

/// Called by G++ if function local statics are initialised for the first time
extern "C" int __cxa_guard_acquire()
{
  spinlock.acquire();
  return 1;
}

/// Called by G++ if function local statics are initialised for the first time
extern "C" void __cxa_guard_abort()
{

}

extern "C" void __cxa_guard_release()
{
  spinlock.release();
}

extern "C" void _Unwind_Resume (struct _Unwind_Exception *exception_object) {}