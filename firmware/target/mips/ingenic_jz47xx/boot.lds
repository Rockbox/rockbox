#include "config.h"
#undef mips

OUTPUT_FORMAT("elf32-tradlittlemips")
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

    .text : {
        loadaddress = .;
        _loadaddress = .;
        *(.init.text);
        *(.text*);
        *(.glue_7);
        *(.glue_7t);
        . = ALIGN(0x4);
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
    
    . = ALIGN(4);

    .data : {
        *(.icode);
        *(.irodata);
        *(.idata);
        *(.data*);
        *(.scommon*);
        *(.sdata*);
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM
    
    . = ALIGN(4);
    
    _gp = ALIGN(16);
    .got : {
        *(.got*)
    }> DRAM
    
    . = ALIGN(4);

    .stack :
    {
         *(.stack)
         _stackbegin = .;
         stackbegin = .;
         . += 0x2000;
         _stackend = .;
         stackend = .;
    } > DRAM
    
    . = ALIGN(4);

    .bss :
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss);
         *(COMMON)
         _end = .;
    } > DRAM
    
    . = ALIGN(4);

    .vectors IRAMORIG :
    {
        _vectorsstart = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
        _vectorsend = .;
    } AT > DRAM
    _vectorscopy = LOADADDR(.vectors);
    
    . = ALIGN(4);
}
