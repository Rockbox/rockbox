#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)

#define PLUGINSIZE 0x8000

#define DRAMORIG 0x09000000
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#define FLASHORIG 0x02000000 + ROM_START
#define FLASHSIZE 256K - ROM_START

#define ENDADDR  (DRAMORIG + DRAMSIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
   FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}
SECTIONS
{
    .vectors :
    {
        _datacopy = .;
    } > FLASH

    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.resetvectors);
        *(.vectors);
        . = _datastart + 0x200;
        *(.data)
        . = ALIGN(0x4);
        _dataend = .;         
        . = ALIGN(0x10);  /* Maintain proper alignment for .text section */
    } > DRAM
    
    /* TRICK ALERT! Newer versions of the linker don't allow output sections
       to overlap even if one of them is empty, so advance the location pointer
       "by hand" */
    .text LOADADDR(.data) + SIZEOF(.data) :
    {
        *(.init.text)
        *(.text)
        . = ALIGN(0x4);
    } > FLASH

    .rodata :
    {
        *(.rodata)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
        _iramcopy = .;
    } > FLASH

    .iram 0xf000000 : AT ( _iramcopy )
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM
  
    .stack :
    {
       *(.stack)
       _stackbegin = .;
       . += 0x2000;
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
        . = ALIGN(0x4);
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
