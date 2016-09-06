#ifndef __ELF_H
#define __ELF_H

#include <stdint.h>

typedef uint16_t Elf32_Half;

typedef uint32_t Elf32_Addr;

typedef uint32_t Elf32_Word;

typedef uint32_t Elf32_Off;

#define ELF_NIDENT	16

#define SHN_UNDEF	0 // Undefined/Not present

#define	SHT_SYMTAB		2	/* symbol table section */
#define	SHT_STRTAB		3	/* string table section */

/* Symbol type - ELFNN_ST_TYPE - st_info */
#define	STT_NOTYPE	0	/* Unspecified type. */
#define	STT_OBJECT	1	/* Data object. */
#define	STT_FUNC	2	/* Function. */
#define	STT_SECTION	3	/* Section. */
#define	STT_FILE	4	/* Source file. */
#define	STT_COMMON	5	/* Uninitialized common block. */
#define	STT_TLS		6	/* TLS object. */
#define	STT_NUM		7
#define	STT_LOOS	10	/* Reserved range for operating system */
#define	STT_HIOS	12	/*   specific semantics. */
#define	STT_LOPROC	13	/* reserved range for processor */
#define	STT_HIPROC	15	/*   specific semantics. */

#define ELF32_ST_TYPE(i)   ((i)&0xf)

typedef struct {
	uint8_t		e_ident[ELF_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct
{
	Elf32_Word	st_name;		/* Symbol name (string tbl index) */
	Elf32_Addr	st_value;		/* Symbol value */
	Elf32_Word	st_size;		/* Symbol size */
	unsigned char	st_info;		/* Symbol type and binding */
	unsigned char	st_other;		/* No defined meaning, 0 */
	Elf32_Half	st_shndx;		/* Section index */
} Elf32_Sym;

#endif
