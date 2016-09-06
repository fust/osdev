#include <kheap.h>
#include <stdio.h>

void k_heapSSInit(KHEAPSS *heap, uint32_t bsize) {
	heap->fblock = 0;
	heap->bsize = bsize;
}

int k_heapSSAddBlock(KHEAPSS *heap, uintptr_t addr, uint32_t size) {
	KHEAPBLOCKSS		*b;
	uint32_t				x;
	uint32_t				*stack;
	uint32_t				stacksz;
	uint32_t				slotres;

	b = (KHEAPBLOCKSS*)addr;
	b->next = heap->fblock;
	heap->fblock = b;

	b->size = size;

	size = size - sizeof(KHEAPBLOCKSS);

	b->max = size / (heap->bsize);

	stacksz = b->max * 4;

	slotres = (stacksz / heap->bsize) * heap->bsize < stacksz ? (stacksz / heap->bsize) + 1 : stacksz / heap->bsize;

	b->top = slotres;

	stack = (uint32_t*)&b[1];
	for (x = slotres; x < b->max; ++x) {
		stack[x] = x * heap->bsize;
	}

	return 1;
}

void *k_heapSSAlloc(KHEAPSS *heap, uint32_t size) {
	KHEAPBLOCKSS		*b;
	uintptr_t				ptr;
	uint32_t				*stack;

	/* too big */
	if (size > heap->bsize) {
		return 0;
	}

	for (b = heap->fblock; b; b = b->next) {
		if (b->top != b->max) {
			stack = (uint32_t*)&b[1];
			ptr = stack[b->top++];
			ptr = (uintptr_t)&b[1] + ptr;
			return (void*)ptr;
		}
	}

	/* no more space left */
	return 0;
}

void k_heapSSFree(KHEAPSS *heap, void *ptr) {
	KHEAPBLOCKSS		*b;
	uintptr_t				_ptr;
	uint32_t				*stack;

	/* find block */
	_ptr = (uintptr_t)ptr;
	for (b = heap->fblock; b; b = b->next) {
		if (_ptr > (uintptr_t)b && _ptr < ((uintptr_t)b + b->size))
			break;
	}

	/* might want to catch this somehow or somewhere.. kinda means corruption */
	if (!b)
		return;

	/* get stack */
	stack = (uint32_t*)&b[1];
	/* normalize offset to after block header */
	/* decrement top index */
	/* place entry back into stack */
	stack[--b->top] = _ptr - (uintptr_t)&b[1];
	return;
}
