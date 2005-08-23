#include "config.h"

ENTRY(start)
#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#else
OUTPUT_FORMAT(elf32-sh)
#endif
INPUT(crt0.o)

#if MEMORYSIZE >= 32
#define PLUGINSIZE 0xC0000
#else
#define PLUGINSIZE 0x8000
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE

#ifdef IRIVER_H100
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#else
#define DRAMORIG 0x09000000
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#define FLASHORIG 0x02000000 + ROM_START
#define FLASHSIZE 256K - ROM_START
#endif

#define ENDADDR  (DRAMORIG + DRAMSIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
   FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}
SECTIONS
{
    .flashheader : 
    {
	/* place flash link address first, so the plugin can check */	
        LONG(FLASHORIG);
        /* followed by the start address, the first vector takes care of this. */
        /* If we ever place the table elsewhere, put a constant here. */
    } > FLASH

    .vectors :
    {
        _datacopy = .;
    } > FLASH

    .data : AT ( _datacopy )
    {
        loadaddress = .;
        _loadaddress = .;
        _datastart = .;
        *(.resetvectors);
        *(.vectors);
        . = ALIGN(0x200);
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
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
        _iramcopy = .;
    } > FLASH

    .iram IRAMORIG : AT ( _iramcopy )
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
       stackbegin = .;
       . += 0x2000;
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

    .audiobuf :
    {
        . = ALIGN(0x4);
        _audiobuffer = .;
    } > DRAM

    .audiobufend ENDADDR:
    {
        _audiobufend = .;
    } > DRAM

    .plugin ENDADDR:
    {
        _pluginbuf = .;
    }
}
