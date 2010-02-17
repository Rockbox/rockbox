#include "config.h"
#include "cpu.h"

ENTRY(start)

OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/s5l8700/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif


#define IRAMORIG 0x0
#define DRAMORIG 0x08000000

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

#define DRAM_SIZE (MEMORYSIZE * 0x100000)

#define DRAMSIZE (DRAM_SIZE - STUBOFFSET - PLUGINSIZE - CODECSIZE)
#define CODECORIG (ENDAUDIOADDR)
#if CONFIG_CPU==S5L8700
#define IRAMSIZE (128*1024)  /* 256KB total - 128KB for core, 128KB for plugins */
#else /* S5L8701 */
#define IRAMSIZE (96*1024)  /* 176KB total - 96KB for core, 80KB for plugins */
#endif

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    loadaddress = DRAMORIG;

    .intvect : {
        _intvectstart = . ;
        *(.intvect)
        _intvectend = _newstart ;  
    } >IRAM AT> DRAM
    _intvectcopy = LOADADDR(.intvect) ;

    .text :
    {
        _loadaddress = .;
        _textstart = .;
        *(.init.text)
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata*)
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
    } > IRAM AT> DRAM
    _iramcopy = LOADADDR(.iram) ;

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .stack (NOLOAD) :
    {
       *(.stack)
       stackbegin = .;
       _stackbegin = .;
       . += 0x2000;
       stackend = .;
       _stackend = .;
       _irqstackbegin = .;
       . += 0x400;
       _irqstackend = .;
       _fiqstackbegin = .;
       . += 0x400;
       _fiqstackend = .;
    } > IRAM

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
    } > DRAM

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}
