ENTRY(_start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .vectors 0x09000000 :
    {
        *(.vectors);
        . = ALIGN(0x200);
        *(.text.start)
        *(.text)
        *(.rodata)
    }

    .bss :
    {
       _stack = . + 0x1000;
    }

    .pad 0x0900C800 :
    {
        LONG(0);
    }
  }
