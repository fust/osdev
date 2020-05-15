#!/bin/bash

cp kernel.bin isodir/boot/
cp initrd/initrd.img isodir/boot
grub-mkrescue -o os.iso isodir
