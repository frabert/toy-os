#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t s);
void *calloc(size_t n, size_t s);
void *realloc(void *ptr, size_t s);
void free(void* ptr);

#ifdef __cplusplus
}
#endif