#include "io.h"

void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port)
{
   uint8_t ret;
   asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

void outl(uint16_t port, uint32_t value)
{
    asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

uint32_t inl(uint16_t port)
{
   uint32_t ret;
   asm volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

uint16_t ins(uint16_t _port) {
	uint16_t rv;
	asm volatile ("inw %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}
