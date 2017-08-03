#ifndef __FRAMEBUFFER_H
#define __FRAMEBUFFER_H

#include <stdint.h>
#include "vbe.h"

void framebuffer_init(vbe_mode_info_t * modeinfo);
uint8_t framebuffer_enabled();

void framebuffer_write_char(const char c);

uint32_t framebuffer_start_address();
uint32_t framebuffer_end_address();

#endif
