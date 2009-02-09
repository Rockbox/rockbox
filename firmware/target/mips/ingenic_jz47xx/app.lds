#include "config.h"
#undef mips

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_jz47xx/crt0.o)

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#define PLUGINSIZE    PLUGIN_BUFFER_SIZE
#define CODECSIZE     CODEC_SIZE

#define DRAMSIZE      ((MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGINSIZE - CODECSIZE)

#define DRAMORIG      0x80004000
#define IRAMORIG      0x80000000
#define IRAMSIZE      (16K-0x220)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR       (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    . = DRAMORIG;

    .text :
    {
        loadaddress = .;
        *(.init.text);
        *(.text*);
    } > DRAM
    
    . = ALIGN(4);

    .rodata :
    {
        *(.rodata);  /* problems without this, dunno why */
        *(.rodata*);
        *(.rodata.str1.1);
        *(.rodata.str1.4);
    } > DRAM
    
    . = ALIGN(4);

    .data :
    {
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
    } > DRAM
    
    . = ALIGN(4);
    
    .stack (NOLOAD):
    {
         *(.stack);
         stackbegin = .;
         . += 0x2000;
         stackend = .;
    } > DRAM
    
    . = ALIGN(4);
    
    .iram IRAMORIG:
    {
        _iramstart = .;
        *(.vectors.1);
        . = 0x100;
        *(.vectors.2);
        . = 0x180;
        *(.vectors.3);
        . = 0x200;
        *(.vectors.4);
        *(.vectors);
        
        *(.icode);
        *(.irodata);
        *(.idata);
        KEEP(*(.vectors))
        *(.vectors);
        _iramend = .;
    } > IRAM AT> DRAM
    _iramcopy = LOADADDR(.iram);
    
    . = ALIGN(4);

    .bss (NOLOAD):
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(COMMON);
         *(.scommon*);
         _end = .;
    } > DRAM
    
    . = ALIGN(4);
    
    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss*);
        _iend = .;
    } > IRAM
    
    .audiobuf ALIGN(4) :
    {
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR:
    {
        audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR:
    {
        codecbuf = .;
    }

    .plugin ENDADDR:
    {
        pluginbuf = .;
    }
    
    /DISCARD/ :
    {
        *(.eh_frame);
    }
}
