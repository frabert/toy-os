#pragma once
#include <stdint.h>
#include <stddef.h>

extern "C" {
  void *memset(void *dst, uint8_t val, size_t n);
  void *memcpy(void *dst, const void *src, size_t n);
  char *strcpy(char *dst, const char *src);
  int strcmp(const char *str1, const char *str2);
  size_t strlen(const char *str);
}