#ifndef __STRING_H
#define __STRING_H
#include "stddef.h"

extern void *memset(void *str, char c, size_t n);

extern void *memcpy(void *dst, const void *src, size_t len);

#endif
