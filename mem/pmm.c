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
	frames = mem_size / 0x1000;
	pmm_map = bitmap_create(frames);

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

//	debug("PMM: Marking addresses 0x%x - 0x%x (length 0x%x) as system-reserved\n", (uint32_t)base, ((uint32_t)base + len), len);

	uint32_t act_len = len / 0x1000;
	uint32_t remainder = len % 0x1000;
	if (remainder > 0) {
		act_len++;
	}

	for (uint32_t i = 0; i < act_len; i++) {
		used_frames++;
		bitmap_set(pmm_map, (bitmap_index_t)(((uint32_t)base / 0x1000) + i));
	}
}

void pmm_dump_map()
{
        debug("PMM: Dumping map (4KB chunks, %d chunks, %d bytes of memory). 0 = free, 1 = occupied, \e[91mL\e[0m = kernel occupied \e[35mK\e[0m = kernel free\n", pmm_num_frames(), pmm_num_frames() * 0x1000);
        debug("Memory map is %d frames long, of which %d are occupied.\n", pmm_num_frames(), pmm_used_frames());
        debug("===================================\n");
        for (uint32_t i = 0; i <= pmm_map->num_bytes; i++) {
        	debug("%d\t", i);
			int16_t j = 0;
			for (; j <= 31; j++) {
					uint8_t res = (uint8_t)(((pmm_map->map[i]) & (1 << j)) != 0);
					debug("%d ", res);
			}
			debug("\t%x \t(0x%x - 0x%x) \n", pmm_map->map[i], i*(0x1000)*32, (i+1)*(0x1000)*32 - 0x1);
        }
        debug("\n=============================== END\n");
}

