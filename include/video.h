#include "stdint.h"
#ifndef __VIDEO_H
#define __VIDEO_H

extern unsigned short *VIDEO_MEM;

extern void puts(const char c);

extern void cls();

extern void write_at(const char *string, uint8_t x, uint8_t y);

#endif
