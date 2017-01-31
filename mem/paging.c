#include "mem/paging.h"
#include "mem/pmm.h"
#include "mem/kmalloc.h"
#include "mem/kheap.h"
#include "sys/bitmap.h"
#include "string.h"
#include "stdint.h"
#include "cpu.h"
#include "idt.h"
#include "debug.h"
#include "spinlock.h"

#define KERN_HEAP_END 0x20000000

page_directory_t *kernel_directory = NULL;
page_directory_t *current_directory = NULL;
extern uint32_t placement_pointer;
extern bitmap_t *pmm_map;
extern uintptr_t *heap_start;
extern uint32_t heap_size;
extern uintptr_t heap_ptr;

spinlock_t alloc_slock;

/**
 * Map a page to an address in the physical memory
 */
void map_page(page_t *page, int is_writable, int is_kernel)
{
	if (!page) {
		PANIC("NULL pointer exception in map_page.");
	}
	if (page->frame != 0) { // Page is already mapped
		page->present = 1;
		page->rw = is_writable ? 1 : 0;
		page->user = is_kernel ? 0 : 1;
		return;
	} else { // Page is not mapped
		spin_lock(&alloc_slock); // This is getting a bit touchy....
		uint32_t frame = (uint32_t)bitmap_first_free(pmm_map);

		ASSERT(frame != (uint32_t) -1, "Out of free frames!");
		bitmap_set(pmm_map, (bitmap_index_t) frame);

		spin_unlock(&alloc_slock);

		page->present = 1;
		page->rw = is_writable ? 1 : 0;
		page->user = is_kernel ? 0 : 1;
		page->frame = (uintptr_t)(frame);
		debug("\tMAP_PAGE: Mapped new page at 0x%x phys\n", frame * 0x1000);
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
debug("MAP_DMA_PAGE: Mapped DMA page for 0x%x\n", address);
	bitmap_set(pmm_map, ((uint32_t)address / 0x1000));
}

void paging_init()
{
	// Allocate some memory for the kernel page directory
	uint32_t phys;
	kernel_directory = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), (uintptr_t *)(&phys));
	memset(kernel_directory, 0, sizeof(page_directory_t));

	kernel_directory->phys_address = (uintptr_t) kernel_directory->tables_phys;

	// Mark the first page (0x0 - 0xFFF) as non-present. (NULL-pointer detection)
	get_page((uintptr_t)0, 1, kernel_directory)->present = 0;

	// Now identity-map the kernel pages.
	uint32_t dma_end = (placement_pointer & 0xFFFFF000) + 0x1000;

	debug("PAGING: Mapping 0x1000 to 0x%x\n", (placement_pointer + 0x3000));
	for (uintptr_t i = 0x1000; i < (placement_pointer + 0x3000); i+= 0x1000) {
		map_dma_page(get_page(i, 1, kernel_directory), 0, 1, i);
	}
	debug("PAGING: [DONE] Mapping 0x1000 to 0x%x\n", (placement_pointer + 0x3000));

	// Identity-map the text-mode VGA buffer
	for (uint32_t i = 0xB8000; i <= 0xC0000; i+= 0x1000) {
		map_dma_page(get_page((uintptr_t)i, 1, kernel_directory), 1, 0, (uintptr_t)i);
	}

	// Register the pagefault handler
	register_interrupt_handler(14, &page_fault);

	// Set the heap's starting address
	heap_ptr = dma_end + 0x100000; // 0x3000 is the amount of blocks required by our cloned page table.
	heap_start = (uintptr_t *) heap_ptr;

	/* Kernel Heap Space */
	for (uintptr_t i = placement_pointer + 0x3000; i < heap_ptr; i += 0x1000) {
		map_page(get_page(i, 1, kernel_directory), 0, 1);
	}

	debug("PAGING: Preallocating heap starting at 0x%x.\n", heap_start);
	// Pre-allocate the heap's memory
	for (uintptr_t i = heap_ptr; i < KERN_HEAP_END; i+= 0x1000) {
		get_page(i, 1, kernel_directory);
	}

	heap_size = (KERN_HEAP_END - heap_ptr);

	debug("PAGING: Calculated heap size. Start: 0x%x, End: 0x%x, Size: 0x%x\n", heap_ptr, KERN_HEAP_END, heap_size);

	debug("PAGING: Cloning kernel directory: ");
	// Copy the current directory
	current_directory = clone_directory(kernel_directory);
	debug("cloned kernel directory from 0x%x to 0x%x physical (0x%x virtual)\n", kernel_directory->phys_address, current_directory->phys_address, current_directory);
	debug("Tables reside at virt: 0x%x, phys: 0x%x [0x%x kern]\n", &current_directory->tables, &current_directory->tables_phys, &kernel_directory->tables);

	//debug_dump_pgdir(current_directory);

	debug("PAGING: Switching to page directory at 0x%x\n", &current_directory->tables_phys);
	// Load CR3 with the new page directory
	switch_page_directory(current_directory);

	heap_install((placement_pointer + 0x1000) & ~0xFFF);

	debug("PAGING: Virtual memory and heap installed\n");
}

