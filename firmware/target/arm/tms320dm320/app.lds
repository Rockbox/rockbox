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
#define TTB_SIZE        0x4000

/* Give this some memory to allow it to align to the MMU boundary.
 * Note that since there are two buffers (YUV/RGB) it calculates the approximate
 * memory needed in steps of 1 Meg.
 */
#define LCD_TTB_AREA    0x100000*((LCD_BUFFER_SIZE>>19)+1)

#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET

#define DRAMORIG        0x00900000 + STUBOFFSET
#define IRAMORIG        0x00000000
#define IRAMSIZE        0x4000

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE - LCD_TTB_AREA)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    .text :
    {
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
        . = ALIGN(0x4);
    } > DRAM

    .data : 
    {
        *(.data*)
        . = ALIGN(0x4);
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .vectors IRAMORIG :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } > IRAM AT> DRAM

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
       . += 0x400;
       fiq_stack = .;
    } > IRAM
    
    /* This overwrites the iram (in ram), so make sure that the iram is copied 
     *  out in crt0.s before the bss section and the rest are used.
     */
    .bss (NOLOAD) :
    {
    	. = ADDR(.data) + SIZEOF(.data);
       	_edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       	_end = .;
    } > DRAM

    .audiobuf (NOLOAD) :
    {
    	. = ALIGN(4);
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR (NOLOAD) :
    {
        audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR (NOLOAD) :
    {
        codecbuf = .;
        . += CODEC_SIZE;
    } > DRAM

    .plugin (NOLOAD) :
    {
        pluginbuf = .;
        . += PLUGIN_BUFFER_SIZE;
    } > DRAM
    
    .ttbtable (NOLOAD) :
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

