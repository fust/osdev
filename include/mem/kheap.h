#ifndef __KHEAP_H
#define __KHEAP_H

#include <stdint.h>

void heap_install();
void* heap_sbrk(uint32_t num_pages);
uint32_t heap_free(void* address, uint32_t size);

#endif
