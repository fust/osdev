#include <vmm.h>
#include <pmm.h>
#include <idt.h>
#include <stdio.h>
#include <kheap.h>
#include <task.h>
#include <debug.h>
#include <video.h>
#include <kmalloc.h>

#define KERNEL_HEAP_INIT 0x00800000
#define KERNEL_HEAP_END  0x04000000

// The kernel's page directory
page_directory_t *kernel_directory=0;

// The current page directory;
page_directory_t *current_directory=0;

extern uint32_t placement_pointer; // Defined in kmalloc.c
uintptr_t heap_alloc_point = KERNEL_HEAP_INIT;
uintptr_t heap_end = 0;
extern uint32_t initial_esp;

void alloc_frame(page_t *page, int is_kernel, int is_writable)
{
	if (page->frame != 0) {
		return; // Already allocated
	}
	else
	{
		uint32_t index = pmm_first_free();

		ASSERT(index != (uint32_t)-1, "Out of frames.\n");

		pmm_bitmap_set(index);

		//debug("\tVMM: Allocate frame at 0x%x", (index * 0x1000));

		page->present = 1;
		page->rw = (is_writable) ? 1 : 0;
		page->user = (is_kernel) ? 0 : 1;
		page->frame = index * 0x1000;
	}
}

void alloc_dma_frame(page_t *page, int is_kernel, int is_writable, uint32_t address)
{
	ASSERT(address > 0x0, "Tried to allocate DMA frame at 0x0!\n");
	ASSERT(is_kernel || !pmm_bitmap_test(address), "Physical frame 0x%x is already taken!\n", address);

	page->present = 1;
	page->rw = (is_writable) ? 1 : 0;
	page->user = (is_kernel) ? 0 : 1;
	page->frame = address / 0x1000;
	pmm_bitmap_set(address);
}

void free_frame(page_t *page)
{
	uint32_t frame;
	if (!(frame = page->frame))
	{
		return; // Not allocated
	}
	else
	{
		pmm_free_block(page->frame);
		page->frame = 0x0;
	}
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir)
{
	address /= 0x1000;
	uint32_t index = address / 1024;

	if (dir->tables[index]) {
		return &dir->tables[index]->pages[address % 1024];
	} else if (make) {
		uint32_t tmp;
		dir->tables[index] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
		debug("Created page table for address 0x%x at 0x%x\n", (address * 0x1000), tmp);
		memset(dir->tables[index], 0, sizeof(page_table_t));
		dir->physical_tables[index] = tmp | 0x7; // PRESENT, RW, US
		return &dir->tables[index]->pages[address % 1024];
	} else {
		return 0;
	}
}

void vmm_init_paging()
{
	kernel_directory = (page_directory_t*) (placement_pointer + 0x1000);//pmm_alloc_blocks(sizeof(page_directory_t) + 1);
	placement_pointer += (sizeof(page_directory_t) + 0x1000);
	placement_pointer = (placement_pointer & 0xFFFFF000) + 0x1000;
	//kernel_directory = (page_directory_t*) kmalloc_a(sizeof(page_directory_t));
	memset(kernel_directory, 0, sizeof(page_directory_t));
}

