#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tcc77x/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG 0x20000000
#define IRAMORIG 0x00000000
#define IRAMSIZE 64K


MEMORY
{
   DRAM : ORIGIN = DRAMORIG + DRAMSIZE - 0x100000, LENGTH = 0x100000
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    .text : {
        *(.init.text)
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
   } > DRAM

    .data : {
        *(.icode)
        *(.irodata)
        *(.idata)
        *(.data*)
        *(.rodata.*)
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM

    .stack :
    {
         *(.stack)
         _stackbegin = .;
         stackbegin = .;
         . += 0x2000;
         _stackend = .;
         stackend = .;
    } > DRAM

    .bss : {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
         _end = .;
     } > DRAM
}
