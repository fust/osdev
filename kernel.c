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
#include <task.h>

extern uintptr_t end; // Defined in linker script
extern uintptr_t kernstart; // Defined in linker script
uintptr_t kernel_start=0;
uintptr_t kernel_end=0;
uint32_t initial_esp;
multiboot_elf_section_header_table_t copied_elf_header;

void kmain(struct multiboot *mboot_ptr, unsigned int initial_stack)
{
	// Mark where we start
	kernel_start = (uintptr_t) &kernstart;
	// Mark where we end
	kernel_end = (uintptr_t) &end;

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

	kprintf("OS loading...\n");
	debug("Kernel end is at 0x%x, Stack is at %x.\n", end, initial_esp);

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
	pmm_init(mem_max, kernel_start, kernel_end);
	kprintf("[ OK ]\n");

	if (mboot_ptr->flags & (1 << 6)) {
		debug("Parsing memory map (0x%x - 0x%x)\n", mboot_ptr->mmap_addr, (mboot_ptr->mmap_addr + mboot_ptr->mmap_length));
		kprintf("Parsing memory map... ");
		mboot_memmap_t * mmap = (void *)mboot_ptr->mmap_addr;

		while((uint64_t) mmap < (uint64_t)(mboot_ptr->mmap_addr + mboot_ptr->mmap_length)) {
			if (mmap->type > 1) { // Unusable memory
				for (uint64_t i = 0; i < mmap->length; i += 0x1000) {
					if (mmap->base_addr + i > 0xFFFFFFFF) break;
					kprintf("Marking 0x%x\n", (uint32_t) mmap->base_addr + i);
					paging_mark_system((mmap->base_addr + i) & 0xFFFFF000);
				}
				debug("Found unusable memory: 0x%x - 0x%x\n", mmap->base_addr, mmap->base_addr + mmap->length);
			} else {
				debug("Found usable memory: 0x%x - 0x%x\n", mmap->base_addr, mmap->base_addr + mmap->length);
			}
			mmap = (mboot_memmap_t *) ((uint64_t)mmap + mmap->size + sizeof(uint64_t));
		}

		kprintf("[ OK ]\n");
	}

	kprintf("Initializing VMM, paging and heap...");
	debug("Init VMM\n");
	vmm_init_paging();
	kprintf("[ OK ] (%d Blocks used)\n", pmm_get_used_block_count());

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

	if (&copied_elf_header) {
		debug_init(&copied_elf_header);
	}

	debug("Init timer\n");
	init_timer(50);
	debug("Timer enabled\n");
	char *buf = "";

	//tasking_install();

	while (1) {
		itoa(get_timer_ticks(), buf, 10);
		write_at(buf, 1, 23);
		__asm__ __volatile__("hlt");
	}

	kprintf("\nHALT\n");
}