void vmm_paging_activate() {
	debug("Kernel directory is at 0x%x - 0x%x (size: 0x%x)\n", kernel_directory, ((uint32_t)kernel_directory + sizeof(page_directory_t)), sizeof(page_directory_t));

	get_page(0,1,kernel_directory)->present = 0;
	pmm_bitmap_set(0);

	int i = 0x1000;
	debug("Mapping kernel memory and page directory structures from 0x1000 to 0x%x\n", placement_pointer);
	while (i < placement_pointer)
	{
		alloc_dma_frame(get_page(i, 1, kernel_directory), 1, 1, i);
		i += 0x1000; // 4096, AKA 4KB, AKA page size
	}
#if 1
	debug("Mapping page directory structures (0x%x - 0x%x)\n", kernel_directory, ((uint32_t)kernel_directory + sizeof(page_t) * pmm_get_block_count()));
	for (uint32_t x = (uint32_t)kernel_directory; x < ((uint32_t)kernel_directory + (sizeof(page_t) * pmm_get_block_count())); x += 0x1000)
	{
		alloc_dma_frame(get_page(x, 0, kernel_directory), 1, 1, x);
	}
#endif

	debug("Mapping VGA buffer\n");
	// Identity map VGA buffer
	for (uint32_t j = 0xb8000; j < 0xc0000; j += 0x1000) {
		alloc_dma_frame(get_page(j, 0, kernel_directory), 1, 1, j);
	}

	debug("Registering page fault handler\n");
	register_interrupt_handler(14, page_fault);
	kernel_directory->physical_address = (uint32_t) kernel_directory->physical_tables;

	debug("Allocating heap: ");
	//heap_alloc_point = (uintptr_t) pmm_alloc_blocks((KERNEL_HEAP_END - KERNEL_HEAP_INIT) / 0x1000);
	debug(" from 0x%x to 0x%x\n", heap_alloc_point, (heap_alloc_point + (KERNEL_HEAP_END - KERNEL_HEAP_INIT)));
#if 1
	uintptr_t kheap_pointer_temp = KERNEL_HEAP_INIT;

	if (kheap_pointer_temp <= placement_pointer + 0x3000) {
		debug("Fixing heap pointer\n");
		kheap_pointer_temp = placement_pointer + 0x100000;
		heap_alloc_point = kheap_pointer_temp;
	}

	debug("Mapping heap memory: 0x%x to 0x%x\n", heap_alloc_point, kheap_pointer_temp);
	// Heap space
	for (uint32_t i = placement_pointer + 0x3000; i < kheap_pointer_temp; i += 0x1000) {
		alloc_frame(get_page(i, 1, kernel_directory), 1, 0);
	}
#endif
#if 1
	debug("Preallocating entire heap: 0x%x to 0x%x\n", kheap_pointer_temp, KERNEL_HEAP_END);
	/* And preallocate the page entries for all the rest of the kernel heap as well */
	for (uint32_t i = kheap_pointer_temp; i < KERNEL_HEAP_END; i += 0x1000) {
		get_page(i, 1, kernel_directory);
	}
	memset((void *)heap_alloc_point, 0, (KERNEL_HEAP_END - KERNEL_HEAP_INIT));
#endif

	debug("Cloning current directory\n");
	current_directory = clone_directory(kernel_directory);

	uint32_t stack = initial_esp & 0xFFFFF000;

	debug("Stack is at 0x%x phys, will be 0x%x virt.\n", stack, get_page(stack, 0, current_directory)->frame * 0x1000);

	debug("Switching page directory: 0x%x\n", current_directory->physical_address);
	vmm_switch_directory(current_directory);
#if 0
	dump_directory(kernel_directory);
#endif
}

void vmm_switch_directory(page_directory_t *new)
{
	current_directory = new;

	debug("SWITCHING CONTEXT: 0x%x (virt: 0x%x)\n", new->physical_address, new);

	asm volatile (
				"mov %0, %%cr3\n"
				"mov %%cr0, %%eax\n"
				"orl $0x80000000, %%eax\n"
				"mov %%eax, %%cr0\n"
				:: "r"(new->physical_address)
				: "%eax");
#if 0
	asm volatile("mov %0, %%cr3":: "r"(new->physical_address));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
#endif
}

void invalidate_page_tables(void) {
	asm volatile (
			"movl %%cr3, %%eax\n"
			"movl %%eax, %%cr3\n"
			::: "%eax");
}

void dump_directory(page_directory_t *dir)
{
	debug(" ---- [k:0x%x u:0x%x]\n", kernel_directory, dir);

	for (uint32_t i = 0; i < 1024; i++) {
		if (!dir->tables[i] || (uintptr_t)dir->tables[i] == (uintptr_t)0xFFFFFFFF) {
			continue;
		}
		if (kernel_directory->tables[i] == dir->tables[i]) {
			debug("  0x%x - kern [0x%x/0x%x] 0x%x\n", dir->tables[i], &dir->tables[i], &kernel_directory->tables[i], i * 0x1000 * 1024);
			for (uint16_t j = 0; j < 1024; ++j) {
				page_t *  p = &dir->tables[i]->pages[j];
				if (p->frame) {
					debug(" k  0x%x 0x%x %s\n", (i * 1024 + j) * 0x1000, p->frame * 0x1000, p->present ? "[present]" : "");
				}
			}
		} else {
			// We'll implement dumping user-space tables later
		}
	}
	debug(" ---- [done]");
}

