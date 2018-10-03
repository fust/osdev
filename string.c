#include "string.h"
#include "mem/kmalloc.h"
#include "debug.h"

void *memset(void *str, char c, size_t n) {
	__asm__ __volatile__("cld; rep stosb"
			: "=c"((int) {0})
			: "D"(str), "a"(c), "c"(n)
			: "flags", "memory");
	return str;
}

void *memcpy(void *dst, const void *src, size_t len) {
	size_t i;
	char *d = dst;
	const char *s = src;
	for (i = 0; i < len; i++) {
		d[i] = s[i];
	}
	return dst;
}

int strcmp(const char * str1, const char *str2) {
	while (*str1 && (*str1 == *str2))
		str1++, str2++;
	return *(const unsigned char*) str1 - *(const unsigned char*) str2;
}

char *strcpy(char *destination, const char *source) {
	while (*source) {
		*destination = *source;
		destination++, source++;
	}

	return destination;
}

int strlen(const char *str) {
	register const char *s;

	for (s = str; *s; s++)
		;
	return (s - str);
}

char *strdup(const char *str) {
	size_t l = strlen(str);
	return memcpy(kmalloc(l + 1), str, l + 1);
}

size_t strspn(const char *str, const char *accept) {
	const char *ptr = str;
	const char *acc;

	while (*str) {
		for (acc = accept; *acc; ++acc) {
			if (*str == *acc) {
				break;
			}
		}
		if (*acc == '\0') {
			break;
		}

		str++;
	}

	return str - ptr;
}

char *strchr(char *str, int character) {

	do {
		if (*str == character) {
			return (char *) str;
		}
	} while (*str++);

	return 0;
}

char * strpbrk(const char *s, const char *accept) {
	while (*s != '\0') {
		const char *a = accept;
		while (*a != '\0')
			if (*a++ == *s)
				return (char *) s;
		++s;
	}

	return NULL;
}

int strcspn(char *string, char *chars) {
	register char c, *p, *s;

	for (s = string, c = *s; c != 0; s++, c = *s) {
		for (p = chars; *p != 0; p++) {
			if (c == *p) {
				return s - string;
			}
		}
	}
	return s - string;
}

char * strtok(char * s, const char * delim) {
	static char *lasts;
	register int ch;

	if (s == 0)
		s = lasts;
	do {
		if ((ch = *s++) == '\0')
			return 0;
	} while (strchr(delim, ch));
	--s;
	lasts = s + strcspn(s, delim);
	if (*lasts != 0)
		*lasts++ = 0;
	return s;
}

