#include "multiboot.h"
#include "video.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "timer.h"
#include "stddef.h"
#include "elf.h"
#include "debug.h"
#include "pmm.h"
#include "vmm.h"
#include "stdio.h"

extern uint32_t *end; // Defined in linker script
uint32_t kernel_end=0;
uint32_t initial_esp;
multiboot_elf_section_header_table_t copied_elf_header;

void kmain(struct multiboot *mboot_ptr, unsigned int initial_stack)
{
	// Mark where we end
	kernel_end = end;

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

	debug("Init PMM\n");
	pmm_init(mem_max);

	kprintf("[ OK ]\n");

	kprintf("Initializing VMM, paging and heap...");
	debug("Init VMM\n");
	vmm_init_paging();
	kprintf("[ OK ]\n");

	if (mboot_ptr->flags & (1 << 6)) {
		debug("Parsing memory map\n");
		kprintf("Parsing memory map...");
		mboot_memmap_t * mmap = (void *)mboot_ptr->mmap_addr;

		while((uint32_t) mmap < mboot_ptr->mmap_addr + mboot_ptr->mmap_length) {
			if (mmap->type != 2) { // Unusable memory
				for (unsigned long long int i = 0; i < mmap->length; i += 0x1000) {
					if (mmap->base_addr + i > 0xFFFFFFFF) break;
					kprintf("Marking 0x%x", (uint32_t) mmap->base_addr + i);
					paging_mark_system((mmap->base_addr + i) & 0xFFFFF000);
				}
			}
			mmap = (mboot_memmap_t *) ((uint32_t)mmap + mmap->size + sizeof(uint32_t));
		}

		kprintf("[ OK ]\n");
	}

	kprintf("Activate paging...");
	debug("Activate paging\n");
	vmm_paging_activate();
	debug("Paging active\n");
	kprintf("[ OK ]\n");

	kprintf("Installing heap...");
	debug("Installing heap\n");
	heap_install();
	debug("Heap installed\n");
	kprintf("[ OK ]\n");

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
