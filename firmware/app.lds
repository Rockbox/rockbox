#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)

#define PLUGINSIZE 0x8000

#ifdef DEBUG
#define DRAMSIZE 0x1f0000 - PLUGINSIZE
#define ORIGADDR 0x09010000

#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE
#define ORIGADDR 0x09000000
#endif
#define ENDADDR  (ORIGADDR + DRAMSIZE)

MEMORY
{
   DRAM : ORIGIN = ORIGADDR, LENGTH = DRAMSIZE
   IRAM : ORIGIN = 0x0f000000, LENGTH = 0x1000
}

SECTIONS
{
    .vectors :
    {
        *(.resetvectors);
        *(.vectors);
    } > DRAM

    .text :
    {
        . = ALIGN(0x200);
        *(.init.text)
        *(.text)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata)
        *(.rodata.str1.4)
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data)
        . = ALIGN(0x4);
        _dataend  = .;
        _iramcopy = .;
    } > DRAM

    .iram 0xf000000 : AT ( _iramcopy)
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM

    /* TRICK ALERT! We want 0x2000 bytes of stack, but we set the section
       size smaller, and allow the stack to grow into the .iram copy */
    .stack ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
    {
       *(.stack)
       _stackbegin = . - SIZEOF(.iram);
       . += 0x2000 - SIZEOF(.iram);
       _stackend = .;
    } > DRAM

    .bss :
    {
       _edata = .;
        *(.bss)
        *(COMMON)
       _end = .;
    } > DRAM

    .mp3buf :
    {
        _mp3buffer = .;
    } > DRAM

    .mp3end ENDADDR:
    {
        _mp3end = .;
    } > DRAM

    .plugin ENDADDR:
    {
        _pluginbuf = .;
    }
}
