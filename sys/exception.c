#include "sys/exception.h"
#include "debug.h"
#include "cpu.h"

void register_df_handler();
void register_gpf_handler();

void register_exception_handlers()
{
	register_df_handler();
	register_gpf_handler();
}

void df_handler(registers_t * regs)
{
	for(;;) {
		__asm__ __volatile__("cli");
		__asm__ __volatile__ ("hlt");
	}
}

void register_df_handler()
{
	register_interrupt_handler(8, &df_handler);
}

void gpf_handler(registers_t * regs)
{
	uint32_t faulting_address;
	__asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));

	debug("General protection fault! 0x%x", faulting_address);
	kprintf("General protection fault! 0x%x", faulting_address);

	for (;;) {
		__asm__ __volatile__("cli");
		__asm__ __volatile__ ("hlt");
	}
}

void register_gpf_handler()
{
	register_interrupt_handler(13, &gpf_handler);
}
