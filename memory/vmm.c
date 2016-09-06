#include <vmm.h>
#include <pmm.h>
#include <idt.h>
#include <stdio.h>
#include <kheap.h>
#include <task.h>
#include <debug.h>
#include <video.h>

#define KERNEL_HEAP_INIT 0x00800000
#define KERNEL_HEAP_END  0x20000000

// The kernel's page directory
page_directory_t *kernel_directory=0;

// The current page directory;
page_directory_t *current_directory=0;

extern uint32_t placement_pointer; // Defined in kmalloc.c

KHEAPSS *kheap;
uint32_t heap_end = 0;

void alloc_frame(page_t *page, int is_kernel, int is_writable)
{
	if (page->frame != 0) {
		return; // Already allocated
	}
	else
	{
		uint32_t index = pmm_first_free();
		pmm_bitmap_set(index);

		page->present = 1;
		page->rw = (is_writable) ? 1 : 0;
		page->user = (is_kernel) ? 0 : 1;
		page->frame = index;
	}
}

void alloc_dma_frame(page_t *page, int is_kernel, int is_writable, uint32_t address)
{
	page->present = 1;
	page->rw = (is_writable) ? 1 : 0;
	page->user = (is_kernel) ? 0 : 1;
	page->frame = address / 0x1000;
	pmm_bitmap_set(page->frame);
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
		page->frame = 0x0;
		pmm_bitmap_unset(frame);
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
		dir->physical_tables[index] = tmp | 0x7; // PRESENT, RW, US
		return &dir->tables[index]->pages[address % 1024];
	} else {
		return 0;
	}
}

void vmm_init_paging()
{
	kernel_directory = (page_directory_t*) kmalloc_a(sizeof(page_directory_t));
	memset(kernel_directory, 0, sizeof(page_directory_t));
}

vmm_paging_activate() {
	int i = 0;
	debug("Mapping kernel memory\n");
	while (i < placement_pointer)
	{
		alloc_dma_frame(get_page(i, 2, kernel_directory), 1, 1, i);
		i += 0x1000; // 4096, AKA 4KB, AKA page size
	}

	debug("Mapping VGA buffer\n");
	// Identity map VGA buffer
	for (uint32_t j = 0xb8000; j < 0xc0000; j += 0x1000) {
		alloc_dma_frame(get_page(j, 0, kernel_directory), 0, 1, j);
	}

	debug("Registering page fault handler\n");
	register_interrupt_handler(14, page_fault);
	kernel_directory->physical_address = (uint32_t) kernel_directory->physical_tables;

	uint32_t kheap_pointer_temp;

	debug("Fixing heap pointer\n");
	if (KERNEL_HEAP_INIT < placement_pointer + 0x3000) {
		kheap_pointer_temp = placement_pointer + 0x100000;
	} else {
		kheap_pointer_temp = KERNEL_HEAP_INIT;
	}

	debug("Mapping heap memory\n");
	// Heap space
	for (uint32_t i = placement_pointer + 0x3000; i < kheap_pointer_temp; i += 0x1000) {
		alloc_frame(get_page(i, 1, kernel_directory), 1, 0);
	}

	debug("Preallocating entire heap\n");
	/* And preallocate the page entries for all the rest of the kernel heap as well */
	for (uint32_t i = kheap_pointer_temp; i < KERNEL_HEAP_END; i += 0x1000) {
		get_page(i, 1, kernel_directory);
	}

	debug("Cloning current directory\n");
	current_directory = clone_directory(kernel_directory);
	debug("Switching page directory\n");
	vmm_switch_directory(current_directory);
	debug("Switched page tables\n");

	kheap = (KHEAPSS *) kheap_pointer_temp;
}

void vmm_switch_directory(page_directory_t *new)
{
	current_directory = new;

	asm volatile("mov %0, %%cr3":: "r"(new->physical_address));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
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
		"Page fault! (%s %s %s %s) at 0x%x\n",
		present ? "present" : "not-present",
		rw ? "read-only" : "read-write",
		us ? "user-mode" : "kernel-mode",
		reserved ? "reserved" : "not-reserved",
		faulting_address
	);

	do_backtrace(&regs);

	while (1) {
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
	k_heapSSInit(&kheap, kheap - KERNEL_HEAP_END);
	heap_end = kheap;
}

void *alloc(uint32_t size)
{
	return k_heapSSAlloc(&kheap, size);
}

void free(void * p)
{
	return k_heapSSFree(&kheap, p);
}
