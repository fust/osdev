#include "mem/kmalloc.h"
#include "stdint.h"
#include "string.h"

extern uint32_t placement_pointer; // Defined in pmm.c

uintptr_t *kmalloc_int(uint32_t size, uint32_t align, uintptr_t *phys)
{
	if (placement_pointer == 0) {
		return NULL;
	}

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
