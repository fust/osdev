INCLUDES = ./include

CC	= i686-elf-gcc
CFLAGS	= -Wall -Wextra -Wno-unused-variable -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -m32 -std=gnu99 -I $(INCLUDES)
LD	= i686-elf-ld

 
OBJFILES =  asm/task.o task/task.o kheap/kheap.o kmalloc/kmalloc.o stdio/kprintf.o debug/debug.o memory/vmm.o memory/pmm.o timer.o interrupts.o idt.o gdt.o asm/interrupts.o io.o video/video.o stdlib.o string.o boot.o kernel.o
 
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
