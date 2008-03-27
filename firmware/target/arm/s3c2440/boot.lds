#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
INPUT(target/arm/s3c2440/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG 0x30000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 4K
#define FLASHORIG 0x0000000
#define FLASHSIZE 1M

SECTIONS
{
    . = DRAMORIG + 0x1000000;

    .text : {
        *(.init.text)
        *(.text*)
    }

    .data : {
        *(.icode)
        *(.irodata)
        *(.idata)
        *(.data*)
        . = ALIGN(0x4);
        _dataend = . ;
     }

    .stack :
    {
         *(.stack)
         _stackbegin = .;
         stackbegin = .;
         . += 0x2000;
         _stackend = .;
         stackend = .;
    }
    .bss : {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
         _end = .;
     }
}
