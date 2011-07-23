#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx233/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#define DRAMORIG DRAM_ORIG
#define IRAMORIG IRAM_ORIG

#define IRAMSIZE IRAM_SIZE

#define DRAMSIZE (DRAM_SIZE - PLUGINSIZE - CODECSIZE - FRAME_SIZE - TTB_SIZE)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODEC_SIZE)

/* INIT section is the codec buffer */
#define INITSIZE  CODEC_SIZE
#define INITSTART ENDAUDIOADDR

MEMORY
{
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
        *(.text*)
    } > DRAM

    .data :
    {
        *(.data*)
        *(.rodata*)
        _dataend = . ;
    } > DRAM

    .iram :
    {
        _iramstart = .; // always 0
        *(.vectors)
        KEEP(*(.vectors));// otherwise there are no references to it and the linker strip it
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .init ENDAUDIOADDR :
    {
        . = ALIGN(4);
        _initstart = .;
        *(.init)
        _initend = .;
    } AT> DRAM

    _initcopy = LOADADDR(.init);

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
        *(.bss*);
        _end = .;
    } > DRAM

    .audiobuf (NOLOAD) :
    {
        . = ALIGN(4);
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .codec ENDAUDIOADDR (NOLOAD) :
    {
        audiobufend = .;
        _audiobufend = .;
        codecbuf = .;
        _codecbuf = .;
    } > DRAM

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
