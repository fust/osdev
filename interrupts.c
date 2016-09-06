#include "interrupts.h"
#include "video.h"
#include <debug.h>

void default_interrupt_handler(registers_t *regs)
{
	write_string("Fatal exception, system halted.");

	do_backtrace(regs);

	__asm__ __volatile__("hlt");
}
