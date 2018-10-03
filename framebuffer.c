#include "framebuffer.h"
#include <stdint.h>
#include "debug.h"
#include "color.h"
#include "font.h"

uint8_t *framebuffer;
uint16_t pitch;
uint8_t bpp;
uint16_t width, height;
uint8_t enabled = 0;

uint32_t screen_x = 0;
uint32_t screen_y = 0;

void put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	uint8_t r = (color >> 16) & 255;
	uint8_t g = (color >> 8) & 255;
	uint8_t b = color & 255;

	unsigned where = x * (bpp / 8) + y * pitch;
	framebuffer[where + 0] = b;
	framebuffer[where + 1] = g;
	framebuffer[where + 2] = r;

}

void draw_char(uint32_t x, uint32_t y, uint32_t character, uint8_t foreground, uint8_t background) {
	uint32_t j = x;
	uint32_t h = y;
	uint8_t * font_char = &number_font[character];
	for (uint32_t l = 0; l <= 12; l++) {
		for (uint32_t i = 8; i > 0; i--) {
			j++;
			if ((font_char[l] & (1 << i))) {
				put_pixel(j, h, 0xffffff);
			}
		}
		h++;
		j = x;
	}
}

void framebuffer_write_char(const char c)
{
	if (c == '\n') {
		screen_x = 0;
		screen_y += 15;
		return;
	}

	if (c == '\t') {
		screen_x += 9;
		return;
	}

	if (c == '\b') {
		screen_x -= 9;
		draw_char(screen_x, screen_y, ' ', 0, 0);
	}

	draw_char(screen_x, screen_y, c, 0, 0);

	screen_x += 10;

	if (screen_x > width - 10) {
		screen_y += 15;
		screen_x = 0;
	}

	if (screen_y > height - 15) {
		screen_y = 0;
		screen_x = 0;
		for (uint32_t i = 0; i < width; i++) {
			for (uint32_t j = 0; j < 10; j++) {
				put_pixel(i, j, 0x000000);
			}
		}
	}
}

void fillrect(uint8_t r, uint8_t g, uint8_t b, uint16_t w, uint16_t h) {
	for (uint32_t i = 0; i < h; i++) {
		for (uint32_t j = 0; j < w; j++) {
			put_pixel(j, i, (r << 16) + (g << 8) + b);
		}
	}
}

void framebuffer_init(vbe_mode_info_t * modeinfo)
{
	debug("\t=== Initializing framebuffer ===\n");
	debug("\t\tResolution: %d x %d @ %d bpp (pitch: %d)\n", modeinfo->width, modeinfo->height, modeinfo->bpp, modeinfo->pitch);
	debug("\t\tFramebuffer is at 0x%x physical\n", modeinfo->framebuffer);

	framebuffer = (uint8_t *) modeinfo->framebuffer;
	pitch = modeinfo->pitch;
	bpp = modeinfo->bpp;
	width = modeinfo->width;
	height = modeinfo->height;

	enabled = 1;
}

uint8_t framebuffer_enabled()
{
	return enabled;
}

uint32_t framebuffer_start_address()
{
	return (uint32_t) framebuffer;
}

uint32_t framebuffer_end_address()
{
	return (uint32_t) framebuffer + (height * pitch);
}
