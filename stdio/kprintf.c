#include "stdio.h"
#include <stdarg.h>
#include "video.h"
#include <stdlib.h>

void kprintf(char * format, ...)
{
	va_list argptr;
	char s[34], c, *str;
	int num;

	va_start(argptr, format);

	while (*format != '\0') {
		if (*format != '%') {
			puts(*format);
		} else {
			*format++;
			switch(*format) {
				case '%':
					puts('%');
					break;
				case 'b':
					num = va_arg(argptr, int);
					itoa(num, s, 2);
					kprintf(s);
					break;
				case 'c':
					c = va_arg(argptr, int); // Yes, it *should* be char but them the compiler complains.
					puts(c);
					break;
				case 's':
					str = va_arg(argptr, char *);
					kprintf(str);
					break;
				case 'x':
					num = va_arg(argptr, int);
					itoa(num, s, 16);
					kprintf(s);
					break;
				case 'd':
				case 'i':
					num = va_arg(argptr, int);
					itoa(num, s, 10);
					kprintf(s);
					break;
				default:
					puts(--*format);
					puts(++*format);
					break;
			}
		}
		*format++;
	}

	va_end(argptr);
}
