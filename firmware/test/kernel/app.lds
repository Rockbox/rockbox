ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .text 0x09010000 :
    {
	*(.vectors)
        . = ALIGN(0x200);
        *(.init.text)
    }

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.rodata)
        *(.data)
    }

    .rodata :
    {
        *(.rodata)
    }

    .bss :
    {
       _edata = .;
        *(.bss)
        *(COMMON)
       _end = .;
       _stack = . + 0x80000;
    }
}
