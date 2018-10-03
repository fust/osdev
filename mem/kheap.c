#include "mem/kheap.h"
#include "sys/bitmap.h"
#include "mem/pmm.h"
#include "stddef.h"
#include "debug.h"

uint32_t heap_enabled = 0;
bitmap_t *map;

uint32_t KERNEL_HEAP_SIZE = 0xA00000; // 10MB
uint32_t HEAP_OFFSET = 0x500000; // 5MB

void heap_install(bitmap_t *bitmap)
{
	map = bitmap;
	heap_enabled = 1;
}

void *heap_sbrk(uint32_t num_pages)
{
	debug("[HEAP] Will try to allocate %d page(s) of memory\n", num_pages);
	uint32_t free = bitmap_first_n_free(map, num_pages);

	if (free == (uint32_t) -1) {
		PANIC("[HEAP] OUT OF MEMORY while requesting %d page(s) of memory. (%d of %d frames used)", num_pages, pmm_used_frames(), pmm_num_frames());
		return NULL;
	}

	for (uint32_t i = 0; i < num_pages; i++) {
		bitmap_set(map, free + i);
	}

	debug("[HEAP] The start address of the new page(s) is: 0x%x\n", (free * 0x1000 + HEAP_OFFSET));

	return (void*) (free * 0x1000 + HEAP_OFFSET);
}

uint32_t heap_free(void* address, uint32_t size)
{
	uint32_t i = ((uint32_t) address - HEAP_OFFSET) / 0x1000;

	for (; i < size; i++) {
		bitmap_clear(map, (bitmap_index_t) i);
	}

	return 0;
}
