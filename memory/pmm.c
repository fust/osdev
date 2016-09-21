#include "pmm.h"
#include "kmalloc.h"
#include "stdio.h"
#include <debug.h>

extern uint32_t placement_pointer;

void pmm_bitmap_set(uint32_t bit)
{
        _pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

void pmm_bitmap_unset(uint32_t bit)
{
        _pmm_bitmap[bit / 32] &= ~ (1 << (bit / 32));
}

bool pmm_bitmap_test(uint32_t bit)
{
        return _pmm_bitmap[bit / 32] & (1 << (bit / 32));
}

uint32_t pmm_get_memory_size()
{
        return _pmm_size;
}

uint32_t pmm_get_block_count()
{
        return _pmm_max;
}

uint32_t pmm_get_used_block_count()
{
	return _pmm_used;
}

uint32_t pmm_get_free_block_count()
{
	return _pmm_size - _pmm_used;
}

uint32_t pmm_get_block_size()
{
	return PMM_BLOCK_SIZE;
}

uint32_t pmm_first_free()
{
	for (uint32_t i=0; i < pmm_get_block_count() / 32; i++)
	{
		if (_pmm_bitmap[i] != 0xFFFFFFFF)
		{
			for (uint32_t j=0; j < 32; j++)
			{
				uint32_t bit = 1 << j;
				if (!(_pmm_bitmap[i] & bit))
				{
					return i*4*8+j;
				}
			}
		}
	}
	return -1;
}

int pmm_first_free_s (size_t size) {

        if (size==0)
                return -1;

        if (size==1)
                return pmm_first_free ();

        for (uint32_t i = 0; i < pmm_get_block_count(); i++)
        {
			if (_pmm_bitmap[i] != 0xffffffff)
			{
				for (int j = 0; j < 32; j++) {
						int bit = 1 << j;
						if (!(_pmm_bitmap[i] & bit) )
						{
							int startingBit = i * 32;
							startingBit += bit;

							uint32_t free = 0;
							for (uint32_t count = 0; count <= size; count++) {
								if (!pmm_bitmap_test(startingBit + count))
									free++; // this bit is clear (free frame)

								if (free==size)
									return i*4*8+j; //free count==size needed; return index
							}
						}
				}
			}
        }

        return -1;
}

void pmm_init(uint32_t memmax, uint32_t kstart, uint32_t kend)
{
	_pmm_size = memmax;
	_pmm_max = (pmm_get_memory_size() * 1024) / PMM_BLOCK_SIZE;
	_pmm_bitmap = (uint32_t *) kend + 0x1000;
	debug("PMM: Our bitmap is at 0x%x - 0x%x\n", _pmm_bitmap, _pmm_bitmap + (pmm_get_block_count() / PMM_BLOCKS_PER_BYTE));
	_pmm_used = 0;

	memset(_pmm_bitmap, 0x0, pmm_get_block_count() / PMM_BLOCKS_PER_BYTE);

	uint32_t aligned_kend = ((kend & 0xFFFFF000) + 0x1000) + (((uint32_t)(_pmm_bitmap + (pmm_get_block_count() / PMM_BLOCKS_PER_BYTE)) & 0xFFFFF000) + 0x1000);

	debug("Reserving lower memory 0x0 - 0x%x\n", kstart);
	uint32_t i;
	for (i = 0x0; i < (kstart / 0x1000); i++)
	{
		pmm_bitmap_set(i);
		_pmm_used++;
	}
	debug("\tReserved %d blocks of lower memory\n", i);

	debug("Reserving kernel area: 0x%x - 0x%x + 0x3000\n", kstart, aligned_kend);
	for (i = (kstart / 0x1000); i < ((aligned_kend + 0x3000) / 0x1000) + 0x1000; i++)
	{
		pmm_bitmap_set(i);
		_pmm_used++;
	}
	debug("\tReserved %d blocks of kernel memory\n", i);

	placement_pointer = (aligned_kend + 0x3000);

	debug("PMM: Marked kernel's end at: 0x%x\n", placement_pointer);
}

uint32_t mmap_first_free () {

	//! find the first free bit
	for (uint32_t i=0; i< pmm_get_block_count() /32; i++)
	{
		if (_pmm_bitmap[i] != 0xffffffff)
		{
			for (uint32_t j=0; j<32; j++) {
				uint32_t bit = 1 << j;
				if (! (_pmm_bitmap[i] & bit) )
				{
					return i*4*8+j;
				}
			}
		}
	}

	return -1;
}

void *pmm_alloc_block()
{
	if (pmm_get_free_block_count() <= 0)
	{
		return 0;
	}

	uint32_t frame = pmm_first_free();

	if (frame == -1)
	{
		return 0;
	}

	pmm_bitmap_set(frame);

	uint32_t physical = frame * PMM_BLOCK_SIZE;

	_pmm_used++;

	return (void *)physical;
}

void pmm_free_block(void *p)
{
	uint32_t physical = (uint32_t)p;
	uint32_t frame = physical / PMM_BLOCK_SIZE;

	pmm_bitmap_unset(frame);
	_pmm_used--;
}

void *pmm_alloc_blocks(size_t size)
{
	if (pmm_get_free_block_count() <= size)
		return 0;

	uint32_t frame = pmm_first_free_s(size);

	if (frame == -1)
		return 0;

	debug("\t%d blocks requested from PMM\n", size);

	for (uint32_t i = 0; i < size; i++)
		pmm_bitmap_set(frame + i);

	uintptr_t addr = frame * PMM_BLOCK_SIZE;
	_pmm_used += size;

	debug("\tAllocated at physical 0x%x\n", addr);
	return (void*) addr;
}

void pmm_free_blocks (void *p, size_t size)
{
	uintptr_t addr = (uintptr_t) p;
	uint32_t frame = addr / PMM_BLOCK_SIZE;

	for (uint32_t i = 0; i < size; i++)
		pmm_bitmap_unset(frame + i);

	_pmm_used -= size;
}
