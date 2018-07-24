#include "utils.h"

void *memset(void *dst, uint8_t val, size_t n) {
  uint8_t *addr = (uint8_t*)dst;
  for(size_t i = 0; i < n; i++) addr[i] = val;
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  const uint8_t *srcaddr = (const uint8_t*)src;
  uint8_t *dstaddr = (uint8_t*)dst;
  for(size_t i = 0; i < n; i++) dstaddr[i] = srcaddr[i];
  return dst;
}

char *strcpy(char *dst, const char *src) {
  size_t i = 0;
  while(src[i]) {
    dst[i] = src[i];
    i++;
  }

  return dst;
}

int strcmp(const char *str1, const char *str2) {
  size_t i = 0;
  while(str1[i] && str2[i]) {
    if(str1[i] < str2[i]) return -1;
    else if(str1[i] > str2[i]) return 1;
  }
  return 0;
}

size_t strlen(const char *str) {
  size_t i = 0;
  while(str[i++]) {;}
  return i;
}