#include "config.h"
#include "imx31l.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx31/crt0.o)

#define DRAMSIZE (1 << 20) /* Limit 1 MB for bootloader */

#define DRAMORIG (0x02000000-0x00100000)
/* #define IRAMORIG 0x1FFFC000 */
#define IRAM      DRAM
#define IRAMSIZE  IRAM_SIZE
#define IRAMORIG  DRAMORIG
#define FLASHORIG 0x0000000
#define FLASHSIZE 1M

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   DEVBSS : ORIGIN = 0x80100000 + FRAME_SIZE, LENGTH = 0x100000 - FRAME_SIZE - TTB_SIZE
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
        _dataend = . ;
    } > DRAM
    
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

    .bss (NOLOAD) :
    {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
         _end = .;
    } > DRAM
   
    .devbss (NOLOAD) :
    {
        _devbssdata = .;
    	*(.qharray)
        *(.devbss*)
        _devbssend = .;
    } > DEVBSS
}
