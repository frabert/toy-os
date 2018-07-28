#include <string.h>
#include <stdint.h>
#include <stdlib.h>

void *memset(void *dst, int val, size_t n) {
  uint8_t *addr = static_cast<uint8_t*>(dst);
  for(size_t i = 0; i < n; i++) addr[i] = val;
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  const uint8_t *srcaddr = static_cast<const uint8_t*>(src);
  uint8_t *dstaddr = static_cast<uint8_t*>(dst);
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

int memcmp ( const void * ptr1, const void * ptr2, size_t num ) {
  const uint8_t *p1 = static_cast<const uint8_t*>(ptr1);
  const uint8_t *p2 = static_cast<const uint8_t*>(ptr2);
  for(size_t i = 0; i < num; i++) {
    if(*p1 < *p2) return -1;
    if(*p1 > *p2) return 1;
    p1++;
    p2++;
  }
  return 0;
}

char * strdup(const char *str1) {
  size_t len = strlen(str1);
  char *new_str = static_cast<char*>(malloc(len + 1));
  strcpy(new_str, str1);
  return new_str;
}

char * strchr (const char * str, int character ) {
  if(character == 0) {
    return (char*)(str + strlen(str));
  } else {
    while(*str) {
      if(*str == character) {
        return (char*)str;
      }
      str++;
    }
  }
  return nullptr;
}