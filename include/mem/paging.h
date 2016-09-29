#ifndef __PAGING_H
#define __PAGING_H

#include "stdint.h"
#include "cpu.h"

typedef struct page {
	uint32_t present		: 1;	// Present
	uint32_t rw				: 1;	// Read/Write
	uint32_t user			: 1;	// User/Supervisor
	uint32_t writethrough	: 1;	// Write Through
	uint32_t cachedisable	: 1;	// Cache disabled
	uint32_t unused			: 7;	// Ignored by CPU, available for use by OS
	uintptr_t frame			: 20;	// Physical address of the 4KB frame
} page_t;

typedef struct page_table {
	page_t pages[1024];			// 1024 pages in a page table (Makes 4MB per page table)
} page_table_t;

typedef struct page_directory {
	uintptr_t tables_phys[1024];	// Physical addresses of our page tables. This is what the MMU uses for address translation
	page_table_t *tables[1024];		// 1024 page tables in a page directory	(Makes 4GB per page directory) (virtual addresses, we use these in the kernel)
	uintptr_t phys_address;			// Physical address of the page directory, to be loaded into CR3
} page_directory_t;

void paging_init();

void map_page(page_t *page, int is_writable, int is_kernel);

page_t *get_page(uintptr_t address, int make, page_directory_t * dir);

void switch_page_directory(page_directory_t * dir);

void page_fault(registers_t regs);

#endif
