#include "stdint.h"
#ifndef __VIDEO_H
#define __VIDEO_H

extern unsigned short *VIDEO_MEM;

void puts(const char c);

void cls();

void write_at(char *string, uint8_t x, uint8_t y);

void update_cursor(int row, int col);

#endif
