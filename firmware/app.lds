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
#ifdef NEO_MEMORY
   ROM : ORIGIN = 0x00020000, LENGTH = 0x1E0000
   SRAM : ORIGIN = 0x0C000000, LENGTH = 0x40000
#else
   DRAM : ORIGIN = ORIGADDR, LENGTH = DRAMSIZE
#endif
   IRAM : ORIGIN = 0x0f000000, LENGTH = 0x1000
}

SECTIONS
{
#ifdef NEO_MEMORY
	.bss :
	{
		_edata = .;
		*(.bss)
		*(COMMON)
		_end = .;
	} > SRAM

	.text :
	{
		. = ALIGN(0x4);
		*(.vectors);
		. = ALIGN(0x200);
		*(.init.text)
		*(.text)
	} > ROM

	.rodata :
	{
		*(.rodata)
		*(.rodata.str1.4)
		. = ALIGN(0x4);
	} > ROM

	.data : AT ( LOADADDR(.romdata) )
	{
                _datastart = .;
		*(.data)
                _dataend = .;
	} > SRAM

	.iram : AT ( _iramcopy )
	{
		_iramstart = .;
		*(.icode)
		*(.idata)
		_iramend = .;
	} > IRAM

        .romdata :
        {
                _datacopy = .;
                . += SIZEOF(.data);
		_iramcopy = .;
		. += SIZEOF(.iram);
        } > ROM

	.stack :
	{
                _stackbegin = .;
		*(.stack)
		. += 0x1000;
               _stackend = .;
	} > SRAM
	
	.mp3buf :
	{
		_mp3buffer = .;
	} > SRAM

	.mp3end 0x0C040000 - 0x300:
	{
		_mp3end = .;
	} > SRAM

	.plugin 0x0C040000:
	{
		_pluginbuf = .;
	} > SRAM
#else
    .vectors :
    {
        *(.resetvectors);
        *(.vectors);
        . = ALIGN(0x200);
        *(.init.text)
    } > DRAM

    .text :
    {
        *(.text)
        . = ALIGN(0x4);
    } > DRAM

    .data :
    {
        *(.data)
        . = ALIGN(0x4);
        _datacopy = .; /* dummy here, only for ROM based */
        _datastart = .;
        _dataend  = .;
    } > DRAM

    .rodata :
    {
        *(.rodata)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > DRAM

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       /* We put the copy of the .iram section here to save space */
       _iramcopy = .;
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

#ifdef DEBUG
    .heap :
    {
        _poolstart = .;
        . = 0x20000;
        _poolend = .;
    } > DRAM
#endif

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

    .iram 0xf000000 : AT ( _iramcopy )
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM
#endif
}
