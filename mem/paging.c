#include "mem/paging.h"
#include "mem/pmm.h"
#include "debug.h"
#include "string.h"
#include "framebuffer.h"
#include "idt.h"
#include "mem/kheap.h"
#include "sys/bitmap.h"

extern uint32_t placement_pointer;
extern uint32_t KERNEL_HEAP_SIZE;
extern uint32_t HEAP_OFFSET;

page_directory_t *current_directory;
page_directory_t *kernel_directory;

uint32_t *page_dir_virt = 0xFFFFF000;
uint32_t *page_tables_virt = 0xFFC00000;

pte_t *get_page(uint32_t address, page_directory_t *directory);
void paging_switch_directory();

void map_page(pte_t *page, int is_kernel, int is_writable)
{
	if (page->frame != 0) {
		page->present = 1;
		page->rw = (is_writable == 1) ? 1 : 0;
		page->user = (is_kernel == 1) ? 0 : 1;
		return;
	} else {
		uint32_t index = (uint32_t) pmm_alloc();
		if(index == NULL) {
			PANIC("Fatal: Out of free memory");
		}
		page->frame = index / 0x1000;
		page->present = 1;
		page->rw = (is_writable == 1) ? 1 : 0;
		page->user = (is_kernel == 1) ? 0 : 1;
	}
}

void map_page_dma(pte_t *page, int is_kernel, int is_writable, uint32_t address)
{
	page->frame = address / 0x1000;
	page->present = 1;
	page->rw = (is_writable == 1) ? 1 : 0;
	page->user = (is_kernel == 1) ? 0 : 1;
	pmm_mark_system((uintptr_t *) address, 0x1000);
}

void paging_init()
{
	debug("Page directory will occupy %d blocks.\n", sizeof(page_directory_t) / 0x1000);

	uintptr_t *pd_loc = pmm_alloc_n(sizeof(page_directory_t) / 0x1000);
	ASSERT(pd_loc != NULL, "Fatal: Out of free memory");

	debug("Got 0x%x - 0x%x as address for our page directory from the PMM.\n", pd_loc, pd_loc + sizeof(page_directory_t));

	kernel_directory = (page_directory_t *) pd_loc;

	memset(kernel_directory, 0, sizeof(page_directory_t));
	get_page(0, kernel_directory); // 0x0 is reserved for NULL pointers.

	// Ensure the physical address is OK
	kernel_directory->phys = (uintptr_t) kernel_directory->tables_phys;

	debug("Allocated page directory at 0x%x, size 0x%x\n", kernel_directory->phys, sizeof(page_directory_t));

	debug("Mapping 0x0 - 0x%x (kernel memory)\n", placement_pointer);

	// Map kernel-space into our directory
	for (uintptr_t j = 0x1000; j < placement_pointer; j+= 0x1000) {
		map_page_dma(get_page(j, kernel_directory), 0, 1, j);
	}

	debug("Mapping VGA text-mode (0xb800 - 0xc0000)\n");
	for (uintptr_t j = 0xb8000; j < 0xc0000; j += 0x1000) {
		map_page_dma(get_page(j, kernel_directory), 0, 1, j);
	}

	uint32_t fb_start = framebuffer_start_address();
	uint32_t fb_end = framebuffer_end_address();
	debug("Mapping framebuffer (0x%x - 0x%x)\n", fb_start, fb_end);
	for (uintptr_t j = (uintptr_t) fb_start; j < (uintptr_t) fb_end; j+= 0x1000) {
		map_page_dma(get_page(j, kernel_directory), 0, 1, j);
	}

	debug("Mapping kernel heap space\n");
	for (uintptr_t j = HEAP_OFFSET; j < KERNEL_HEAP_SIZE; j+= 0x1000) {
		map_page_dma(get_page(j, kernel_directory), 0, 1, j);
	}

	bitmap_t *heap_map = bitmap_create(KERNEL_HEAP_SIZE);
	debug("Mapping kernel heap bitmap (0x%x - 0x%x)\n", heap_map, heap_map + bitmap_size(heap_map));
	for (uintptr_t j = (uintptr_t) heap_map; j < heap_map + bitmap_size(heap_map); j += 0x1000) {
		map_page_dma(get_page(j, kernel_directory), 0, 1, j);
	}
	//map_page_dma(get_page(heap_map, kernel_directory), 0, 1, heap_map);
	debug("Installing heap\n");
	heap_install(heap_map);

	debug("Registered pagefault handler\n");
	register_interrupt_handler(14, &pagefault);

	paging_switch_directory(kernel_directory);

	debug("Paging enabled.\n");
}

pte_t *get_page(uint32_t address, page_directory_t *directory)
{
	address /= 0x1000;

	uint32_t directory_index = address / 1024;
	uint32_t table_index = address % 1024;

	if (directory->tables[directory_index]) {
		return &directory->tables[directory_index]->pages[table_index];
	} else {
		uint32_t tmp = (uint32_t) pmm_alloc();

		if(tmp == NULL) {
			PANIC("Fatal: Out of free memory");
		}

		directory->tables[directory_index] = (page_table_t *) tmp;
		memset(directory->tables[directory_index], 0, sizeof(page_table_t));
		directory->tables_phys[directory_index] = tmp | 0x7;
		return &directory->tables[directory_index]->pages[table_index];
	}

	return 0;
}

void paging_switch_directory(page_directory_t *directory)
{
	current_directory = directory;

	debug("Switching to new page directory at 0x%x\n", directory->phys);

	__asm__ __volatile__ (
				"mov %0, %%cr3\n"
				"mov %%cr0, %%eax\n"
				"orl $0x80000000, %%eax\n"
				"mov %%eax, %%cr0\n"
				:: "r"(directory->phys)
				: "%eax");
}

void pagefault(registers_t *regs)
{
	uint32_t faulting_address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));

    if (faulting_address == 0) {
    	PANIC("NULL POINTER EXCEPTION!");
    }

    int present = regs->err_code & 0x1; // Page not present
	int rw = regs->err_code & 0x2;           // Write operation?
	int us = regs->err_code & 0x4;           // Processor was in user-mode?
	int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

	kprintf(
		"Page fault! ( %s, %s, %s, %s, %s ) at 0x%x\nUsed memory: %d of %d pages\n",
		present ? "protection violation" : "non-present page",
		rw ? "on write" : "on read",
		us ? "user-mode" : "kernel-mode",
		reserved ? "read reserved field" : "did not read reserved field",
		id ? "instruction fetch" : "no instruction fetch",
		faulting_address,
		pmm_used_frames(),
		pmm_num_frames()
	);

	debug(
		"Page fault! ( %s, %s, %s, %s, %s ) at 0x%x\n",
		present ? "protection violation" : "non-present page",
		rw ? "on write" : "on read",
		us ? "user-mode" : "kernel-mode",
		reserved ? "read reserved field" : "did not read reserved field",
		id ? "instruction fetch" : "no instruction fetch",
		faulting_address
	);

	PANIC("PAGEFAULT!");
}
