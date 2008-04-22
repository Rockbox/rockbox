#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
INPUT(target/arm/s3c2440/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#include "cpu.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGINSIZE - CODECSIZE - LCD_BUFFER_SIZE - TTB_SIZE

#define DRAMORIG 0x00000000 + STUBOFFSET

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    .vectors DRAMORIG :
    {
        _vectorstart = .;
        *(.vectors*);
        *(.init.text)
        . = ALIGN(0x4);
    } > DRAM

    .text :
    {
        _textstart = .;
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > DRAM

    _textcopy = LOADADDR(.text);

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

    .iram :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > DRAM

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > DRAM

    .bss :
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

    .audiobuf ALIGN(4) :
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR:
    {
        audiobufend = .;
        _audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR:
    {
        codecbuf = .;
        _codecbuf = .;
    }

    .plugin ENDADDR:
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
