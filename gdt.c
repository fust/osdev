#include "gdt.h"
#include "video.h"

extern void gdt_flush(uint32_t);

static void gdt_set_gate(int32_t, uint32_t, uint32_t, uint8_t, uint8_t);

gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt;

void init_gdt()
{
	gdt.limit = (sizeof(gdt_entry_t) * 5) - 1;
	gdt.base = (uint32_t)&gdt_entries;

	gdt_set_gate(0, 0, 0, 0, 0);
	gdt_set_gate(1, 0x00000000, 0xFFFFFFFF, 0x9A, 0xCF);
	gdt_set_gate(2, 0x00000000, 0xFFFFFFFF, 0x92, 0xCF);
	gdt_set_gate(3, 0x00000000, 0xFFFFFFFF, 0xFA, 0xCF);
	gdt_set_gate(4, 0x00000000, 0xFFFFFFFF, 0xF2, 0xCF);

	gdt_flush((uint32_t)&gdt);
}

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;

	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].flags = (limit >> 16) & 0x0F;

	gdt_entries[num].flags |= flags & 0xF0;
	gdt_entries[num].access = access;
}
