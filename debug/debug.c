#include "debug.h"
#include "stdio.h"
#include "stddef.h"
#include <cpu.h>
#include <video.h>
#include <bochs/bochs.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "mem/kmalloc.h"

uint8_t panicing = 0;

static struct {
	struct symbol *symbols;
	int count;
} symbol_table;

static void get_elf32_symbols_mboot(uint32_t section_headers, Elf32_Half shnum, Elf32_Half shstrndx)
{
	Elf32_Shdr *sh_table;
	char *strings = NULL;
	Elf32_Sym *syms = NULL;
	int elf_sym_count = 0;
	int function_syms = 0;
	int i;
	uint32_t strtable_size = 0;

	sh_table = (Elf32_Shdr *) section_headers;

	for (i = 0; i < shnum; i++) {
		if (i == shstrndx)
			continue;

		if (sh_table[i].sh_type == SHT_STRTAB) {
			ASSERT(strings == NULL, "Can't handle multiple string tables\n");

			if (sh_table[i].sh_addr == 0)
				continue;

			strings = (char *) kmalloc(sh_table[i].sh_size);

			if (!strings) {
				kprintf("Warning: Failed to allocate memory for strings\n");
				return;
			}

			strtable_size = sh_table[i].sh_size;

			memcpy(strings, (Elf32_Addr *) sh_table[i].sh_addr, sh_table[i].sh_size);
		}

		if (sh_table[i].sh_type == SHT_SYMTAB) {
			int j;

			ASSERT(syms == NULL, "Can't handle multiple symbol tables\n");

			if (sh_table[i].sh_addr == 0)
				continue;

			syms = (Elf32_Sym *)sh_table[i].sh_addr;
			elf_sym_count = sh_table[i].sh_size / sizeof(Elf32_Sym);

			for (j = 0; j < elf_sym_count; j++) {
				Elf32_Sym *sym = &syms[j];
				if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_size && sym->st_name) {
					function_syms++;
				}
			}
		}
	}

	if (!strings) {
		kprintf("Warning: No string section in ELF\n");
	}

	if (!syms) {
		kprintf("Warning: No symbol section in ELF\n");
		kfree(strings);
	}

	if (!strings || !syms)
		return;

	symbol_table.symbols = (struct symbol *) kmalloc(sizeof(struct symbol) * function_syms);
	if (!symbol_table.symbols) {
		kprintf("Warning: Failed to allocate space for symbols\n");
		kfree(strings);
		return;
	}

	/* Store count for walking the array later */
	symbol_table.count = function_syms;
	int f = 0;
	for (i = 0; i < elf_sym_count; i++) {
		Elf32_Sym *sym = &syms[i];
		if (ELF32_ST_TYPE(sym->st_info) != STT_FUNC || !sym->st_size || sym->st_name > strtable_size)
			continue;

		symbol_table.symbols[--function_syms].start = (uint32_t *)sym->st_value;
		symbol_table.symbols[function_syms].end = (uint32_t *)(sym->st_value + sym->st_size);
		symbol_table.symbols[function_syms].name = &strings[sym->st_name];
	}

	if (function_syms) {
		debug("Warning: Leftover %d symbols, space wasted\n", function_syms);
	}
	/* Adjust the pointer if there are leftovers */
	symbol_table.count -= function_syms;
	symbol_table.symbols = &symbol_table.symbols[function_syms];

	debug("Debugger initialized. Loaded %d kernel symbols\n", symbol_table.count);
}

void get_elf_symbols_mboot(multiboot_elf_section_header_table_t *mboot_elf)
{
	get_elf32_symbols_mboot(mboot_elf->addr, mboot_elf->num, mboot_elf->shndx);
}

void debug_init(multiboot_elf_section_header_table_t * elf_header)
{
	debug("Kernel symbols present: %x\nDebugger initializing.", elf_header->addr);
	get_elf_symbols_mboot(elf_header);
}

/* Check if the address exists in the given symbol. Returns the offset from the beginning if so */
static inline uint32_t __symbol_offset(uint32_t *addr, struct symbol *sym)
{
	if (addr >= sym->end)
		return 0;

	if (addr < sym->start)
		return 0;

	return addr - sym->start;
}

struct sym_offset get_symbol(uint32_t *addr)
{
	struct symbol *sym;
	int i;

	for (i = 0; i < symbol_table.count; i++) {
		sym = &symbol_table.symbols[i];
		uint32_t offset = __symbol_offset(addr, sym);
		if (offset)
			return (struct sym_offset){sym->name, offset};
	}

	return (struct sym_offset){"????", 0};
}

void backtrace(void *fp)
{
	do {
		uint32_t prev_ebp = *((uint32_t *)fp);
		uint32_t prev_ip = *((uint32_t *)(fp + 4));
		struct sym_offset sym_offset = get_symbol((void *)prev_ip);
		kprintf("\t%s (+0x%x) [0x%x]\n", sym_offset.name, sym_offset.offset, prev_ip);
		debug("\t%s (+0x%x) [0x%x]\n", sym_offset.name, sym_offset.offset, prev_ip);
		fp = (void *) prev_ebp;
		/*  Stop if ebp is not in the kernel */
		if (fp <= (void *)0x00100000)// || fp >= kernel_end)
			break;
	} while(1);
}

void backtrace_now(void)
{
	uint32_t fp;

	__asm__ __volatile__ ("movl %%ebp, %0" : "=r" (fp));

	kprintf("==== Backtrace:\n");
	debug("==== Backtrace:\n");

	backtrace((void *) fp);
}

void do_backtrace(registers_t * regs)
{
	backtrace((void *) regs->ebp);
}

void debug(char * format, ...)
{
	va_list argptr;
	char s[34], c, *str;
	int num;

	va_start(argptr, format);

	while (*format != '\0') {
		if (*format != '%') {
			BochsConsolePrintChar(*format);
		} else {
			*format++;
			switch(*format) {
				case '%':
					BochsConsolePrintChar('%');
					break;
				case 'b':
					num = va_arg(argptr, int);
					itoa(num, s, 2);
					debug(s);
					break;
				case 'c':
					c = va_arg(argptr, int); // Yes, it *should* be char but them the compiler complains.
					BochsConsolePrintChar(c);
					break;
				case 's':
					str = va_arg(argptr, char *);
					debug(str);
					break;
				case 'x':
					num = va_arg(argptr, int);
					itoa(num, s, 16);
					debug(s);
					break;
				case 'd':
					num = va_arg(argptr, int);
					itoa(num, s, 10);
					debug(s);
					break;
				default:
					BochsConsolePrintChar(--*format);
					BochsConsolePrintChar(++*format);
					break;
			}
		}
		*format++;
	}

	va_end(argptr);
}
