#include "mem/kheap.h"
#include "stdint.h"
#include "mem/paging.h"
#include "string.h"
#include "debug.h"

uintptr_t *heap_start;
uintptr_t heap_end = NULL;
uint32_t heap_size = 0x80000;

extern page_directory_t *current_directory;

void heap_install (uintptr_t *end)
{
	heap_end = (uintptr_t) end;
}

void *sbrk(uintptr_t size)
{
	uintptr_t sz = size * 0x1000;

#if 0
	debug("SBRK: st: 0x%x, end: 0x%x, sz: 0x%x, req: %d (0x%x)\n", (uintptr_t) heap_start, heap_end, heap_size, size, sz);
#endif
	ASSERT((heap_end - (uintptr_t) heap_start) + sz < heap_size, "Tried to allocate beyond end of heap!");

	uintptr_t *address = (uintptr_t*)heap_end;

	for (uintptr_t i = heap_end; i < heap_end + sz; i += 0x1000)
	{
		map_page(get_page(i, 0, current_directory), 1, 0);
	}

	heap_end += sz;
	memset((void *)address, 0, sz);

	return (void *)address;
}

int liballoc_lock()
{
	return 0; // Need to fix spinlocks first
}

int liballoc_unlock()
{
	return 0; // Need to fix spinlocks first
}

void* liballoc_alloc(int size)
{
	return sbrk((uintptr_t) size);
}

int liballoc_free(void* ptr,int sz)
{
	return 0; // We'll worry about this later
}
