ENTRY (start)

SECTIONS
{
    . = 0x00100000;
    
    kernstart = .; _kernstart = .; __kernstart = .;

    .__mbHeader : {
        *(.__mbHeader)
    }

    .text ALIGN (0x1000) :
    {
        *(.text)
    }

    .rodata ALIGN (0x1000) :
    {
        *(.rodata*)
    }

    .data ALIGN (0x1000) :
    {
        *(.data)
    }

    .bss ALIGN (0x1000) :
    {
        sbss = .;
        *(COMMON)
        *(.bss)
	*(.bootstrap_stack)
        ebss = .;
    }

    end = .; _end = .; __end = .;
}
