#include "string.h"

void *memset(void *str, char c, size_t n)
{
	uint8_t *s = (uint8_t *)str;
	for( ; n != 0; n--, s[n] = c);
	return str;
}

void *memcpy(void *dst, const void *src, size_t len)
{
	size_t i;
	char *d = dst;
	const char *s = src;
	for (i=0; i<len; i++) {
		d[i] = s[i];
	}
	return dst;
}
