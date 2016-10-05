#ifndef __BITMAP_H
#define __BITMAP_H

#include "stdint.h"
#include "stdbool.h"

typedef struct bitmap {
	uintptr_t * map;
	uint32_t num_bytes;
} bitmap_t;

typedef uint32_t bitmap_index_t;

#define BYTE_SIZE (uint32_t) 32

bitmap_t * bitmap_create(uint32_t num_indices);

void bitmap_set(bitmap_t * bitmap, bitmap_index_t index);

void bitmap_clear(bitmap_t * bitmap, bitmap_index_t index);

bool bitmap_test(bitmap_t * bitmap, bitmap_index_t index);

uintptr_t bitmap_first_free(bitmap_t * bitmap);

uintptr_t bitmap_first_n_free(bitmap_t * bitmap, uint32_t n);

#endif
