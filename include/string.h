#ifndef __STRING_H
#define __STRING_H
#include "stddef.h"

void *memset(void *str, char c, size_t n);

void *memcpy(void *dst, const void *src, size_t len);

int strcmp(const char * str1, const char *str2);

char *strcpy (char *destination, const char *source);

#endif
