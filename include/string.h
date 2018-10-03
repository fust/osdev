#ifndef __STRING_H
#define __STRING_H
#include "stddef.h"

void *memset(void *str, char c, size_t n);

void *memcpy(void *dst, const void *src, size_t len);

int strcmp(const char * str1, const char *str2);

char *strcpy(char *destination, const char *source);

int strlen(const char *str);

char *strdup(const char *str);

size_t strspn(const char * str, const char * accept);

char *strchr(char *str, int character);

char * strpbrk(const char *s, const char *accept);

int strcspn(char *string, char *chars);

char * strtok(char * s, const char * delim);

#endif
