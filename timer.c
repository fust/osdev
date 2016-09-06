#include "timer.h"
#include "io.h"
#include "idt.h"

uint32_t ticks = 0;

static void timer_callback(registers_t regs)
{
	ticks++;
}

void init_timer(uint32_t frequency)
{
	register_interrupt_handler(IRQ0, &timer_callback);

	uint32_t divisor = 1193180 / frequency;

	outb(0x43, 0x36);

	uint8_t low = (uint8_t)(divisor & 0xFF);
	uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

	outb(0x40, low);
	outb(0x40, high);
}

uint32_t get_timer_ticks()
{
	return ticks;
}
