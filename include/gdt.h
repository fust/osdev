#ifndef __GDT_H
#define __GDT_H

#include <stdint.h>

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t flags;
	uint8_t base_high;
} __attribute__ ((packed));
typedef struct gdt_entry gdt_entry_t;

struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__ ((packed));
typedef struct gdt_ptr gdt_ptr_t;

extern void init_gdt();

#endif
