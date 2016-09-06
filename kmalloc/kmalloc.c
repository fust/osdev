#include <kmalloc.h>
#include <vmm.h>
#include <kheap.h>
#include <stdio.h>

extern uint32_t end; // end is defined in the linker script. Points to kernel end.
uint32_t placement_pointer = (uint32_t)&end;

extern KHEAPSS *kheap; // Defined in vmm.c
extern uint32_t heap_end;

uint32_t kmalloc_real(uint32_t size, int align, uint32_t * physical)
{
	if (kheap) {
		uint32_t ret = alloc(size);
		if (ret > 0) {
			if (physical) {
				*physical = virtual_to_physical(ret);
			}
			return ret;
		}

		if (heap_end < kheap - kheap->bsize) {
			k_heapSSAddBlock(kheap, heap_end + 0x1000, size);
			heap_end += 0x1000;
			return kmalloc_real(size, align, physical);
		}

		return 0;
	}

	if (align && (placement_pointer & 0xFFFFF000)) {
		placement_pointer &= 0xFFFFF000; // Round down to the lowest page-aligned address (4Kb)
		placement_pointer += 0x1000; // Add 1Kb to the pointer
	}

	if (physical) {
		*physical = placement_pointer;
	}

	uint32_t address = placement_pointer;
	placement_pointer += size;

	return address;
}

uint32_t kmalloc_a(uint32_t size)
{
	return kmalloc_real(size, 1, 0);
}

uint32_t kmalloc_p(uint32_t size, uint32_t * physical)
{
	return kmalloc_real(size, 0, physical);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t * physical)
{
	return kmalloc_real(size, 1, physical);
}

uint32_t kmalloc(uint32_t size)
{
	return kmalloc_real(size, 0, 0);
}

void kfree(void * p)
{
	if (kheap) {
		return free(p);
	}
}
