#include "dev/kbd.h"
#include "dev/ps2.h"
#include "cpu.h"
#include "stdint.h"
#include "fs/vfs.h"
#include "debug.h"
#include "sys/pipe.h"
#include "mem/kmalloc.h"
#include "kbd_keycodes.h"

ps2_dev_t dev_int;

vfs_node_t *kbd_pipe;

uint8_t *keystates;

void kbd_write(ps2_byte_t data);
ps2_byte_t kbd_read();
void kbd_create_event(uint8_t scancode, kbd_event_t *event);
uint32_t convert_scan_to_key(uint8_t scancode);

uint8_t kbdcodes[128] = {
		KEY_RESERVED, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE,
		KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE,
		KEY_ENTER,
		KEY_LEFTCTRL,
		KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE,
		KEY_GRAVE,
		KEY_LEFTSHIFT,
		KEY_BACKSLASH,
		KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT,
		KEY_KPASTERISK,
		KEY_LEFTALT, KEY_SPACE,
		KEY_CAPSLOCK,
		KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
		KEY_NUMLOCK,
		KEY_SCROLLLOCK,
		KEY_KP7, KEY_KP8, KEY_KP9,
		KEY_KPMINUS,
		KEY_KP4, KEY_KP5, KEY_KP6, KEY_KPPLUS,
		KEY_KP1, KEY_KP2, KEY_KP3,
		KEY_KP0, KEY_KPDOT,
		0,
		0,
		0,
		KEY_F11, KEY_F12,
		0 // All other undefined.
};

void keyboard_init(ps2_dev_t device)
{
	debug("KBD: Initializing...\n");

	keystates = (uint8_t *)kmalloc(sizeof(uint8_t) * 128);

	if (device.read == 0 || device.write == 0) {
		debug("KBD: Invalid or incomplete device specification. Aborting.\n");
		return;
	}

	dev_int = device;

	kbd_pipe = pipe_device_create(sizeof(kbd_event_t) * 8); // 8 events
	vfs_mount("/dev/kbd", kbd_pipe);

	int tries = 0;

	while (tries < 3) {
		kbd_write(0xF4);
		debug("KBD: Wrote 0xF4 command.\n");
		uint8_t res = kbd_read();
		if (res != 0xFA) {
			debug("KBD: Failed to enable scanning.\n");
			tries++;
		} else {
			debug("KBD: Enabled scanning.\n");
			kbd_write((ps2_byte_t)0xED);
			kbd_read();
			kbd_write(0);
			kbd_read();
			break;
		}
	}
}

void keyboard_interrupt(registers_t regs)
{
	ps2_byte_t code = kbd_read();

	if (code > 128) { // Skip undefined keys
		return;
	}

	kbd_event_t event = {0, 0, 0};

	kbd_create_event(code, &event);

	uint8_t ledbyte = 0;

	if (keystates[KEY_SCROLLLOCK]) {
		ledbyte |= 1;
	}
	if (keystates[KEY_NUMLOCK]) {
		ledbyte |= 1 << 1;
	}
	if (keystates[KEY_CAPSLOCK]) {
		ledbyte |= 1 << 2;
	}

	kbd_write((ps2_byte_t)0xED);
	kbd_read();
	kbd_write(ledbyte);
	kbd_read();

	if (kbd_pipe) {
		if (vfs_write(kbd_pipe, 0, sizeof(kbd_event_t), (uint8_t *)&event) != 0) {
			debug("KBD: Error pushing to pipe (buffer full?)\n");
		}
	}
}

void kbd_write(ps2_byte_t data)
{
	if (dev_int.write == 0) {
		return;
	}

	dev_int.write(PS2_DATA_PORT, data);
}

ps2_byte_t kbd_read()
{
	if (dev_int.read == 0) {
		return -1;
	}

	return dev_int.read(PS2_DATA_PORT);
}

uint32_t convert_scan_to_key(uint8_t scancode)
{
	return (uint32_t) kbdcodes[scancode]; // For now just use the US keyboard. Keymaps will come at a later stage.
}

void kbd_create_event(uint8_t scancode, kbd_event_t *event)
{
	event->scancode = scancode;
	event->keycode = convert_scan_to_key(scancode);
	event->flags = 0x0;

	if (event->keycode == KEY_NUMLOCK || event->keycode == KEY_SCROLLLOCK || event->keycode == KEY_CAPSLOCK) {
		if (scancode & 0x80) {
			event->flags |= KBD_FLAG_RELEASED
		} else {
			event->flags |= KBD_FLAG_PRESSED
			keystates[event->keycode] = !keystates[event->keycode];
		}
	} else {
		if (keystates[event->keycode]) {
			event->flags |= KBD_FLAG_REPEATED;
		} else if (scancode & 0x80) {
			event->flags |= KBD_FLAG_RELEASED;
			keystates[event->keycode] = 0;
		} else {
			keystates[event->keycode] = 1;
			event->flags |= KBD_FLAG_PRESSED;
		}
	}
}