void page_fault(registers_t regs)
{
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	uint32_t faulting_address;
	asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

	// The error code gives us details of what happened.
	int present   = !(regs.err_code & 0x1); // Page not present
	int rw = regs.err_code & 0x2;           // Write operation?
	int us = regs.err_code & 0x4;           // Processor was in user-mode?
	int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

	// Output an error message.
	kprintf(
		"Page fault! (%s %s %s %s) at 0x%x (eip=0x%x)\n",
		present == 1 ? "present" : "not-present",
		rw == 1 ? "write" : "read",
		us == 1 ? "user-mode" : "kernel-mode",
		reserved == 1 ? "reserved" : "not-reserved",
		faulting_address,
		regs.eip
	);
	kprintf("   (0x%x)   (p:%d,rw:%d,user:%d,res:%d,id:%d) at 0x%x eip: 0x%x\n", regs.err_code, present, rw, us, reserved, id, faulting_address, regs.eip);
#if 0
	dump_directory(current_directory);
#endif
	PANIC("HALTING");

	while (1) {
		asm volatile("cli");
		asm volatile("hlt");
	}
}

uint32_t virtual_to_physical(uint32_t virtual)
{
	uint32_t remainder = virtual % 0x1000;
	uint32_t frame = virtual / 0x1000;
	uint32_t table = frame / 1024;
	uint32_t subframe = frame % 1024;

	if (current_directory->tables[table]) {
		page_t * p = &current_directory->tables[table]->pages[subframe];
		return p->frame * 0x1000 + remainder;
	} else {
		return 0;
	}
}

void paging_mark_system(uint32_t address)
{
	pmm_bitmap_set(address);
}


void heap_install(void )
{
#if 0
	k_heapSSInit(kheap, (0x10000 + sizeof(KHEAPBLOCKSS)));
#endif
	heap_end = heap_alloc_point;
	debug("HEAP_END IS AT: 0x%x\n", heap_end);
}

void* liballoc_alloc(int num_pages)
{
	ASSERT((num_pages * 0x1000) % 0x1000 == 0, "Kernel requested to expand heap by a non-page-multiple value");
	ASSERT(heap_end % 0x1000 == 0, "Kernel heap is not page-aligned!");
	ASSERT(heap_end + (num_pages * 0x1000) <= KERNEL_HEAP_END, "Kernel tried to allocate %d page(s) beyond the end of it's heap.\n", num_pages);

	uintptr_t addr = heap_end; // Remember the current address, we should return this.
	if (heap_end + (num_pages * 0x1000) > heap_alloc_point) {
		debug("Out of kernel heap space when trying to allocate %d page(s).\nHeap is at: 0x%x - 0x%x (should be 0x%x)\n", num_pages, heap_alloc_point, heap_end, (heap_end + (num_pages * 0x1000)));
		for (uintptr_t i = heap_end; i < heap_end + (num_pages * 0x1000); i += 0x1000) {
		//	debug("\tAllocating frame at 0x%x in directory at 0x%x...\n", i, kernel_directory);
			page_t *page = get_page(i, 0, kernel_directory);
			alloc_frame(page, 1, 1);
		//	debug("\t\tAllocated. Frame points to 0x%x\n", page->frame);
		}
		//debug("Done.\n");
	}

	heap_end += (num_pages * 0x1000);
	memset((void *)addr, 0x0, (num_pages * 0x1000));

	debug("VMM: Heap end is now at 0x%x. New pages allocated at 0x%x\n", heap_end, addr);

	return (void *) addr;
}

int liballoc_free(void* page_start, int num_pages)
{
	return 0; // We'll worry about freeing later.
}

#if 0
void *alloc(uint32_t size)
{
	return k_heapSSAlloc(kheap, size / kheap->bsize);
}

void free(void * p)
{
	return k_heapSSFree(kheap, p);
}
#endif
