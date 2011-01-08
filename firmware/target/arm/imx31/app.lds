#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx31/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#include "imx31l.h"
/* Subtract 1MB for the FRAME/TTB section */
#define DRAMSIZE ((MEMORYSIZE * 0x100000) - CODECSIZE - \
                  PLUGINSIZE - QHARRAY_SIZE - FRAME_SIZE - TTB_SIZE)

#define DRAMORIG 0x0
/* #define IRAMORIG 0x1FFFC000 */
#define IRAMORIG DRAMORIG
#define IRAM     DRAM
#define IRAMSIZE IRAM_SIZE

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

/* INIT section is the codec buffer */
#define INITSIZE  CODECSIZE
#define INITSTART ENDAUDIOADDR

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    INIT : ORIGIN = INITSTART, LENGTH = INITSIZE
    QHARRAY : ORIGIN = QHARRAY_PHYS_ADDR, LENGTH = QHARRAY_SIZE
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

#if 0 /* Unneeded at the moment */
    /* .ncdata section is placed at uncached physical alias address and is
     * loaded at the proper cached virtual address - no copying is
     * performed in the init code */
    .ncdata . + NOCACHE_BASE :
    {
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
    } AT> DRAM
#endif
    /DISCARD/ :
    {
        *(.eh_frame)
    }

    /* NOLOAD sections at end start here. Any part of the binary image past
     * this point is discarded or copied elsewhere. */
    _noloaddram = .;

    .vectors 0x0 :
    {
        _vectorsstart = .;
        *(.vectors)
        _vectorsend = .;
    } AT> DRAM

    _vectorscopy = LOADADDR(.vectors);
    
    .init :
    {
        . = ALIGN(4);
        _initstart = .;
        *(.init)
        *(.initdata)
        _initend = .;
    } > INIT AT> DRAM

    _initcopy = LOADADDR(.init);

    .stack _noloaddram (NOLOAD) :
    {
        . = ALIGN(4);
        *(.stack)
        stackbegin = .;
        . += 0x2000;
        stackend = .;
    } > DRAM
    
    .bss (NOLOAD) :
    {
        _edata = .;
        *(.bss*)
        *(.ibss)
        *(COMMON)
        . = ALIGN(0x4);
    } > DRAM
    
    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
        *(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE); /* >= Cache line boundary */
    } AT> DRAM
    
    .endaddr . - NOCACHE_BASE (NOLOAD) :
    {
        _end = .;
    } > DRAM

    .audiobuf (NOLOAD) :
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR (NOLOAD) :
    {
        audiobufend = .;
        _audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR (NOLOAD) :
    {
        codecbuf = .;
        _codecbuf = .;
    }

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
    
    .qharray (NOLOAD) :
    {
	_qharray = .;
	*(.qharray)
	_qharrayend = .;
    } > QHARRAY
}

