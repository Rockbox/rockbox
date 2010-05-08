#include "config.h"
#include "imx31l.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx31/crt0.o)

#define DRAMSIZE (1 << 20) /* Limit 1 MB for bootloader */

#define DRAMORIG  0x02000000
/* #define IRAMORIG 0x1FFFC000 */
#define IRAM      DRAM
#define IRAMSIZE  IRAM_SIZE
#define IRAMORIG  DRAMORIG
#define FLASHORIG 0x0000000
#define FLASHSIZE 1M

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   QHARRAY : ORIGIN = QHARRAY_PHYS_ADDR, LENGTH = QHARRAY_SIZE
}

SECTIONS
{
    . = DRAMORIG;

    .text :
    {
        *(.init.text)
        *(.text*)
	*(.icode)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata)  /* problems without this, dunno why */
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    .data :
    {
        *(.irodata)
        *(.idata)
        *(.data*)
        . = ALIGN(0x4);
        *(.sdmacode)
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM

#if 0 /* Unneeded at the moment */
    /* .ncdata section is placed at uncached physical alias address and is
     * loaded at the proper cached virtual address - no copying is
     * performed in the init code */
    .ncdata . + NOCACHE_BASE :
    {
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
    } AT> DRAM
#endif
    
    .vectors 0x0 :
    {
        _vectorsstart = .;
        *(.vectors);
        KEEP(*(.vectors));
        _vectorsend = .;
    } AT > DRAM
    _vectorscopy = LOADADDR(.vectors);

    .stack (NOLOAD) :
    {
        *(.stack)
        _stackbegin = .;
        stackbegin = .;
        . += 0x2000;
        _stackend = .;
        stackend = .;
    } > IRAM

    /* .bss and .ncbss are treated as a single section to use one init loop to
     * zero it - note "_edata" and "_end" */
    .bss (NOLOAD) :
    {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
         . = ALIGN(0x4);
    } > DRAM
    
    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
        *(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
    } AT> DRAM

    .endaddr . - NOCACHE_BASE (NOLOAD) :
    {
        _end = .;
    } > DRAM

    .qharray (NOLOAD) :
    {
	_qharray = .;
	*(.qharray)
	_qharrayend = .;
    } > QHARRAY
}
