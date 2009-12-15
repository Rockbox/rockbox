#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tms320dm320/crt0.o)

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#ifndef LCD_NATIVE_WIDTH
#define LCD_NATIVE_WIDTH LCD_WIDTH
#endif

#ifndef LCD_NATIVE_HEIGHT
#define LCD_NATIVE_HEIGHT LCD_HEIGHT
#endif

#define LCD_FUDGE       LCD_NATIVE_WIDTH%32

#define LCD_BUFFER_SIZE  ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)

/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE         (0x4000)

/* Give this some memory to allow it to align to the MMU boundary.
 * Note that since there are two buffers (YUV/RGB) it calculates the approximate
 * memory needed in steps of 1 Meg.
 */
#define LCD_TTB_AREA    0x100000*((LCD_BUFFER_SIZE>>19)+1)

#define BOOT_OFFSET     0x1000000

#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - BOOT_OFFSET

#define DRAMORIG    (0x00900000+BOOT_OFFSET) /* actually it's 0x00900000 */
#define IRAMORIG    0x00000000
#define IRAMSIZE    0x4000
#define FLASHORIG   0x00100000
#define FLASHSIZE   8M

/* End of the audio buffer, where the codec buffer starts */
#define TTB_BEGIN   (DRAMORIG + DRAMSIZE - LCD_TTB_AREA)

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
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    .data : {
        *(.data*)
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM

    .vectors IRAMORIG :
    {
        _vectorsstart = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
        _vectorsend = .;
    } > IRAM AT > DRAM
    _vectorscopy = LOADADDR(.vectors);
    
    .iram :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata*)
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

    .stack (NOLOAD) :
    {
         *(.stack)
         stackbegin = .;
         . += 0x2000;
         stackend = .;
    } > IRAM
    
    .irqstack (NOLOAD) :
    {
       *(.stack)
       . += 0x400;
       irq_stack = .;
    } > IRAM
    
    .fiqstack (NOLOAD) :
    {
       *(.stack)
       . += 0x100;
       fiq_stack = .;
    } > IRAM
    
 	.bss (NOLOAD) :
    {
    	. = ADDR(.data) + SIZEOF(.data) + SIZEOF(.vectors) + SIZEOF(.iram);
       	_edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       	_end = .;
    } > DRAM
    
    .ttbtable TTB_BEGIN (NOLOAD) :
    {
        . = ALIGN (0x4000);
        _ttbstart = .;
        . += TTB_SIZE;
    } > DRAM
    
    /* The LCD buffer should be at the end of memory to protect against
     * overflowing something else when the YUV blitter is fudging the screen 
     * size.
     */
    
    .lcdbuffer (NOLOAD) :
    {
        _lcdbuf = .;
        . += LCD_BUFFER_SIZE;
    } > DRAM
    
    .lcdbuffer2 (NOLOAD) :
    {
        _lcdbuf2 = .;
        . += LCD_BUFFER_SIZE;
    } > DRAM
}
