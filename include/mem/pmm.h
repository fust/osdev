#ifndef __PMM_H
#define __PMM_H

#include "stdint.h"
#include "stddef.h"

void pmm_init(uint32_t mem_size, uint32_t kernel_end);

uintptr_t *pmm_alloc();

uintptr_t *pmm_alloc_n(size_t n);

void pmm_free(uintptr_t *p);

void pmm_mark_system (uintptr_t *base, uint32_t len);

#endif
