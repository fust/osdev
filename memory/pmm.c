#include "pmm.h"
#include "kmalloc.h"
#include "stdio.h"

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

void pmm_init(uint32_t memmax)
{
	_pmm_size = memmax;
	_pmm_max = (pmm_get_memory_size() * 1024) / PMM_BLOCK_SIZE;
	_pmm_bitmap = kmalloc(pmm_get_block_count() / PMM_BLOCKS_PER_BYTE);
	_pmm_used = pmm_get_block_count();

	memset(_pmm_bitmap, 0x0, pmm_get_block_count() / PMM_BLOCKS_PER_BYTE);
	pmm_bitmap_set(0);
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
