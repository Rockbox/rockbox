#include "config.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tcc780x/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE

#define DRAMORIG 0x20000000 + STUBOFFSET
#define ITCMORIG 0x00000000
#define ITCMSIZE 0x1000
#define DTCMORIG 0xA0000000
#define DTCMSIZE 0x2000
#define SRAMORIG 0x10000000
#define SRAMSIZE 0xc000

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    /* Seperate data & instruction TCMs plus SRAM. */
    ITCM : ORIGIN = ITCMORIG, LENGTH = ITCMSIZE
    DTCM : ORIGIN = DTCMORIG, LENGTH = DTCMSIZE
    SRAM : ORIGIN = SRAMORIG, LENGTH = SRAMSIZE
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

    .vectors ITCMORIG :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } > ITCM AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

    .itcm :
    {
        _itcmstart = .;
        _itcmend = .;
    } > ITCM AT> DRAM

    _itcmcopy = LOADADDR(.itcm);
    
    .dtcm :
    {
        _dtcmstart = .;
        _dtcmend = .;
    } > DTCM AT> DRAM

    _dtcmcopy = LOADADDR(.dtcm);

    .iram :
    {
        _iramstart = .;
        *(.irodata)
        *(.idata)
        *(.icode)
        . = ALIGN(0x4);
        _iramend = .;
    } > SRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);
    
    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > SRAM

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > SRAM

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
