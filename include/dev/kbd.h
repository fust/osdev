#ifndef __KBD_H
#define __KBD_H

#include "stdint.h"
#include "dev/ps2.h"
#include "cpu.h"

#define KBD_CMD_LEDS 0xED;

#define KBD_FLAG_CTRL 0x1;
#define KBD_FLAG_ALT 0x2;
#define KBD_FLAG_SHIFT 0x4;
#define KBD_FLAG_SUPER 0x8;

#define KBD_FLAG_NUMLED 0x10;
#define KBD_FLAG_CAPSLED 0x20;
#define KBD_FLAG_SCROLLED 0x40;

#define KBD_FLAG_PRESSED 0x80;
#define KBD_FLAG_RELEASED 0x100;
#define KBD_FLAG_REPEATED 0x200;

typedef struct kbd_event {
	uint8_t scancode;
	uint32_t keycode;
	uint16_t flags; // See KBD_FLAG_* masks
} kbd_event_t;

void keyboard_init(ps2_dev_t device);
void keyboard_interrupt(registers_t regs);

#endif
