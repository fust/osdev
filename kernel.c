#include "multiboot.h"
#include "video.h"
#include "gdt.h"
#include "idt.h"
#include "mem/pmm.h"
#include "interrupts.h"
#include "timer.h"
#include "stddef.h"
#include "elf.h"
#include "debug.h"
#include "stdio.h"
#include "stdlib.h"

extern uintptr_t end; // Defined in linker script
uintptr_t kernel_end = 0;
uint32_t initial_esp;
multiboot_elf_section_header_table_t copied_elf_header;

void kmain(struct multiboot *mboot_ptr, unsigned int initial_stack)
{
	// Mark where we end
	kernel_end = (uintptr_t) &end;

	pmm_set_kernel_end(kernel_end); // Now kmalloc() works!

	// Get initial stack location
	initial_esp = initial_stack;

	// Get available memory
	uint32_t mem_max = 0;
	// First check if the multiboot struct provided it
	if (mboot_ptr->flags | 0x20) {
		mem_max = (uint32_t)((mboot_ptr->mem_lower + mboot_ptr->mem_upper) * 1024);
	}
	// Mem_max is now either 0 or the memory size in bytes

	cls();

	if (mboot_ptr->flags & MULTIBOOT_FLAG_MMAP) {
		debug("Found a memory map (Thanks, %s!): 0x%x with length %d (%d items)\n", mboot_ptr->boot_loader_name, mboot_ptr->mmap_addr, mboot_ptr->mmap_length, mboot_ptr->mmap_length / sizeof(mboot_memmap_t));

		mboot_memmap_t* mmap = mboot_ptr->mmap_addr;
		while(mmap < mboot_ptr->mmap_addr + mboot_ptr->mmap_length) {
			mmap = (mboot_memmap_t*) ( (unsigned int)mmap + mmap->size + sizeof(mmap->size) );
			debug("\tFound memory region: 0x%x length: 0x%x type: %x.\n", mmap->base_addr, mmap->length, mmap->type);
		}
	}

	if (mboot_ptr->flags & MULTIBOOT_INFO_ELF_SHDR) {
		if (mboot_ptr->u.elf_sec.size == sizeof(Elf32_Shdr)) {
			copied_elf_header = mboot_ptr->u.elf_sec;
		} else {
			kprintf("ELF header size mismatch! Will not load symbol table.\n");
		}
	}

	if (&copied_elf_header) {
		debug_init(&copied_elf_header);
	}

	kprintf("OS loading...\n");
	debug("Stack is at %x.\n", initial_esp);

	kprintf("Loading GDT...");
	debug("Loading GDT\n");

	init_gdt();

	kprintf("[ OK ]\n");
	kprintf("Loading IDT...");

	debug("Loading IDT\n");
	init_idt();

	kprintf("[ OK ]\n");

	debug("Enabling interrupts\n");
	__asm__ __volatile__ ("sti");
	debug("Interrupts enabled\n");

	kprintf("Interrupts enabled...\n");
	kprintf("Initializing PMM with %d MB of memory...", (mem_max / 1024) / 1024);
	pmm_init(mem_max);
	kprintf(" [ OK ]\n");

	pmm_alloc();

	debug("Init timer\n");
	init_timer(50);
	debug("Timer enabled\n");
	char *buf = "";

	while (1) {
		itoa(get_timer_ticks(), buf, 10);
		write_at(buf, 1, 23);
		__asm__ __volatile__("hlt");
	}

	kprintf("\nHALT\n");
}
