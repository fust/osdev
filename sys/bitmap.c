#include "sys/bitmap.h"
#include "stdint.h"
#include "stdbool.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "debug.h"

/**
 * Create a new bitmap
 *
 * num_indices: The number of indices to hold. Is divided by BYTE_SIZE internally.
 *
 * returns: the bitmap created or NULL on failure
 */
bitmap_t * bitmap_create(uint32_t num_indices)
{
	bitmap_t *b = (bitmap_t *)kmalloc((size_t) (sizeof(bitmap_t) + (num_indices / BYTE_SIZE)));

	if (b == NULL) {
		return NULL;
	}

	memset(b, 0, (size_t) (sizeof(bitmap_t) + (num_indices / BYTE_SIZE)));
	b->map = (uintptr_t *)b + sizeof(bitmap_t);
	b->num_bytes = num_indices / BYTE_SIZE;

	return b;
}

/**
 * Set the index in the given bitmap to 1
 */
void bitmap_set(bitmap_t * bitmap, bitmap_index_t index)
{
	bitmap->map[index / BYTE_SIZE] |= 1 << (index % 32);
}

/**
 * Set the index in the given bitmap to 0
 */
void bitmap_clear(bitmap_t * bitmap, bitmap_index_t index)
{
	bitmap->map[index / BYTE_SIZE] &= ~ (1 << (index % 32));
}

/**
 * Test if the index in the given bitmap is 1
 */
bool bitmap_test(bitmap_t * bitmap, bitmap_index_t index)
{
	return (bitmap->map[index / BYTE_SIZE] & (1 << (index % 32))) == 1 ? true : false;
}

/**
 * Find the first free index in the given bitmap or NULL when none found
 *
 * returns: pointer to the first available block or -1 when none found.
 */
bitmap_index_t bitmap_first_free(bitmap_t * bitmap)
{
	for (uint32_t i = 0; i < bitmap->num_bytes; i++) {
		if (bitmap->map[i] != 0xFFFFFFFF) { // If there are empty items in this byte
			for (uint32_t j = 0; j < BYTE_SIZE; j++) { // Loop over all bits and check for a free bit
				if (!(bitmap->map[i] & (1 << j))) {
					return (bitmap_index_t) (i * BYTE_SIZE + j);
				}
			}
		}
	}

	return -1;
}

/**
 * Find the first (consecutive) N free indices in the bitmap.
 *
 * returns: pointer to the first free block in the series or -1 when none found.
 */
bitmap_index_t bitmap_first_n_free(bitmap_t * bitmap, uint32_t n)
{
	bitmap_index_t found_address = -1;
	uint32_t found_count = 0;

	for (uint32_t i = 0; i < bitmap->num_bytes; i++) {
		if (bitmap->map[i] != 0xFFFFFFFF) { // If there are empty items in this byte
			for (uint32_t j = 0; j < BYTE_SIZE; j++) { // Loop over all bits and check for a free bit
				if (!(bitmap->map[i] & (1 << j))) {
					if (found_address == (uint32_t) -1) {
						found_address = (i * BYTE_SIZE + j);
					}
					found_count++;
					if (found_count == n) {
						return found_address;
					}
				} else {
					found_address = -1;
					found_count = 0;
				}
			}
		}
	}

	return -1;
}
