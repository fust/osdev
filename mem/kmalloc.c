#include "mem/kmalloc.h"
#include "mem/kheap.h"
#include "string.h"
#include <stdint.h>

extern uint32_t placement_pointer;
extern uint32_t heap_enabled;

uint32_t kmalloc_int(uint32_t size, uint8_t align, uint32_t *phys)
{
	if (heap_enabled) {
		// If the heap is enabled, we return only full pages.
		return (uint32_t) klmalloc(size);
	} else {
		if (align == 1 && (placement_pointer & 0xFFFFF000)) {
			placement_pointer &= 0xFFFFF000;
			placement_pointer += 0x1000;
		}

		if (phys) {
			*phys = placement_pointer;
		}

		uint32_t tmp = placement_pointer;
		placement_pointer += size;

		return tmp;
	}
}

uint32_t kmalloc(uint32_t size)
{
	return kmalloc_int(size, 0, 0);
}

uint32_t kmalloc_a(uint32_t size)
{
	return kmalloc_int(size, 1, 0);
}

uint32_t kmalloc_p(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 0, phys);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 1, phys);
}

uint32_t kfree(uint32_t *address)
{
	if(heap_enabled) {
		klfree(address);
	}
	return 0;
}

uint32_t kcalloc(uint32_t size)
{
	uint32_t p = kmalloc(size);
	if (p) {
		memset((void *)p, 0, size);
	}

	return p;
}

// Liballoc hooks

int liballoc_lock()
{
	return 0;
}

int liballoc_unlock()
{
	return 0;
}

void* liballoc_alloc(size_t size)
{
	return heap_sbrk(size);
}

int liballoc_free(void* address, size_t size)
{
	return heap_free(address, (uint32_t) size);
}
