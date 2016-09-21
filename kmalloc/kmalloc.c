#include <kmalloc.h>
#include <vmm.h>
#include <kheap.h>
#include <stdio.h>
#include <debug.h>
#include <liballoc.h>

extern uintptr_t end; // end is defined in the linker script. Points to kernel end.
uint32_t placement_pointer = (uint32_t)&end;

//extern KHEAPSS *kheap; // Defined in vmm.c
extern uintptr_t heap_end;

uint32_t heap_length = 0;

uintptr_t kmalloc_real(uint32_t size, int align, uint32_t * physical)
{
	if (heap_end) {
		uintptr_t ret = (uintptr_t) malloc(size);
		if (ret > 0) {
			if (physical) {
				physical = virtual_to_physical(ret);
				debug("\t\t\tPhysical address requested. Is: 0x%x\n", &physical);
				if (align) {
					physical = (uint32_t) &physical & 0xFFFFF000;
					physical = (uint32_t) &physical + 0x1000;
					debug("\t\t\tPhysical address aligned: 0x%x\n", &physical);
				}
			}
		//	debug("\t\tAllocated %d bytes at virtual address 0x%x\n", size, ret);
			return ret;
		}
	//	debug("\t\tGot 0x%x from malloc().\nOut of heap space. Trying to allocate another block.\n", ret);
#if 0
		if (kheap->bsize + (uint32_t) kheap < heap_end) {
			uint32_t address = ((uint32_t) kheap + ((heap_length * kheap->bsize) + kheap->bsize));
			if (address + ((heap_length * kheap->bsize) + kheap->bsize) > heap_end) {
				PANIC("Could not allocate requested %d bytes of memory.\nSystem halted. ", size);
				return 0;
			}
			debug("\t\tAllocating another block of heap space at 0x%x of %d bytes\n", address, (kheap->bsize * size));
			k_heapSSAddBlock(kheap, address, (kheap->bsize * size));
			heap_length+=size;
			debug("\t\tHeap now ends at 0x%x\n", kheap + ((heap_length * kheap->bsize) + kheap->bsize));
			debug("\t\tRecursing kmalloc_real\n");
			return kmalloc_real(size, align, physical);
		}
#endif
		PANIC("Could not allocate requested %d bytes of memory.\nSystem halted. ", size);
		return 0;
	}

#if 1
	if (align && (placement_pointer & 0xFFFFF000)) {
		placement_pointer &= 0xFFFFF000; // Round down to the lowest page-aligned address (4Kb)
		placement_pointer += 0x1000; // Add 1Kb to the pointer
	}

	if (physical) {
		*physical = placement_pointer;
	}

	uintptr_t address = placement_pointer;
	placement_pointer += size;
#endif
#if 0
	uintptr_t address = pmm_alloc_blocks((size / 0x1000) + 1);

	if (physical) {
		*physical = address;
	}
#endif
	return address;
}

uintptr_t kmalloc_a(uint32_t size)
{
	return kmalloc_real(size, 1, 0);
}

uintptr_t kmalloc_p(uint32_t size, uintptr_t * physical)
{
	return kmalloc_real(size, 0, physical);
}

uintptr_t kmalloc_ap(uint32_t size, uintptr_t * physical)
{
	return kmalloc_real(size, 1, physical);
}

uintptr_t kmalloc(uint32_t size)
{
	return kmalloc_real(size, 0, 0);
}

void kfree(void * p)
{
	if (heap_end) {
		return free(p);
	}
}
