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

#define LCD_FUDGE       LCD_NATIVE_WIDTH%32

#define LCD_BUFFER_SIZE  ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)

/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE        0x4000

/* Give this 1 meg to allow it to align to the MMU boundary */
#define LCD_TTB_AREA    0x100000

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
        *(.rodata.str1.1)
        *(.rodata.str1.4)
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
    
    /* This could probably be shortened so that the audio buffer overwrites
     * at the IRAM stuff (assuming that it is copied first in crt0.S), but
     * leave it for now since the space is not critical at the moment.
     */
 	.bss (NOLOAD) :
    {
    	. = ADDR(.data) + SIZEOF(.data) + SIZEOF(.vectors) + SIZEOF(.iram);
       	_edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       	_end = .;
    } > DRAM
    

    .audiobuf (NOLOAD) :
    {
    	. = ALIGN(4);
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR (NOLOAD) :
    {
        audiobufend = .;
        _audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR (NOLOAD) :
    {
        codecbuf = .;
        _codecbuf = .;
        . += CODEC_SIZE;
    } > DRAM

    .plugin (NOLOAD) :
    {
        _pluginbuf = .;
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
}

