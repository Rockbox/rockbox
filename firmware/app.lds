#include "config.h"

ENTRY(start)
#if CONFIG_CPU == MCF5249
OUTPUT_FORMAT(elf32-m68k)
#elif CONFIG_CPU == TCC730
OUTPUT_FORMAT(elf32-calmrisc16)
#else
OUTPUT_FORMAT(elf32-sh)
#endif
INPUT(crt0.o)

#define PLUGINSIZE 0x8000

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET

#ifdef IRIVER_H100
#define DRAMORIG 0x30000000 + STUBOFFSET
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#endif

#define ENDADDR  (DRAMORIG + DRAMSIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
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
        *(.rodata.str1.1)
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

    .iram IRAMORIG : AT ( _iramcopy)
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
       stackbegin = . - SIZEOF(.iram);
       . += 0x2000 - SIZEOF(.iram);
       _stackend = .;
       stackend = .;
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
        mp3buffer = .;
    } > DRAM

    .mp3end ENDADDR:
    {
        mp3end = .;
        _mp3end = .;
    } > DRAM

    .plugin ENDADDR:
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
