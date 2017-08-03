#ifndef __PMM_H
#define __PMM_H

#include <stdint.h>
#include <stddef.h>

uint32_t pmm_num_frames();

uint32_t pmm_free_frames();

uint32_t pmm_used_frames();

void pmm_set_kernel_end(uint32_t kernel_end);

void pmm_init(uint32_t mem_size);

uintptr_t *pmm_alloc();

uintptr_t *pmm_alloc_n(size_t n);

void pmm_free(uintptr_t *p);

void pmm_mark_system (uintptr_t *base, uint32_t len);

void pmm_dump_map();

#endif
