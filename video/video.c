#include "video.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "io.h"

unsigned short *VIDEO_MEM = (unsigned short *)0xB8000;

uint8_t cursor_x, cursor_y = 0;

void puts(const char c )
{
	// Create attribute byte
	uint8_t attribute = (0 << 4) | (15 & 0x0F);

	if (c == '\n') {
		cursor_x = 0;
		cursor_y++;
		return;
	}

	if (c == '\t') {
		cursor_x += 4;
		return;
	}

	// Calculate the offset
	if (cursor_x > 80) {
		cursor_y++;
		cursor_x = 0;
	}

	if (cursor_y > 25) {
		// Scrolling
		for (size_t i = 1; i <= 25; i++) {
			uint16_t *loc = VIDEO_MEM + (i * 80);
			uint16_t *dst = VIDEO_MEM + ((i - 1) * 80);
			memcpy(loc, dst, sizeof(char) * 80);
		}
		cursor_y = 0;
	}

	// Calculate the position
	uint16_t *location = VIDEO_MEM + (cursor_y * 80 + cursor_x);

	// Write the actual character
	memset(location, (c | (attribute << 8)), sizeof(char));
	//*location = c | (attribute << 8);

	// Update cursor location
	cursor_x++;

	update_cursor(cursor_y, cursor_x);
}

void cls()
{
	// Create attribute byty
	uint8_t attribute = (0 << 4) | (15 & 0x0F);
	uint16_t blank = 0x20 | (attribute << 8);

	uint32_t i;
	for (i = 0; i < 80 * 25; i++) {
		VIDEO_MEM[i] = blank;
	}

	cursor_x = 0;
	cursor_y = 0;
}

void write_at(char *string, uint8_t x, uint8_t y)
{
	if (x > 80 || y > 25) {
		return;
	}

	uint8_t orig_x, orig_y;
	orig_x = cursor_x;
	orig_y = cursor_y;

	cursor_x = x;
	cursor_y = y;

	kprintf(string);

	cursor_x = orig_x;
	cursor_y = orig_y;
}

/* void update_cursor(int row, int col)
 * by Dark Fiber
 */
void update_cursor(int row, int col)
{
	unsigned short position = (row * 80) + col;

    // cursor LOW port to vga INDEX register
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    // cursor HIGH port to vga INDEX register
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char )((position >> 8) & 0xFF));
}
