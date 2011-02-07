#include "config.h"

ENTRY(_start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tms320dm320/crt0.o)

#ifndef LCD_NATIVE_WIDTH
#define LCD_NATIVE_WIDTH LCD_WIDTH
#endif

#ifndef LCD_NATIVE_HEIGHT
#define LCD_NATIVE_HEIGHT LCD_HEIGHT
#endif

#define LCD_FUDGE       LCD_NATIVE_WIDTH%32

#define LCD_BUFFER_SIZE ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)

/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE        0x4000

/* Give this some memory to allow it to align to the MMU boundary.
 * Note that since there are two buffers (YUV/RGB) it calculates the approximate
 * memory needed in steps of 1 Meg.
 */
#define LCD_TTB_AREA    0x100000*((LCD_BUFFER_SIZE>>19)+1)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG        0x00900000

#define FLASHORIG       0x00100000
#define FLASHSIZE       0x00800000

#define ITCMORIG        0x00000000
#define ITCMSIZE        0x4000

#define DTCMORIG        0x00020000
#define DTCMSIZE        0x4000

PRO_STACK_SIZE =  0x2000;
IRQ_STACK_SIZE =  0x600;
FIQ_STACK_SIZE =  0x400;

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  \
    (DRAMORIG + DRAMSIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE - LCD_TTB_AREA)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    ITCM : ORIGIN = ITCMORIG, LENGTH = ITCMSIZE
    DTCM : ORIGIN = DTCMORIG, LENGTH = DTCMSIZE
}

SECTIONS
{
    /* Set up variables needed for memory initialization */
    _sdram_start    = DRAMORIG;
    _sdram_sizem    = ((_endsdram - _sdram_start) / 0x100000);

    _flash_start    = FLASHORIG;
    _flash_sizem    = (FLASHSIZE / 0x100000);

    /* crt0.S initialization */
    .init :
    {
        . = ALIGN(0x4);
        loadaddress = .;
        *(.init)
    } > DRAM

    .vectors ITCMORIG :
    {
        . = ALIGN(0x4);
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } > ITCM AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

    .text :
    {
        . = ALIGN(0x4);
        *(.text*)
    } > DRAM

    /* Thumb interworking sections - for some reason LD dies even if these
     * sections are empty.
     */
    .glue :
    {
       . = ALIGN(0x4);
       *(.glue_7)  /* ARM calling Thumb */
       *(.glue_7t) /* Thumb calling ARM */
    } > DRAM

    .rodata :
    {
        . = ALIGN(0x4);
        *(.rodata*)
    } > DRAM

    .data : 
    {
        . = ALIGN(0x4);
        *(.data*)
    } > DRAM

    .iram :
    {
        . = ALIGN(0x4);
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > ITCM AT> DRAM

    _iramcopy = LOADADDR(.iram);

 	.bss (NOLOAD) :
    {
        . = ALIGN(0x4);
       	_bss_start = .;
        *(.bss*)
        *(COMMON)
       	_bss_end = .;
    } > DRAM

    .ibss (NOLOAD) :
    {
        . = ALIGN(0x4);
        _ibss_start = .;
        *(.ibss)
        _ibss_end = .;
    } > ITCM

    /* Program stack space */
    .pro_stack (NOLOAD): 
    {
        . = ALIGN(0x4);
        *(.stack)
        stackbegin = .;        /* Variable for thread.c */
        _pro_stack_end = .;       
        . += PRO_STACK_SIZE;
        _pro_stack_start = .;
        stackend = .;          /* Variable for tread.c */
    } > ITCM

    /* IRQ stack space */
    .irq_stack (NOLOAD):
    {
        . = ALIGN(0x4);
        _irq_stack_end = .;
        . += IRQ_STACK_SIZE;
        _irq_stack_start = .;
    } > ITCM

    /* FIQ stack space */
    .fiq_stack (NOLOAD):
    {
        . = ALIGN(0x4);
        _fiq_stack_end = .;
        . += FIQ_STACK_SIZE;
        _fiq_stack_start = .;
    } > ITCM

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
    
    _endsdram = .;

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

