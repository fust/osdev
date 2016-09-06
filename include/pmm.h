#include "stdlib.h"
#include "stdbool.h"
#include "string.h"

#ifndef __PMM_H
#define __PMM_H

#define PMM_BLOCKS_PER_BYTE 8
#define PMM_BLOCK_SIZE 4096
#define PMM_BLOCK_ALIGN PMM_BLOCK_SIZE

static uint32_t _pmm_size = 0;
static uint32_t _pmm_used = 0;
static uint32_t _pmm_max = 0;
static uint32_t *_pmm_bitmap=0;

void pmm_init(uint32_t memmax);

#endif
