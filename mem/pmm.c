#include "mem/pmm.h"
#include "mem/kmalloc.h"
#include "sys/bitmap.h"
#include "string.h"
#include "debug.h"

uint32_t placement_pointer = 0;
bitmap_t *pmm_map = NULL;

void pmm_set_kernel_end(uint32_t kernel_end)
{
	debug("PMM: Kerel's end is at 0x%x\n", placement_pointer);

	placement_pointer = kernel_end;
}

void pmm_init(uint32_t mem_size)
{
	pmm_map = bitmap_create(mem_size);

	debug("PMM: Allocated bitmap at 0x%x - 0x%x, internal map is at 0x%x\n", pmm_map, placement_pointer, pmm_map->map);
}

uintptr_t *pmm_alloc()
{
	if (!pmm_map) {
		return NULL;
	}

	uintptr_t address = bitmap_first_free(pmm_map);

	if (address == (uint32_t) -1) {
		return NULL;
	}

	bitmap_set(pmm_map, (bitmap_index_t) address);

	return (uintptr_t*) address;
}

uintptr_t *pmm_alloc_n(size_t n)
{
	if (!pmm_map) {
		return NULL;
	}

	uintptr_t address = bitmap_first_n_free(pmm_map, n);

	if (address == (uint32_t) -1) {
		return NULL;
	}

	for (uint32_t i = 0; i < n; i++) {
		bitmap_set(pmm_map, address + i);
	}

	return (uintptr_t*) address;
}

void pmm_free(uintptr_t *p)
{
	if (!pmm_map) {
		return;
	}

	bitmap_clear(pmm_map, (bitmap_index_t) p);
}

void pmm_mark_system (uintptr_t *base, uint32_t len)
{
	if (!pmm_map) {
		return;
	}

	for (uint32_t i = 0; i < len; i++) {
		bitmap_set(pmm_map, (bitmap_index_t)(base + (i * 0x1000)));
	}
}
