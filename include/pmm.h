#include "stdlib.h"
#include "stdbool.h"
#include "string.h"

#ifndef __PMM_H
#define __PMM_H

#define PMM_BLOCKS_PER_BYTE 8
#define PMM_BLOCK_SIZE 0x1000 // 4096 bytes
#define PMM_BLOCK_ALIGN PMM_BLOCK_SIZE

static uint32_t _pmm_size = 0;
static uint32_t _pmm_used = 0;
static uint32_t _pmm_max = 0;
static uint32_t *_pmm_bitmap=0;

bool pmm_bitmap_test(uint32_t bit);

uint32_t pmm_first_free();

uint32_t pmm_get_block_size();

uint32_t pmm_get_block_count();

void pmm_init(uint32_t memmax, uint32_t kstart, uint32_t kend);

void *pmm_alloc_block();

void pmm_free_block();

void *pmm_alloc_blocks(size_t size);

void pmm_free_blocks (void *p, size_t size);

#endif
