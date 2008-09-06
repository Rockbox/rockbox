#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/tcc77x/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE
                                                                                                                                              
#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE

#define DRAMORIG 0x20000000
#define IRAMORIG 0x00000000
#define IRAMSIZE 64K


/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)


MEMORY
{
#ifdef TCCBOOT
   DRAM : ORIGIN = DRAMORIG + DRAMSIZE - 0x100000, LENGTH = 0x100000
#else
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
#endif
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    .text : {
        loadaddress = .;                                                                                                                      
        _loadaddress = .;                                                                                                                     
        . = ALIGN(0x200);                                                                                                                     
        *(.init.text)
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
   } > DRAM

    .data : {
        *(.icode)
        *(.irodata)
        *(.idata)
        *(.data*)
        *(.rodata.*)
        *(.rodata)
        . = ALIGN(0x4);
        _dataend = . ;
    } > DRAM

    .stack :
    {
         *(.stack)
         _stackbegin = .;
         stackbegin = .;
         . += 0x2000;
         _stackend = .;
         stackend = .;
    } > DRAM

    .bss : {
         _edata = .;
         *(.bss*);
         *(.ibss);
         *(COMMON)
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
