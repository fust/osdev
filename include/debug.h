#ifndef __DEBUG__H
#define __DEBUG_H

#include <stdint.h>
#include <elf.h>
#include <multiboot.h>
#include <stdio.h>
#include <cpu.h>
#include <video.h>
#include <mem/paging.h>

#define BOCHS_BREAK() { asm volatile ("xchgw %bx, %bx"); }

extern page_directory_t *current_directory;
extern uint8_t panicing;

struct symbol {
	uint32_t *start;
	uint32_t *end;
	const char *name;
};

struct sym_offset {
	const char *name;
	uint32_t offset;
};

void debug_init(multiboot_elf_section_header_table_t *);

void backtrace_now(void);

void debug(char * format, ...);

void do_backtrace(registers_t * regs);

#define PANIC(format, ...) do { \
	cls(); \
	__asm__ volatile ("cli"); \
	kprintf("PANIC: "); \
	debug("PANIC: "); \
	kprintf(format, ##__VA_ARGS__); \
	kprintf("\n"); \
	debug(format, ##__VA_ARGS__); \
	backtrace_now(); \
	if (panicing == 1) { \
		__asm__ volatile("ud2"); \
	} \
	panicing = 1; \
	/*debug_dump_pgdir(current_directory); */\
	__asm__ volatile("ud2"); \
} while (0)


#define ASSERT(x, format, ...) do { \
	if (!(x)) { \
		kprintf("%s: %s\n", __FILE__, #x); \
		backtrace_now(); \
		PANIC(format, ##__VA_ARGS__); \
	} \
} while (0)

#endif
