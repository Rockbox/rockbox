#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)
MEMORY
{
   DRAM : ORIGIN = 0x09000000, LENGTH = (2 * 0x100000) - 0x8000
   IRAM : ORIGIN = 0x0f000000, LENGTH = 0x1000
   FLASH : ORIGIN = 0x02000000 + ROM_START, LENGTH = 256K - ROM_START
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

    .mp3end (0x09000000 + (2 * 0x100000) - 0x8000):
    {
        _mp3end = .;
    } > DRAM

    .plugin (0x09000000 + (2 * 0x100000) - 0x8000):
    {
        _pluginbuf = .;
    }
}
