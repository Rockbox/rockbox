ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .text 0x09018000 :
    {
        *(.rodata)
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
       _end = . + 0x8000;
       _stack = . + 0x9000;
       _edata = .;
    }
}
