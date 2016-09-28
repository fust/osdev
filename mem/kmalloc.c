#include "mem/kmalloc.h"
#include "mem/pmm.h"
#include "stdint.h"
#include "string.h"
#include "sys/bitmap.h"
#include "debug.h"

extern uint32_t placement_pointer; // Defined in pmm.c
extern bitmap_t *pmm_map; // Defined in pmm.c

uintptr_t *kmalloc_int(uint32_t size, uint32_t align, uintptr_t *phys)
{
	if (placement_pointer == 0 && !pmm_map) {
		return NULL;
	}

	if (!pmm_map) { // No physical memory management yet, use the dump allocator
		if (align) {
			placement_pointer &= 0xFFFFF000;
			placement_pointer += 0x1000;
		}

		uintptr_t address = placement_pointer;

		placement_pointer += size;

		if (phys) {
			phys = (uintptr_t *)address;
		}

		return (uintptr_t *)address;
	} else { // Physical memory management has been initialized, use that.
		uint32_t blocks = size / 0x1000;
		if (size % 0x1000 > 0) {
			blocks++;
		}
		uintptr_t *address = pmm_alloc_n(blocks);

		if (phys) {
			*phys = (uint32_t) address;
		}
		return address;
	}
}

uintptr_t *kmalloc(uint32_t size)
{
	return kmalloc_int(size, 0, NULL);
}

uintptr_t *kmalloc_a(uint32_t size)
{
	return kmalloc_int(size, 1, NULL);
}

uintptr_t *kmalloc_p(uint32_t size, uintptr_t *phys)
{
	return kmalloc_int(size, 0, phys);
}

uintptr_t *kmalloc_ap(uint32_t size, uintptr_t *phys)
{
	return kmalloc_int(size, 1, phys);
}

void kfree(void *p)
{
	if (pmm_map) {
		pmm_free((uintptr_t *) p);
	}
}
