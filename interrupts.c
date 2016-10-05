#include "interrupts.h"
#include "video.h"
#include <debug.h>

void default_interrupt_handler(registers_t *regs)
{
	PANIC("Fatal exception, system halted. EIP=0x%x", regs->eip);
	__asm__ __volatile__("hlt");
}
