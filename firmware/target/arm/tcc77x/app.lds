#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tcc77x/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#include "imx31l.h"

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - CODECSIZE

#define DRAMORIG 0x20000000
#define IRAMORIG 0x00000000
#define IRAMSIZE IRAM_SIZE

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
        . = ALIGN(0x200);
        *(.init.text)
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

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .vectors 0x0 :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

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

    .ibss (NOLOAD) :
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

