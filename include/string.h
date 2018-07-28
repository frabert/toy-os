#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void * memmove ( void * destination, const void * source, size_t num );
void * memcpy ( void * destination, const void * source, size_t num );
int memcmp ( const void * ptr1, const void * ptr2, size_t num );
void * memset ( void * ptr, int value, size_t num );
size_t strlen(const char* c);
char * strdup(const char *str1);
char * strchr (const char * str, int character );
char *strcpy(char *dst, const char *src);

#ifdef __cplusplus
}
#endif