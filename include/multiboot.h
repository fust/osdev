#include <stdint.h>

#ifndef __MULTIBOOT_H
#define __MULTIBOOT_H

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_AOUT_SYMS                0x00000010
/* is there an ELF section header table? */
#define MULTIBOOT_INFO_ELF_SHDR                 0X00000020

/* The symbol table for a.out. */
 struct multiboot_aout_symbol_table
 {
   uint32_t tabsize;
   uint32_t strsize;
   uint32_t addr;
   uint32_t reserved;
 };
 typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

 /* The section header table for ELF. */
 struct multiboot_elf_section_header_table
 {
   uint32_t num;
   uint32_t size;
   uint32_t addr;
   uint32_t shndx;
 };
 typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;


struct multiboot
{
   uint32_t flags;
   uint32_t mem_lower;
   uint32_t mem_upper;
   uint32_t boot_device;
   uint32_t cmdline;
   uint32_t mods_count;
   uint32_t mods_addr;
   union
   {
	    multiboot_aout_symbol_table_t aout_sym;
	    multiboot_elf_section_header_table_t elf_sec;
   } u;
   uint32_t mmap_length;
   uint32_t mmap_addr;
   uint32_t drives_length;
   uint32_t drives_addr;
   uint32_t config_table;
   uint32_t boot_loader_name;
   uint32_t apm_table;
   uint32_t vbe_control_info;
   uint32_t vbe_mode_info;
   uint32_t vbe_mode;
   uint32_t vbe_interface_seg;
   uint32_t vbe_interface_off;
   uint32_t vbe_interface_len;
}  __attribute__((packed));

typedef struct multiboot_header multiboot_header_t; 

typedef struct {
	uint32_t size;
	uint32_t base_addr_low;
	uint32_t base_addr_high;
	uint32_t length_low;
	uint32_t length_high;
	uint32_t type;
} __attribute__ ((packed)) mboot_memmap_t;

#endif
