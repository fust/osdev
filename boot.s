;
; boot.s -- Kernel start location. Also defines multiboot header.
;           Based on Bran's kernel development tutorial file start.asm
;

MBOOT_PAGE_ALIGN    equ 1<<0    ; Load kernel and modules on a page boundary
MBOOT_MEM_INFO      equ 1<<1    ; Provide your kernel with memory info
MBOOT_VID_MOD       equ 1<<2    ; Information about the video mode table must be available to the kernel
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; Multiboot Magic value
; NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does not
; pass us a symbol table.
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VID_MOD
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)


[BITS 32]                       ; All instructions should be 32-bit.

[GLOBAL mboot]                  ; Make 'mboot' accessible from C.
[SECTION .__mbHeader]
align 4
mboot:
    dd  MBOOT_HEADER_MAGIC      ; GRUB will search for this value on each
                                ; 4-byte boundary in your kernel file
    dd  MBOOT_HEADER_FLAGS      ; How GRUB should load your file / settings
    dd  MBOOT_CHECKSUM          ; To ensure that the above values are correct
    
    dd  mboot                   ; Location of this descriptor
;    dd  code                    ; Start of kernel '.text' (code) section.
;    dd  bss                     ; End of kernel '.data' section.
    dd  end                     ; End of kernel.
    dd  start                   ; Kernel entry point (initial EIP).

section .bootstrap_stack
align 16
stack_bottom:
times 32768 db 0		; 32KB stack
stack_top:

[GLOBAL start]                  ; Kernel entry point.
[EXTERN code]
[EXTERN bss]
[EXTERN end]
[EXTERN kmain]                   ; This is the entry point of our C code

start:
    mov esp, stack_bottom
    and esp, $-16
    ; Load multiboot information:
    push esp
    push ebx

    ; Execute the kernel:
    cli                         ; Disable interrupts.
    call kmain                   ; call our main() function.
    jmp $                       ; Enter an infinite loop, to stop the processor
                                ; executing whatever rubbish is in the memory
                                ; after our kernel!
