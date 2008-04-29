#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-m68k)
STARTUP(target/coldfire/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#ifdef IRIVER_H100_SERIES
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#elif defined(IRIVER_H300_SERIES)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x003f0000
#define FLASHSIZE 4M
#endif

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

    .bss DRAMORIG+0x800000:
    {
       _edata = .;
        *(.ibss)
        *(.bss*)
        *(COMMON)
       _end = .;
    } > DRAM
}
