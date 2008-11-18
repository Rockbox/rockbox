#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#include "cpu.h"
#define IRAMSIZE 0x50000
#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGINSIZE - CODECSIZE

#define IRAMORIG 0x0
#define DRAMORIG 0x30000000 + STUBOFFSET

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    loadaddress = 0x30000000;

    .text :
    {
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

    _initdata_end =.;

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

    .vectors IRAMORIG:
    {
        _vectors_start = .;
        *(.init.text)
    } > IRAM AT > DRAM

    _vectorscopy = LOADADDR(.vectors);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .iram _iend :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

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

    .codec ENDAUDIOADDR  (NOLOAD) :
    {
        codecbuf = .;
        _codecbuf = .;
    }

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
