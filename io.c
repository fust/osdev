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

uint16_t inw(uint16_t port)
{
	uint16_t ret;
	asm volatile("inw %%dx, %%ax":"=a"(ret):"d"(port));
	return ret;
}

void inws(uint16_t* buffer, uint8_t count, uint16_t port)
{
  __asm__ __volatile__(
    "rep insw\n"
    : "=c" ( count ), "=D" ( buffer )
    : "d" ( port ), "0" ( count ), "1" ( buffer )
    : "memory"
  );
}
