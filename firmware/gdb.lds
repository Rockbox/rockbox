ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o system.o)
SECTIONS
{
    .vectors 0x09010000 :
    {
        *(.vectors);
        . = ALIGN(0x200);
        *(.init.text)
    }

    .text :
    {
        *(.text)
    }

    .data :
    {
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
       _stack = . + 0x8000;
    }
}
