#ifndef _VMM_H
#define _VMM_H

// DLMalloc defines

#include <stdint.h>
#include <cpu.h>

typedef struct page {
	unsigned int present:1;
	unsigned int rw:1;
	unsigned int user:1;
	unsigned int accessed:1;
	unsigned int dirty:1;
	unsigned int unused:7;
	unsigned int frame:20;
} __attribute__((packed)) page_t;

typedef struct page_table {
	page_t pages[1024];
} page_table_t;

typedef struct page_directory {
	uint32_t physical_tables[1024];	/* Physical addresses of the tables */
	page_table_t *tables[1024];	/* 1024 pointers to page tables... */
	uint32_t physical_address;	/* The physical address of physical_tables */
} page_directory_t;

void vmm_init_paging();

void vmm_switch_directory(page_directory_t *new);

page_t *get_page(uint32_t address, int make, page_directory_t *dir);

uint32_t virtual_to_physical(uint32_t virtual);

void page_fault(registers_t regs);

void heap_install(void );

void *alloc(uint32_t size);

void free(void * p);

#endif