page_t *get_page(uintptr_t address, int make, page_directory_t * dir)
{
	address /= 0x1000; // 0x1000 = 4KB = 4096 bytes per page
	uint32_t table_idx = address / 1024; // 1024 pages per table

	if (make == 0) {
		debug("PAGING: Checking for page for address 0x%x in directory at 0x%x\n", (address * 0x1000), dir);
	}

	if (dir->tables[table_idx]) { // If the table already exists
		return &dir->tables[table_idx]->pages[address % 1024]; // Return the page's address. (address % 1024 is the offset into the table)
	} else if (make) { // Could not find the requested page but we were asked to create it, so.......
		uint32_t tmp; // This will temporarily hold the physical address of the new table
		dir->tables[table_idx] = (page_table_t *) kmalloc_p(sizeof(page_table_t), (uintptr_t *)(&tmp)); // Allocate some memory for the new table
		memset(dir->tables[table_idx], 0, sizeof(page_table_t)); // Clear the entire table
		dir->tables_phys[table_idx] = tmp | 0x7; // Load the physical address, set flags (Present, RW, User-mode)

		debug("PAGING: No table exists for address 0x%x. Created at v: 0x%x ph: 0x%x.\n", (address * 0x1000), dir->tables[table_idx], tmp | 0x7);

		return &dir->tables[table_idx]->pages[address % 1024]; // Same as above, return the page's address.
	} else { // Could not find the requested page and we were not asked to create one
		debug("PAGING: Could not find a page for address 0x%x!\n", (address * 0x1000));
		return NULL; // NULL pointer
	}
}

uintptr_t virt_to_phys(uintptr_t virt)
{
	/*page_t *page = get_page((uint32_t)virt, 0, current_directory);
	return page->frame*0x1000 + ((uint32_t)virt&0xFFF);*/

	uintptr_t remainder = virt % 0x1000;
	uintptr_t frame = virt / 0x1000;
	uintptr_t table = frame / 1024;
	uintptr_t idx = frame % 1024;

	if (current_directory->tables[table]) {
		page_t * p = &current_directory->tables[table]->pages[idx];
		return p->frame * 0x1000 + remainder;
	} else {
		return NULL;
	}
}

void switch_page_directory(page_directory_t * dir)
{
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

void invalidate_page_tables(void) {
	asm volatile (
			"movl %%cr3, %%eax\n"
			"movl %%eax, %%cr3\n"
			::: "%eax");
}

void debug_dump_pgdir(page_directory_t *dir)
{
	debug(" Dumping page directory: kern: 0x%x usr: 0x%x.\n================================\n", kernel_directory, dir);
	for (uintptr_t i = 0; i < 1024; i++) {
		if (!dir->tables[i] || dir->tables[i] == 0xFFFFFFFF) {
			continue;
		}
		if (kernel_directory->tables[i] == dir->tables[i]) {
			// This is a kernel directory
			debug("\t0x%x [kernel] (0x%x : 0x%x) vi: 0x%x\n", dir->tables[i], &dir->tables[i], &kernel_directory->tables[i], i * 0x1000 * 1024);
			for (uintptr_t j = 0; j < 1024; j++) {
				page_t *page = &dir->tables[i]->pages[j];
				if (page->frame) {
					debug("\t\t kern vi: 0x%x ph: 0x%x %s\n", (i * 1024 + j) * 0x1000, page->frame * 0x1000, page->present ? "[present]" : "");
				}
			}
		} else {
			debug("\t0x%x [user] [0x%x] 0x%x [0x%x]\n", dir->tables[i], &dir->tables[i], i * 0x1000 * 1024, kernel_directory->tables[i]);
			for (uintptr_t j = 0; j < 1024; j++) {
				page_t *page = &dir->tables[i]->pages[j];
				if (page->frame) {
					debug("\t\t      vi: 0x%x ph: 0x%x %s\n", (i * 1024 + j) * 0x1000, page->frame * 0x1000, page->present ? "[present]" : "");
				}
			}
		}
	}

	debug("================================\nDone.\n");
}
