#include "config.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_jz47xx/crt0.o)

#define DRAMSIZE ((MEMORYSIZE-4) * 0x100000)

#define DRAMORIG 0x80E04000
#define IRAMORIG 0x80000000
#define IRAMSIZE 16K

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    . = DRAMORIG;

    .text :
    {
        _loadaddress = .;
        *(.init.text);
        *(.text*);
    } > DRAM

    . = ALIGN(4);

    .rodata :
    {
        *(.rodata*);
    } > DRAM

    . = ALIGN(4);

    .data :
    {
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
    } > DRAM

    . = ALIGN(4);

    .iram IRAMORIG:
    {
        _iramstart = .;
        *(.vectors.1);
        . = 0x100;
        *(.vectors.2);
        . = 0x180;
        *(.vectors.3);
        . = 0x200;
        *(.vectors.4);
        *(.vectors);

        *(.icode);
        *(.irodata);
        *(.idata);
        KEEP(*(.vectors*))
        _iramend = .;
    } > IRAM AT> DRAM
    _iramcopy = LOADADDR(.iram);

    . = ALIGN(4);

    .bss (NOLOAD):
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss*);
         *(COMMON);
         *(.scommon*);
         _end = .;
    } > DRAM

    _bootend = .;

    .stack (NOLOAD):
    {
         *(.stack)
         stackbegin = .;
         . += 0x2000;
         stackend = .;
    } > IRAM
}
