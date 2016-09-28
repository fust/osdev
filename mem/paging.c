#include "mem/paging.h"
#include "mem/pmm.h"
#include "mem/kmalloc.h"
#include "sys/bitmap.h"
#include "string.h"
#include "stdint.h"
#include "cpu.h"
#include "idt.h"
#include "debug.h"

page_directory_t *kernel_directory = NULL;
page_directory_t *current_directory = NULL;
extern bitmap_t *pmm_map;

/**
 * Map a page to an address in the physical memory
 */
void map_page(page_t *page, int is_writable, int is_kernel)
{
	if (page->frame != 0) { // Page is already mapped
		page->present = 1;
		page->rw = is_writable ? 1 : 0;
		page->user = is_kernel ? 0 : 1;
		return;
	} else { // Page is not mapped
		uint32_t frame = (uint32_t)bitmap_first_free(pmm_map);

		ASSERT(frame != (uint32_t) -1, "Out of free frames!");
		bitmap_set(pmm_map, (bitmap_index_t) frame);

		page->present = 1;
		page->rw = is_writable ? 1 : 0;
		page->user = is_kernel ? 0 : 1;
		page->frame = (uintptr_t)(frame * 0x1000);
	}
}

/**
 * Map a page to a specific physical address.
 */
void map_dma_page(page_t *page, int is_writable, int is_kernel, uintptr_t address)
{
	page->present = 1;
	page->rw = is_writable ? 1 : 0;
	page->user = is_kernel ? 0 : 1;
	page->frame = address / 0x1000;

	bitmap_set(pmm_map, ((uint32_t)address / 0x1000));
}

void paging_init()
{
	// Allocate some memory for the kernel page directory
	uint32_t phys;
	kernel_directory = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), (uintptr_t *)(&phys));
	kernel_directory->phys_address = phys;

	// Mark the first page (0x0 - 0xFFF) as non-present. (NULL-pointer detection)
	get_page((uintptr_t)0, 1, kernel_directory)->present = 0;

	// Now identity-map the kernel pages.
	// Since the page directory is the last allocated, we can (though shouldn't!) assume that is our identity-map end.
	uint32_t dma_end = ((phys + sizeof(page_directory_t)) & 0xFFFFF000) + 0x1000;

	for (uint32_t i = 0x100000; i <= dma_end; i += 0x1000) {
		map_dma_page(get_page((uintptr_t)i, 1, kernel_directory), 1, 1, (uintptr_t)i);
	}

	// Identity-map the text-mode VGA buffer
	for (uint32_t i = 0xB8000; i <= 0xC0000; i+= 0x1000) {
		map_dma_page(get_page((uintptr_t)i, 1, kernel_directory), 1, 0, (uintptr_t)i);
	}

	// Register the pagefault handler
	register_interrupt_handler(14, &page_fault);

	// Load CR3 with the new page directory
	switch_page_directory(kernel_directory);
}

page_t *get_page(uintptr_t address, int make, page_directory_t * dir)
{
	address /= 0x1000; // 0x1000 = 4KB = 4096 bytes per page
	uint32_t table_idx = address / 1024; // 1024 pages per table
	if (dir->tables[table_idx]) { // If the table already exists
		return &dir->tables[table_idx]->pages[address % 1024]; // Return the page's address. (address % 1024 is the offset into the table)
	} else if (make) { // Could not find the requested page but we were asked to create it, so.......
		uint32_t tmp; // This will temporarily hold the physical address of the new table
		dir->tables[table_idx] = (page_table_t *) kmalloc_p(sizeof(page_table_t), (uintptr_t *)(&tmp)); // Allocate some memory for the new table
		memset(dir->tables[table_idx], 0, sizeof(page_table_t)); // Clear the entire table
		dir->tables_phys[table_idx] = tmp | 0x7; // Load the physical address, set flags (Present, RW, User-mode)
		return &dir->tables[table_idx]->pages[address % 1024]; // Same as above, return the page's address.
	} else { // Could not find the requested page and we were not asked to create one
		return 0; // Return 0, AKA NULL pointer
	}
}

void switch_page_directory(page_directory_t * dir) {
	current_directory = dir;
	asm volatile (
			"mov %0, %%cr3\n"
			"mov %%cr0, %%eax\n"
			"orl $0x80000000, %%eax\n"
			"mov %%eax, %%cr0\n"
			:: "r"(dir->phys_address)
			: "%eax");
}

void page_fault(registers_t regs)
{
	uint32_t faulting_address;
	asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

	int present   = !(regs.err_code & 0x1); // Page not present
	int rw = regs.err_code & 0x2;           // Write operation?
	int us = regs.err_code & 0x4;           // Processor was in user-mode?
	int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

	PANIC("Page fault! at 0x%x (EIP: 0x%x p: %d, rw: %d, us: %d, reserved: %d, i: %d)\n",
				faulting_address, regs.eip, present, rw, us, reserved, id);
}
