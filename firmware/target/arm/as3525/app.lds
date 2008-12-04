#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

#if MEMORYSIZE <= 2
/* we put the codec buffer in IRAM */
#define LOWMEM
#endif

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#include "cpu.h"

#define IRAMSIZE 0x50000

#ifdef LOWMEM
#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGINSIZE
#define CODECORIG (IRAMORIG + IRAMSIZE - CODEC_SIZE)
#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGINSIZE - CODECSIZE
#define CODECORIG (ENDAUDIOADDR)
#endif

#define IRAMORIG 0x0
#define DRAMORIG 0x30000000 + STUBOFFSET

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#ifdef LOWMEM
#define ENDADDR (ENDAUDIOADDR)
#else
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)
#endif

MEMORY
{
#ifdef LOWMEM
    IRAM : ORIGIN = IRAMORIG, LENGTH = (IRAMSIZE - CODEC_SIZE)
    CODEC_IRAM : ORIGIN = CODECORIG, LENGTH = CODEC_SIZE
#else
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
#endif
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    loadaddress = 0x30000000;

    .text :
    {
        _loadaddress = .;
        _textstart = .;
        *(.text)
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

    .vectors IRAMORIG:
    {
        _vectors_start = .;
        *(.init.text)
    } > IRAM AT > DRAM

    _vectorscopy = LOADADDR(.vectors);

    .iram :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM AT> DRAM

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    _iramcopy = LOADADDR(.iram);

    .stack (NOLOAD) :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > DRAM

    .bss (NOLOAD) :
    {
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

    .codec CODECORIG (NOLOAD) :
    {
        codecbuf = .;
        _codecbuf = .;
#ifdef LOWMEM
    } > CODEC_IRAM
#else
    } > DRAM
#endif

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
