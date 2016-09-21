#include <task.h>
#include <kmalloc.h>
#include <string.h>
#include <vmm.h>
#include <stdio.h>
#include <debug.h>

extern page_directory_t * kernel_directory; // Defined in vmm.c
extern page_directory_t * current_directory;

task_t *current_task=0;
task_t *task_queue=0;

uint8_t current_pid=0;

task_t *find_last_task()
{
	task_t *tmp = task_queue;
	while (tmp->next) {
		tmp = tmp->next;
	}

	return tmp;
}

static void idle_task(void)
{
	__asm__ __volatile__ ("hlt");
}

task_t *idle_task_create()
{
	debug("Allocating kernel idle task\n");
	task_t *task = (task_t *)kmalloc(sizeof(task_t));

	debug("Cloning current directory\n");
	page_directory_t *directory = clone_directory(current_directory);
	debug("Cloned directory. It's at: 0x%x\n", directory->physical_address);

	debug("Setting up task struct\n");
	task->pid = current_pid++;
	debug("Allocating EBP/ESP (stack)\n");
	task->ebp = task->esp = (uintptr_t)kmalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
	debug("Set up rest of task\n");
	task->eip = &idle_task;
	task->page_directory = directory;
	task->next = 0;
	task->running = 0;
	task->wall_time = 0;

	return task;
}

void tasking_install()
{
	asm volatile ("cli");

	debug("Initializing multitasking\n");

	task_t *task = idle_task_create();

	current_task = task_queue = task;

	debug("Switching to idle task directory at 0x%x, EIP=0x%x\n", task->page_directory->physical_address, task->eip);
	//vmm_switch_directory(task->page_directory);

	asm volatile ("sti");
}

task_t *task_create()
{
	task_t *task = (task_t *)kmalloc(sizeof(task_t));

	page_directory_t *directory = clone_directory(kernel_directory);

	task->pid = current_pid++;
	task->ebp = task->esp = (uintptr_t)kmalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
	task->eip = &idle_task;
	task->page_directory = directory;
	task->next = 0;
	task->running = 0;
	task->wall_time = 0;

	task_t *last_task = find_last_task();
	last_task->next = task;

	return task;
}

void task_remove(task_t *task)
{
}

page_directory_t *clone_directory(page_directory_t * src)
{
	uintptr_t phys;

	debug("Allocating new page directory from current (0x%x)\n", src->tables);
	page_directory_t * dir = (page_directory_t *)kmalloc_ap(sizeof(page_directory_t), &phys);

	memset((void*)dir, 0, sizeof(page_directory_t));

	dir->physical_address = phys;
	debug("Got 0x%x (0x%x) phys (0x%x virt) for new directory\nClearing directory\n", phys, dir->physical_address, dir);

	uint32_t i;

	uint32_t sc = 0;
	uint32_t kc = 0;
	uint32_t uc = 0;

	debug("Start copying\n");

	for (i = 0; i < 1024; i++) {
		if (i == 0) {
			debug("\tCopy 0x%x\n", src->tables[i]);
		}
		if (!src->tables[i] || (uintptr_t)src->tables[i] == (uintptr_t)0xFFFFFFFF) {
			sc++;
			continue;
		}
		if (kernel_directory->tables[i] == src->tables[i]) {
			/* Kernel tables are simply linked together */
			dir->tables[i] = src->tables[i];
			dir->physical_tables[i] = src->physical_tables[i];
			kc++;
		} else {
			/* User tables must be cloned */
			uintptr_t phys;
			dir->tables[i] = clone_table(src->tables[i], &phys);
			dir->physical_tables[i] = phys | 0x07;
			uc++;
		}
	}
	debug("Copied %d tables. (%d kernel, %d user, %d skipped) to 0x%x physical\n", (kc + uc), kc, uc, sc, dir->physical_address);
	return dir;
}

page_table_t * clone_table(page_table_t * src, uint32_t * physAddr)
{
	page_table_t * table = (page_table_t *)kmalloc_p(sizeof(page_table_t), physAddr);
	memset(table, 0, sizeof(page_table_t));
	uint32_t i;
	for (i = 0; i < 1024; ++i) {
		/* For each frame in the table... */
		if (!src->pages[i].frame) {
			continue;
		}
		/* Allocate a new frame */
		alloc_frame(&table->pages[i], 0, 0);
		/* Set the correct access bit */
		if (src->pages[i].present)	table->pages[i].present = 1;
		if (src->pages[i].rw)		table->pages[i].rw = 1;
		if (src->pages[i].user)		table->pages[i].user = 1;
		if (src->pages[i].accessed)	table->pages[i].accessed = 1;
		if (src->pages[i].dirty)	table->pages[i].dirty = 1;
		/* Copy the contents of the page from the old table to the new one */
		copy_page_physical(src->pages[i].frame * 0x1000, table->pages[i].frame * 0x1000);
	}
	return table;
}

void switch_task()
{
}
