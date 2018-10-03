#include "multiboot.h"
#include "video.h"
#include "gdt.h"
#include "idt.h"
#include "sys/exception.h"
#include "timer.h"
#include "debug.h"
#include "mem/pmm.h"
#include "mem/paging.h"
#include "interrupts.h"
#include <stddef.h>
#include "stdio.h"
#include <stdlib.h>
#include "vbe.h"
#include "framebuffer.h"
#include "fs/vfs.h"
#include "fs/initrd.h"
#include "pci/pci.h"
#include "fs/ata.h"

extern uintptr_t end; // Defined in linker script
uintptr_t kernel_end = 0;
uint32_t initial_esp;
multiboot_elf_section_header_table_t copied_elf_header;

void kmain(struct multiboot *mboot_ptr, unsigned int initial_stack)
{
	// Mark where we end
	kernel_end = (uintptr_t) &end;

	uint32_t initrd_loc, initrd_end = 0;

	if (mboot_ptr->mods_count > 0) {
		debug("Got %d modules to load, assuming first is our INITRD\n", mboot_ptr->mods_count);
		initrd_loc = *((uint32_t *) mboot_ptr->mods_addr);
		initrd_end = *(uint32_t *)(mboot_ptr->mods_addr + sizeof(uint32_t));
		debug("INITRD is at 0x%x - 0x%x\n", initrd_loc, initrd_end);
		kernel_end = initrd_end;
	}

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

	debug("Got our basic info:\n");
	debug("\tKernel end: 0x%x\n", kernel_end);
	debug("\tInitial ESP: 0x%x\n", initial_esp);
	debug("\tMemory size: %d MB\n", (mem_max / 1024) / 1024);

	if (mboot_ptr->flags | 0x400) {
		debug("\tvbe_control_info: %x\n", mboot_ptr->vbe_control_info);
		debug("\tvbe_mode_info: %x\n", mboot_ptr->vbe_mode_info);
		debug("\tvbe_mode: %x\n", mboot_ptr->vbe_mode);

		vbe_mode_info_t *modeinfo = (vbe_mode_info_t *) mboot_ptr->vbe_mode_info;
		framebuffer_init(modeinfo);
	} else {
		cls();
	}

	if (mboot_ptr->flags | 0x40) {
		debug("\tdrives_length: %d\n", mboot_ptr->drives_length);
		debug("\tdrives_addr: %x\n", mboot_ptr->drives_addr);
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

	register_exception_handlers();

	debug("Enabling interrupts\n");
	__asm__ __volatile__ ("sti");
	debug("Interrupts enabled\n");

	kprintf("Interrupts enabled...\n");

	init_timer(1000);
	debug("Timer enabled.\n");

	uint32_t memsize = 0x0;

	if (mboot_ptr->flags & MULTIBOOT_FLAG_MMAP) {
		debug("Found a memory map (Thanks, %s!): 0x%x with length %d (%d items)\n", mboot_ptr->boot_loader_name, mboot_ptr->mmap_addr, mboot_ptr->mmap_length, mboot_ptr->mmap_length / sizeof(mboot_memmap_t));
		kprintf("Found a memory map (Thanks, %s!): 0x%x with length %d (%d items)\n", mboot_ptr->boot_loader_name, mboot_ptr->mmap_addr, mboot_ptr->mmap_length, mboot_ptr->mmap_length / sizeof(mboot_memmap_t));
		mboot_memmap_t* mmap = (mboot_memmap_t*)mboot_ptr->mmap_addr;
		uint8_t idx = 0;
		while(mmap < mboot_ptr->mmap_addr + mboot_ptr->mmap_length) {

			debug("\tChunk: 0x%d, Start: 0x%x\tEnd: 0x%x\tLength: 0x%x\tType: %x.\n", idx, mmap->base_addr_low, (mmap->base_addr_low + mmap->length_low), mmap->length_low, mmap->type);
			kprintf("\tChunk: 0x%d, Start: 0x%x\tEnd: 0x%x\tLength: 0x%x\tType: %x.\n", idx, mmap->base_addr_low, (mmap->base_addr_low + mmap->length_low), mmap->length_low, mmap->type);

			if (mmap->type == 1) {
				memsize += mmap->length_low;
			}

			mmap = (mboot_memmap_t*) ( (uint32_t)mmap + mmap->size + sizeof(uint32_t) );
			idx++;
		}
	}

	kprintf("Initializing PMM with %d KiB of memory...", memsize / 1024);
	pmm_init(memsize);
	if (mboot_ptr->flags & MULTIBOOT_FLAG_MMAP) {
		mboot_memmap_t* mmap = (mboot_memmap_t*)mboot_ptr->mmap_addr;
		uint8_t idx = 0;
		while(mmap < mboot_ptr->mmap_addr + mboot_ptr->mmap_length) {
			if (mmap->type != 1) {
				pmm_mark_system(mmap->base_addr_low, mmap->length_low);
			}

			mmap = (mboot_memmap_t*) ( (uint32_t)mmap + mmap->size + sizeof(uint32_t) );
			idx++;
		}
	}
	kprintf(" [ OK ]\n");

	if (mboot_ptr->flags & MULTIBOOT_FLAG_MMAP) {
		mboot_memmap_t* mmap = (mboot_memmap_t*)mboot_ptr->mmap_addr;
		while(mmap < mboot_ptr->mmap_addr + mboot_ptr->mmap_length) {
			if (mmap->type != 1) {
				pmm_mark_system((uintptr_t *)mmap->base_addr_low, mmap->length_low);
			}

			mmap = (mboot_memmap_t*) ( (uint32_t)mmap + mmap->size + sizeof(uint32_t) );
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

	debug("Initializing paging structures.\n");

	paging_init();

	debug("Initializing VFS.\n");
	vfs_install();

	initrd_set_location(initrd_loc);

	filesystem_t * initrd = (filesystem_t *) kmalloc(sizeof(filesystem_t));
	initrd->name = strdup("INITRD");
	initrd->mount = &initrd_mount;
	register_filesystem(initrd);

	debug("Mounting InitRD from node at 0x%x.\n", initrd_loc);
	vfs_mount(initrd);
	debug("InitRD mounted.\n");

	char *fname = "/file1.txt";
	debug("Opening file %s\n", fname);
	vfs_inode_t * root = file_open(fname, 0);
	if (root) {
		debug("Reading %s\n", fname);
		vfs_tnode_t * node;
		if (root->flags & VFS_TYPE_DIRECTORY) {
			node = vfs_readdir(root);
			for (uint32_t i = 0; i < root->num_subfiles; i++) {
				debug("\t- %d : %s\n", root->subfiles[i].inode->inode_num, root->subfiles[i].name);
			}
			kprintf("Mounted root filesystem: %s. Inode #%d\n", node->name, node->inode->inode_num);
			debug("InitRD root dir reports: name: %s; inode: %d\n", node->name, node->inode->inode_num);
		} else if (root->flags & VFS_TYPE_FILE) {
			debug("Allocating buffer of 32 chars.\n");
			char *buff = kmalloc(sizeof (char) * 32);
			debug("Start reading %d chars of data.\n", 32);
			uint32_t read_len = vfs_read(root, 32, 0, buff);
			debug("Read %d bytes. Contents: '%s'\n", read_len, buff);
		}
	} else {
		debug("Could not open %s\n", fname);
	}

	kprintf("System up and running.\nUsing %d of %d KB of memory\n", pmm_used_frames() * 4, pmm_num_frames() * 4);
	debug("System up and running.\nUsing %d of %d KB of memory\n", pmm_used_frames() * 4, pmm_num_frames() * 4);

	/*
	debug("Enumerating PCI devices.\n");
	pci_enumerate_busses();*/
	ata_init();

	kprintf("\nHALT\n");
	debug("Halting system.\n");

	while (1) {
		__asm__ __volatile__("hlt");
	}
}
