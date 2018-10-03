#ifndef __KMALLOC_H
#define __KMALLOC_H

#include <stdint.h>
#include "mem/liballoc.h"

uint32_t kmalloc(uint32_t size);
uint32_t kmalloc_a(uint32_t size);
uint32_t kmalloc_p(uint32_t size, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);

uint32_t kfree(uint32_t *address);

uint32_t kcalloc(uint32_t size);

#endif
