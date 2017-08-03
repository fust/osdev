#ifndef __PAGING_H
#define __PAGING_H

#include <stdint.h>
#include "cpu.h"

typedef struct pte {
	uint32_t present : 1;
	uint32_t rw : 1;
	uint32_t user : 1;
	uint32_t wt : 1;
	uint32_t cd : 1;
	uint32_t accessed : 1;
	uint32_t dirty : 1;
	uint32_t zero : 1;
	uint32_t global : 1;
	uint32_t avail : 3;
	uint32_t frame : 20;
} pte_t;

typedef struct page_table {
	pte_t pages[1024];
} page_table_t;

typedef struct page_directory {
	uintptr_t tables_phys[1024];
	page_table_t *tables[1024];
	uintptr_t phys;
} page_directory_t;

void paging_init();

void pagefault(registers_t *regs);

#endif
