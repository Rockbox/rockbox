#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/s3c2440/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG  0x00000000
#define IRAMORIG  0x40000000
#define IRAMSIZE  4K
#define FLASHORIG 0x00000000
#define FLASHSIZE 1M

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    .vectors DRAMORIG :
    {
        _vectorstart = .;
        *(.vectors*);
        *(.init.text)
        . = ALIGN(0x4);
    } > DRAM

    .text :
    {
        _textstart = .;
        *(.text)
        *(.text*)
        *(.icode)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > DRAM

    _textcopy = LOADADDR(.text);

    .rodata :
    {
        *(.rodata)  /* problems without this, dunno why */
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        *(.irodata)
        . = ALIGN(0x4);
    } > DRAM

    .data :
    {
        *(.data*)
        *(.idata)
        . = ALIGN(0x4);
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    } 

    _initdata_end =.;

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > DRAM 

    .bss :
    {
       _edata = .;
        *(.bss*)
        *(.ibss)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM
}
