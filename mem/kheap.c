#include "mem/kheap.h"
#include "stdint.h"
#include "mem/paging.h"
#include "string.h"
#include "spinlock.h"
#include "debug.h"

uintptr_t *heap_start;
uintptr_t heap_end = NULL;
uint32_t heap_size = 0x1000000;
uintptr_t heap_ptr;

extern page_directory_t *current_directory;

spinlock_t liballoc_slock;

void heap_install (uintptr_t *end)
{
	heap_end = (uintptr_t) end;
}

void *sbrk(uintptr_t size)
{
	uintptr_t sz = size * 0x1000;

	debug("SBRK: st: 0x%x, end: 0x%x, sz: 0x%x, req: %d (0x%x)\n", (uintptr_t) heap_start, heap_end, heap_size, size, sz);
	ASSERT((heap_end + sz) < heap_size, "Tried to allocate beyond end of heap!");

	uintptr_t address = heap_end;

	if (heap_end + sz > heap_ptr) {
		debug("\tSBRK: End of kernel heap, allocating more (we're at 0x%x and want to be at 0x%x)\n", heap_end, heap_end + sz);
		for (uintptr_t i = heap_end; i < heap_end + sz; i += 0x1000) {
			map_page(get_page(i, 0, current_directory), 1, 0);
		}

		invalidate_page_tables();
	}

	heap_end += sz;
	memset((void *)address, 0, sz);

	debug("SBRK: Done. Mapped at: 0x%x\n", address);
	debug("SBRK: Memory usage : %dKB of %dKB\n", (pmm_used_frames() * 0x1000) / 1024, (pmm_num_frames() * 0x1000) / 1024);

	return (void *)address;
}

int liballoc_lock()
{
	spin_lock(&liballoc_slock);
	return 0;
}

int liballoc_unlock()
{
	spin_unlock(&liballoc_slock);
	return 0;
}

void* liballoc_alloc(int size)
{
	return sbrk((uintptr_t) size);
}

int liballoc_free(void* ptr,int sz)
{
	debug("LIBALLOC_FREE: 0x%x - 0x%x\n", ptr, ((int) ptr) + sz);
	return 0; // We'll worry about this later
}
