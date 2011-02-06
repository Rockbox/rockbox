#include "config.h"

ENTRY(_vectors)

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

#define LCD_BUFFER_SIZE  ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)

/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE         (0x4000)

/* Give this some memory to allow it to align to the MMU boundary.
 * Note that since there are two buffers (YUV/RGB) it calculates the approximate
 * memory needed in steps of 1 Meg.
 */
#define LCD_TTB_AREA    0x100000*((LCD_BUFFER_SIZE>>19)+1)

/* Bootloader only uses/knows about the upper 32 M */
#define DRAMORIG    0x02900000
#define DRAMSIZE    (MEMORYSIZE * 0x80000)

#define IRAMORIG    0x00000000
#define IRAMSIZE    0x4000

#define FLASHORIG   0x00100000
#define FLASHSIZE   0x00800000

PRO_STACK_SIZE =  0x2000;
IRQ_STACK_SIZE =  0x400;
FIQ_STACK_SIZE =  0x400;

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
    /* Set up variables needed for memory initialization */
    _sdram_start    = DRAMORIG;
    _sdram_sizem    = (DRAMSIZE / 0x100000);

    _flash_start    = FLASHORIG;
    _flash_sizem    = (FLASHSIZE / 0x100000);

    .vectors :
    {
        _vectorsstart = .;
        KEEP(*(.vectors))
        _vectorsend = .;
    } > IRAM AT> FLASH
    _vectorscopy = LOADADDR(.vectors);

    /* crt0.S initialization */
    .init :
    {
        . = ALIGN(0x4);
        _loadaddress = .;
        *(.init)
    } > FLASH

    /* Program code */
    .text :
    {
        . = ALIGN(0x4);
        *(.text*)
    } > FLASH

    /* Thumb interworking sections - for some reason LD dies even if these
     * sections are empty.
     */
    .glue :
    {
       . = ALIGN(0x4);
       *(.glue_7)  /* ARM calling Thumb */
       *(.glue_7t) /* Thumb calling ARM */
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(0x4);
        *(.rodata*)
    } > FLASH

    /* Dynamic data - this needs to be copied out of flash before it is used. */
    .data :
    {
        . = ALIGN(0x4);
        _dramstart = .;
        *(.data*)
        _dramend = .;
    } > DRAM AT> FLASH
    _dramcopy = LOADADDR(.data);

 	.bss (NOLOAD) :
    {
        . = ALIGN(0x4);
       	_bss_start = .;
        *(.bss*)
        *(COMMON)
       	_bss_end = .;
    } > DRAM

    .iram :
    {
        . = ALIGN(0x4);
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > IRAM AT> FLASH

    _iramcopy = LOADADDR(.iram);
    
    .ibss (NOLOAD) :
    {
        . = ALIGN(0x4);
        _ibss_start = .;
        *(.ibss)
        _ibss_end = .;
    } > IRAM

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
    } > IRAM

    /* IRQ stack space */
    .irq_stack (NOLOAD):
    {
        . = ALIGN(0x4);
        _irq_stack_end = .;
        . += IRQ_STACK_SIZE;
        _irq_stack_start = .;
    } > IRAM

    /* FIQ stack space */
    .fiq_stack (NOLOAD):
    {
        . = ALIGN(0x4);
        _fiq_stack_end = .;
        . += FIQ_STACK_SIZE;
        _fiq_stack_start = .;
    } > IRAM
    
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
