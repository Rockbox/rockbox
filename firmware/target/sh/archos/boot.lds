#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
STARTUP(target/sh/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG 0x09000000
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#define FLASHORIG 0x02000000 + ROM_START
#define FLASHSIZE 256K - ROM_START

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
   FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}

SECTIONS
{
    .vectors :
    {
        _datacopy = .;
    } > FLASH

    .data : AT ( _datacopy )
    {
        _datastart = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
        . = ALIGN(0x200);
        *(.icode)
        *(.irodata)
        *(.idata)
        *(.data*)
        . = ALIGN(0x4);
        _dataend = .;
        . = ALIGN(0x10);  /* Maintain proper alignment for .text section */
    } > IRAM

    /* TRICK ALERT! Newer versions of the linker don't allow output sections
       to overlap even if one of them is empty, so advance the location pointer
       "by hand" */
    .text LOADADDR(.data) + SIZEOF(.data) :
    {
        *(.init.text)
        *(.text*)
        . = ALIGN(0x4);
    } > FLASH

    .rodata :
    {
        *(.rodata*)
        . = ALIGN(0x4);
        _iramcopy = .;
    } > FLASH

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       stackbegin = .;
       . += 0x2000;
       _stackend = .;
       stackend = .;
    } > IRAM

    .bss :
    {
       _edata = .;
        *(.ibss)
        *(.bss*)
        *(COMMON)
       _end = .;
    } > IRAM
}
