INCLUDES = ./include

CC	= i686-elf-gcc
CFLAGS	= -Wall -Wextra -Wno-unused-value -Wno-unused-variable -nostdlib -fno-builtin -fms-extensions -nostartfiles -nodefaultlibs -m32 -std=gnu99 -I $(INCLUDES)
LD	= i686-elf-ld

 
OBJFILES =  console/console.o fs/ramdisk.o fs/vfs.o mem/liballoc/liballoc.o mem/kheap.o mem/paging.o mem/pmm.o sys/bitmap.o mem/kmalloc.o stdio/kprintf.o debug/debug.o timer.o interrupts.o idt.o gdt.o asm/interrupts.o io.o video/video.o stdlib.o string.o boot.o kernel.o
 
all: kernel.img
 
.s.o:
	nasm -f elf -o $@ $<
 
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<
 
kernel.bin: $(OBJFILES)
	$(LD) -T linker.ld -o $@ $^
 
kernel.img: kernel.bin
	./update_image.sh
 
clean:
	$(RM) $(OBJFILES) kernel.bin
 
install:
	$(RM) $(OBJFILES) kernel.bin
