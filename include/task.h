#ifndef __TASK_H
#define __TASK_H

#include <vmm.h>
#include <stdint.h>

#define KERNEL_STACK_SIZE 0x4000

typedef struct task {
	uint8_t pid;
	uint32_t esp, ebp;
	uint32_t eip;
	page_directory_t *page_directory;
	struct task *next;
	uint8_t running;
	uint8_t wall_time;
} task_t;

task_t *task_create();

void task_remove(task_t *task);

page_directory_t *clone_directory(page_directory_t * src);

page_table_t * clone_table(page_table_t * src, uint32_t * physAddr);

void switch_task();

extern void copy_page_physical(uint32_t, uint32_t);

extern uint32_t read_eip();

#endif
