#include "config.h"

ENTRY(start)

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
STARTUP(target/coldfire/crt0.o)
#elif defined(CPU_PP)
OUTPUT_FORMAT(elf32-littlearm)
STARTUP(target/arm/crt0-pp.o)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
STARTUP(target/arm/crt0.o)
#elif CONFIG_CPU == SH7034
OUTPUT_FORMAT(elf32-sh)
STARTUP(target/sh/crt0.o)
#else
OUTPUT_FORMAT(elf32-sh)
STARTUP(crt0.o)
#endif


#if MEMORYSIZE >= 32
#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#else
#define PLUGINSIZE 0x8000
#endif


#ifdef IRIVER_H100_SERIES
#define CODECSIZE CODEC_SIZE
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0xc000
#define FLASHORIG 0x00100028
#define FLASHSIZE 0x000eff80
#else
#define DRAMORIG 0x09000000
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#define FLASHORIG 0x02000000 + ROM_START
#define FLASHSIZE 256K - ROM_START
#endif

#ifdef CODECSIZE
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - CODECSIZE
/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)
#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE
/* Where the audio buffer ends, and the plugin buffer starts */
#define ENDADDR ENDAUDIOADDR
#endif

/* End of the audio buffer, where the codec/plugin buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

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
    
    /DISCARD/ : 
    {
        *(.eh_frame)
    }

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
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > IRAM

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       stackbegin = .;
       . += 0x2000;
       _stackend = .;
       stackend = .;
#if IRAMSIZE > 0x1000
    } > IRAM
#else
    } > DRAM
#endif

#ifdef CPU_COLDFIRE
    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
#else
    .bss :
#endif
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

    .audiobuf ALIGN(4):
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

#ifdef CODECSIZE
    .audiobufend ENDAUDIOADDR:
#else
    .audiobufend ENDADDR:
#endif
    {
        _audiobufend = .;
        audiobufend = .;
    } > DRAM

#ifdef CODECSIZE
    .codec ENDAUDIOADDR:
    {
        codecbuf = .;
        _codecbuf = .;
    }
#endif
    
    .plugin ENDADDR:
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
