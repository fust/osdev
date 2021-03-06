#ifndef __IO_H
#define __IO_H

#include <stdint.h>

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

uint16_t ins(uint16_t _port);

#define insl(port, buffer, count) asm volatile("cld; rep; insl" :: "D" (buffer), "d" (port), "c" (count))

#endif
