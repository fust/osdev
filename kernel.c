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
#include "mem/kmalloc.h"
#include "mem/paging.h"
#include "mem/liballoc/liballoc.h"
#include "console/console.h"
#include "fs/ramdisk.h"
#include "dev/ps2.h"
#include "dev/kbd.h"
#include "sys/pipe.h"
#include "dev/pci.h"
#include "dev/ata.h"

#if 1
extern pipe_t *kbd_pipe;
#endif

extern uintptr_t end; // Defined in linker script
uintptr_t kernel_end = 0;
uint32_t initial_esp;
multiboot_elf_section_header_table_t copied_elf_header;
vfs_dir_t *fs_root;

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
	debug("Stack is at 0x%x.\n", initial_esp);

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

	kprintf("Initializing paging...");
	paging_init();
	kprintf(" [ OK ]\n");

#if 0
	// This will be used for testing once the kernel heap allocator is done.
	uint32_t phys1;
	uint32_t phys2;
	uint32_t *alloc1 = (uint32_t *)kmalloc_p(sizeof(uint32_t), (uintptr_t *)(&phys1));
	uint32_t *alloc2 = (uint32_t *)kmalloc_p(sizeof(uint32_t), (uintptr_t *)(&phys2));
	debug("Allocated...\n");

	*alloc1 = 0x1000;
	*alloc2 = 0x1000;
	debug("Allocate block at 0x%x and 0x%x. Values: (%x, %x), Phys (0x%x, 0x%x)\n", alloc1, alloc2, *alloc1, *alloc2, phys1, phys2);
#endif

	kprintf("Initializing VFS");
	vfs_install();
	kprintf(" [ OK ]\n");

	kprintf("Initializing RAMFS");
	fs_root = ramdisk_init();
	kprintf(" [ OK ]\n");

	kprintf("Initializing (P/S)ATA devices");
	ata_init();
	kprintf(" [ OK ]\n");

#if 0
	vfs_dirent_t *dev = vfs_read_dir(fs_root, 2);
	if (dev) {
		debug("Inode 2: %s%s\n", fs_root->fname, dev->fname);
		uint8_t *buff = (uint8_t *)kmalloc(sizeof (uint8_t));
		char *b2 = "TESTING123";
		vfs_write(&dev->node, 0, sizeof(char) * 11, b2);
		vfs_read(&dev->node, 0, 32, buff);
		debug("\tContents: %s\n", buff);
	} else {
		debug("Could not find /dev!\n");
	}
#endif

	kprintf("Init timer");
	init_timer(50);
	kprintf(" [ OK ]\n");

	kprintf("Init PS/2");
	ps2_init();
	kprintf(" [ OK ]\n");

	kprintf("Init keyboard");
	ps2_dev_t kbd;
	kbd.handler = &keyboard_interrupt;
	ps2_register_device_driver(kbd, 0, &keyboard_init);
	kprintf(" [ OK ]\n");

	debug_print_vfs_tree();

	console_run();

	//kprintf("Reached end of control, system stopped.\n");

	while (1) {
		__asm__ __volatile__("hlt");
	}

	kprintf("\nHALT\n");
}
