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

#define DRAMSIZE      (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - STUBOFFSET - CODEC_SIZE

#define DRAMORIG      0x80004000
#define IRAMORIG      0x80000000
#define IRAMSIZE      (16K-0x220)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR       (ENDAUDIOADDR + CODEC_SIZE)

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
        _loadaddress = .;
        *(.init.text);
        *(.text*);
    } > DRAM
    
    . = ALIGN(4);

    .rodata :
    {
        *(.rodata*);
    } > DRAM
    
    . = ALIGN(4);

    .data :
    {
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
    } > DRAM
       
    . = ALIGN(4);
    
    /* Set the load address of .iram at the same address as .bss
     * so RAM won't be wasted as .iram in the end will get copied
     * to IRAM. */
    .iram IRAMORIG: AT (_edata)
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
    } > IRAM
    _iramcopy = LOADADDR(.iram);
    
    . = ALIGN(4);
    
    .stack (NOLOAD):
    {
         *(.stack);
         stackbegin = .;
         . += 0x2000;
         stackend = .;
    } > IRAM
    
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
    }

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
