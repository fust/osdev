#ifndef __KMALLOC_H
#define __KMALLOC_H

#include <stdint.h>

uintptr_t kmalloc_real(uint32_t size, int align, uintptr_t * physical);

uintptr_t kmalloc_a(uint32_t size);

uintptr_t kmalloc_p(uint32_t size, uintptr_t * physical);

uintptr_t kmalloc_ap(uint32_t size, uintptr_t * physical);

uintptr_t kmalloc(uint32_t size);

void kfree(void * p);

#endif
