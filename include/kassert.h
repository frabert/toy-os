#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void panic_raw(const char *error);

#ifdef __cplusplus
}
#endif

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define panic(x) panic_raw("Kernel panic at " __FILE__ ":" S__LINE__ " - " x "\n")

#define assert(x) if(!(x)) panic("Assertion failed: " #x)