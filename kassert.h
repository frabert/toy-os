#pragma once

#include "screen.h"
#include "debug.h"

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

#define KPANIC(msg, ...) { \
  os::Screen::getInstance().write("Kernel panic at " __FILE__ ":" S__LINE__ " - " msg, __VA_ARGS__); \
  debug_break(); \
  while(1) {;} \
}

#define KASSERT(cond) { \
  if(!(cond)) { \
    KPANIC("Assertion failed (" #cond ")", ""); \
  } \
}

#define KASSERT_MSG(cond, msg) { \
  if(!(cond)) { \
    KPANIC(msg, ""); \
  } \
}
