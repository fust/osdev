INCLUDES = ./include

CC	= i686-elf-gcc
CFLAGS	= -Wall -Wextra -Wno-unused-value -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -nostdlib -fno-builtin -fms-extensions -nostartfiles -nodefaultlibs -m32 -std=gnu99 -I $(INCLUDES)
LD	= i686-elf-ld

OBJFILES = $(patsubst %.c,%.o,$(wildcard ./*.c))
OBJFILES += $(patsubst %.c,%.o,$(wildcard ./*/*.c))
OBJFILES += $(patsubst %.c,%.o,$(wildcard ./*/*/*.c))
OBJFILES += $(patsubst %.s,%.o,$(wildcard ./*.s))
OBJFILES += $(patsubst %.s,%.o,$(wildcard ./*/*.s))
#OBJFILES =  console/console.o dev/ata.o dev/pci.o ds/tree.o ds/list.o ds/hashtable.o dev/kbd.o dev/ps2.o sys/pipe.o fs/ramdisk.o fs/vfs.o mem/liballoc/liballoc.o mem/kheap.o mem/paging.o mem/pmm.o sys/bitmap.o mem/kmalloc.o stdio/kprintf.o debug/debug.o timer.o interrupts.o idt.o gdt.o asm/interrupts.o io.o video/video.o stdlib.o string.o boot.o kernel.o

all: kernel.img

.s.o:
	nasm -f elf -o $@ $<

.c.o:
	@$(CC) $(CFLAGS) -o $@ -c $<

kernel.bin: $(OBJFILES)
	$(LD) -T linker.ld -o $@ $^

kernel.img: kernel.bin
	./update_image.sh

clean:
	$(RM) $(OBJFILES) kernel.bin

install:
	$(RM) $(OBJFILES) kernel.bin
