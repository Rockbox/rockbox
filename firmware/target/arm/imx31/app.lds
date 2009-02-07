#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx31/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#include "imx31l.h"
/* Subtract 1MB for the FRAME/TTB section */
#define DRAMSIZE (MEMORYSIZE * 0x100000 - 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE

#define DRAMORIG (0x0 + STUBOFFSET)
/* #define IRAMORIG 0x1FFFC000 */
#define IRAMORIG DRAMORIG
#define IRAM     DRAM
#define IRAMSIZE IRAM_SIZE

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    DEVBSS : ORIGIN = CSD0_BASE_ADDR + (MEMORYSIZE * 0x100000 - 0x100000), \
             LENGTH = 0x100000 - FRAME_SIZE - TTB_SIZE
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
        *(.icode)
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
        *(.irodata)
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
        *(.idata)
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
        *(.ibss)
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
    
    .devbss (NOLOAD) :
    {
    	_devbssdata = .;
    	*(.qharray)
    	*(.devbss*)
    	_devbssend = .;
    } > DEVBSS
}

