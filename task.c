#include "task.h"
#include "stdint.h"
#include "mem/paging.h"
#include "mem/kmalloc.h"
#include "debug.h"

extern page_directory_t *kernel_directory;

page_directory_t *clone_directory(page_directory_t *src)
{
	uintptr_t *phys;

	// Allocate new directory ...
	page_directory_t *dest = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), phys);
	// ... and zero it out
	memset(dest, 0, sizeof(page_directory_t));

	// Calculate the physical offset
	uint32_t offset = (uint32_t)dest->tables_phys - (uint32_t)dest;
	// Set physical address
	dest->phys_address = *phys + offset;

	for (uint32_t i = 0; i < 1024; i++) {
		if (!src->tables[i] || src->tables[i] == 0xFFFFFFFF) {
			continue;
		}
		if (src->tables[i] == kernel_directory->tables[i]) {
			// Kernel tables are linked
			dest->tables[i] = src->tables[i];
			dest->tables_phys[i] = src->tables_phys[i];
			debug("TASK: Linking table at 0x%x (0x%x) for address 0x%x - 0x%x\n", &src->tables_phys[i], &dest->tables_phys[i], i * 0x1000 * 1024, (i * 0x1000 * 1024) + 0x3FFFFF);
		} else {
			debug("TASK:\ttable at 0x%x is user-space. Skipping.\n", &src->tables_phys[i]);
			// User-space tables are copied.
			// @todo: Implement this.
		}
	}

	return dest;
}
