#include "mem/pmm.h"
#include "mem/kmalloc.h"
#include "sys/bitmap.h"
#include "string.h"
#include "debug.h"
#include "stdint.h"

uint32_t placement_pointer = 0;
bitmap_t *pmm_map = NULL;
uint32_t frames = 0;
uint32_t used_frames = 0;

uint32_t pmm_num_frames()
{
	return frames;
}

uint32_t pmm_free_frames()
{
	return frames - used_frames;
}

uint32_t pmm_used_frames()
{
	return used_frames;
}

void pmm_set_kernel_end(uint32_t kernel_end)
{
	placement_pointer = kernel_end;
	debug("PMM: Kerel's end is at 0x%x\n", placement_pointer);
}

void pmm_init(uint32_t mem_size)
{
	pmm_map = bitmap_create(mem_size);
	frames = mem_size / 0x1000;

	debug("PMM: Allocated bitmap at 0x%x - 0x%x, internal map is at 0x%x\nMarking system pages...\n", pmm_map, placement_pointer, pmm_map->map);

	pmm_mark_system(0, placement_pointer); // Mark all allocated memory up to now as used.

	debug("PMM: Initialized.\n");
}

uintptr_t *pmm_alloc()
{
	if (!pmm_map) {
		return NULL;
	}

	uintptr_t frame = bitmap_first_free(pmm_map);

	if (frame == (uint32_t) -1) {
		return NULL;
	}

	bitmap_set(pmm_map, (bitmap_index_t) frame);
	used_frames++;

	return (uintptr_t*) (frame * 0x1000);
}

/**
 * Note: n is in blocks (4096 bytes)!
 */
uintptr_t *pmm_alloc_n(size_t n)
{
	if (!pmm_map) {
		return NULL;
	}

	if (n == 1) {
		return pmm_alloc();
	}

	uintptr_t frame = bitmap_first_n_free(pmm_map, n);

	if (frame == (uint32_t) -1) {
		return NULL;
	}

	for (uint32_t i = 0; i < n; i++) {
		bitmap_set(pmm_map, frame + i);
	}

	used_frames += n;

	return (uintptr_t*) (frame * 0x1000);
}

void pmm_free(uintptr_t *p)
{
	if (!pmm_map) {
		return;
	}

	used_frames--;

	bitmap_clear(pmm_map, (bitmap_index_t) ((uint32_t) p / 0x1000));
}

void pmm_mark_system (uintptr_t *base, uint32_t len)
{
	if (!pmm_map) {
		return;
	}

	for (uint32_t i = 0; i < (len / 0x1000); i++) {
		used_frames++;
		bitmap_set(pmm_map, (bitmap_index_t)((uint32_t)base + i));
	}
}
