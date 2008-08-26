#include "config.h"
#undef mips

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_jz47xx/crt0.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG 0x80E00000 /* HACK */
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
        loadaddress = .;
        _loadaddress = .;
        *(.init.text);
        *(.text*);
        *(.glue_7);
        *(.glue_7t);
        *(.rel.dyn);
    } > DRAM
    
    .vectors :
    {
        _vectorsstart = .;
        KEEP(*(.vectors))
        *(.vectors);
    } > DRAM
    
    . = ALIGN(4);

    .rodata :
    {
        *(.rodata);  /* problems without this, dunno why */
        *(.rodata*);
        *(.rodata.str1.1);
        *(.rodata.str1.4);
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    .data :
    {
        *(.icode);
        *(.irodata);
        *(.idata);
        *(.data*);
        *(.sdata*);
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM
    
    _gp = ALIGN(16);
    .got :
    {
        *(.got*)
    } > DRAM
    
    . = ALIGN(4);

    .bss :
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss);
         *(COMMON)
         *(.scommon*);
         _end = .;
    } > DRAM
    
    .iram IRAMORIG:
    {
        . = 0x220; /* Vectors take in 0x80000000 -> 0x80000220 */
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM
    
    .stack :
    {
         *(.stack)
         . = ALIGN(0x4);
         _stackbegin = .;
         stackbegin = .;
         . += 0x2000;
         _stackend = .;
         stackend = .;
    } > IRAM
}
