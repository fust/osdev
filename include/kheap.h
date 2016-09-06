#ifndef __KHEAP_H
#define __KHEAP_H

#include <stdint.h>

typedef struct _KHEAPBLOCKSS {
	struct _KHEAPBLOCKSS	*next;
	uint32_t					top;
	uint32_t					max;
	uintptr_t					size;			/* total size in bytes including this header */
} KHEAPBLOCKSS;

typedef struct _KHEAPSS {
	KHEAPBLOCKSS			*fblock;
	uint32_t					bsize;
} KHEAPSS;

void k_heapSSInit(KHEAPSS *heap, uint32_t bsize);

void *k_heapSSAlloc(KHEAPSS *heap, uint32_t size);

void k_heapSSFree(KHEAPSS *heap, void *ptr);

#endif
