#include "sys/bitmap.h"
#include "stdint.h"
#include "kmalloc.h"
#include "string.h"

bitmap_t bitmap_create(uint32_t num_indices)
{
}

void bitmap_set(bitmap_t * bitmap, bitmap_index_t index)
{
}

void bitmap_clear(bitmap_t * bitmap, bitmap_index_t index)
{
}

void bitmap_test(bitmap_t * bitmap, bitmap_index_t index)
{
}

uintptr_t bitmap_first_free(bitmap_t * bitmap)
{
}

uintptr_t bitmap_first_n_free(bitmap_t * bitmap, uint32_t n)
{
}
