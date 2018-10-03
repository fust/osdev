#include "io.h"

void outb(uint16_t port, uint8_t value) {
	__asm__ __volatile__ ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ __volatile__("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

void outs(uint16_t port, uint16_t value) {
	__asm__ __volatile__ ("outw %1, %0" : : "dN" (port), "a" (value));
}

uint16_t ins(uint16_t port) {
	uint16_t rv;
	__asm__ __volatile__ ("inw %1, %0" : "=a" (rv) : "dN" (port));
	return rv;
}

void outl(uint16_t port, uint32_t value) {
	__asm__ __volatile__ ("outl %1, %0" : : "dN" (port), "a" (value));
}

uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ __volatile__("inl %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}
