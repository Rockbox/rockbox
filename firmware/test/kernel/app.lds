ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .text 0x09010000 :
    {
	*(.vectors)
        . = ALIGN(0x200);
        crt0.o(.text)
        *(.rodata)
    }

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
       _end = .;
       _stack = . + 0x1000;
       _edata = .;
    }
}
