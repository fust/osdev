#ifndef __KHEAP_H
#define __KHEAP_H

#include "stdint.h"

void heap_install(uintptr_t *end);

void *sbrk(uint32_t size);

#endif
