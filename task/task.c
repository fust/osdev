#include <task.h>
#include <kmalloc.h>
#include <string.h>
#include <vmm.h>

extern page_directory_t * kernel_directory; // Defined in vmm.c
extern page_directory_t * current_directory;

page_directory_t *clone_directory(page_directory_t * src)
{
	uint32_t phys;
	page_directory_t * dir = (page_directory_t *)kmalloc_p(sizeof(page_directory_t), &phys);

	memset(dir, 0, sizeof(page_directory_t));

	dir->physical_address = phys;
	uint32_t i;
	for (i = 0; i < 1024; i++) {
		if (!src->tables[i] || (uint32_t)src->tables[i] == (uint32_t)0xFFFFFFFF) {
			continue;
		}
		if (kernel_directory->tables[i] == src->tables[i]) {
			/* Kernel tables are simply linked together */
			dir->tables[i] = src->tables[i];
			dir->physical_tables[i] = src->physical_tables[i];
		} else {
			/* User tables must be cloned */
			uint32_t phys;
			dir->tables[i] = clone_table(src->tables[i], &phys);
			dir->physical_tables[i] = phys | 0x07;
		}
	}
	return dir;
}

page_table_t * clone_table(page_table_t * src, uint32_t * physAddr)
{
	page_table_t * table = (page_table_t *)kmalloc_p(sizeof(page_table_t), physAddr);
	memset(table, 0, sizeof(page_table_t));
	uint32_t i;
	for (i = 0; i < 1024; ++i) {
		/* For each frame in the table... */
		if (!src->pages[i].frame) {
			continue;
		}
		/* Allocate a new frame */
		alloc_frame(&table->pages[i], 0, 0);
		/* Set the correct access bit */
		if (src->pages[i].present)	table->pages[i].present = 1;
		if (src->pages[i].rw)		table->pages[i].rw = 1;
		if (src->pages[i].user)		table->pages[i].user = 1;
		if (src->pages[i].accessed)	table->pages[i].accessed = 1;
		if (src->pages[i].dirty)	table->pages[i].dirty = 1;
		/* Copy the contents of the page from the old table to the new one */
		copy_page_physical(src->pages[i].frame * 0x1000, table->pages[i].frame * 0x1000);
	}
	return table;
}
