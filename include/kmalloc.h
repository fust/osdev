#ifndef __KMALLOC_H
#define __KMALLOC_H

#include <stdint.h>

uint32_t kmalloc_real(uint32_t size, int align, uint32_t * physical);

uint32_t kmalloc_a(uint32_t size);

uint32_t kmalloc_p(uint32_t size, uint32_t * physical);

uint32_t kmalloc_ap(uint32_t size, uint32_t * physical);

uint32_t kmalloc(uint32_t size);

void kfree(void * p);

#endif
