ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .vectors 0x09000000 :
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
       _end = .;
       _stack = . + 0x1000;
       _edata = .;
    }

    .pad 0x0900C800 :
    {
        LONG(0);
    }
  }
