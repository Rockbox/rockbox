#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tms320dm320/crt0.o)

#define LCD_BUFFER_SIZE  (LCD_WIDTH*LCD_HEIGHT*2)

/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE         (0x4000)

#define DRAMSIZE (MEMORYSIZE * 0x100000) - TTB_SIZE - LCD_BUFFER_SIZE

#define DRAMORIG 0x01900000 /* actually it's 0x00900000 */
#define IRAMORIG 0x00000000
#define IRAMSIZE 16K
#define FLASHORIG 0x00100000
#define FLASHSIZE 8M

/* Now we have the LCD buffer */
#define LCDBEGIN (DRAMSIZE+0x00900000)

/* Finally the TTB table */
#define TTBBEGIN (LCDBEGIN + LCD_BUFFER_SIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
   FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}

SECTIONS
{
    . = DRAMORIG;

    .text : {
        loadaddress = .;
        _loadaddress = .;
        *(.init.text)
        *(.icode)
        *(.text*)
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
        *(.irodata)
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    .data : {
        *(.idata)
        *(.data*)
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

    .bss :
    {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
         _end = .;
    } > DRAM

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
    
    .lcdbuffer LCDBEGIN (NOLOAD) :
    {
        _lcdbuf = .;
    }
    
    .ttbtable TTBBEGIN (NOLOAD) :
    {
        _ttbstart = .;
    }
}
