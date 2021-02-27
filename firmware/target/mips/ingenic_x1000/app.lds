#include "config.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_x1000/crt0.o)

/* Stub area is used for loading new firmware via RoLo */
#define STUBSIZE    0x4000
#define SDRAM_ORIG  0x80000000

/* IRAM contains stub, DRAM contains main app */
#define IRAMORIG SDRAM_ORIG
#define IRAMSIZE STUBSIZE
#define DRAMORIG (SDRAM_ORIG + STUBSIZE)
#define DRAMSIZE (MEMORYSIZE * 0x100000 - STUBSIZE)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDCODECADDR  (ENDAUDIOADDR + CODEC_SIZE)

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
    } > DRAM

    .iram IRAMORIG: AT (_bssbegin)
    {
        _iramstart = .;
        . = 0x000; /* TLB refill */
        KEEP(*(.vectors.1));
        . = 0x100; /* Cache error */
        KEEP(*(.vectors.2));
        . = 0x180; /* General exception */
        KEEP(*(.vectors.3));
        . = 0x200; /* Interrupt */
        KEEP(*(.vectors.4));
        KEEP(*(.vectors));

        *(.icode);
        *(.irodata);
        *(.idata);
        _iramend = .;
    } > IRAM
    _iramcopy = LOADADDR(.iram);

    . = ALIGN(4);
    .stack (NOLOAD) :
    {
         *(.stack);
         stackbegin = .;
         . += 0x1E00;
         stackend = .;
         _irqstackbegin = .;
         . += 0x300;
         _irqstackend = .;
    } > IRAM

    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.sbss*);
        *(.bss*);
        *(COMMON);
        *(.scommon*);
        _bssend = .;
        _end = .;
    } > DRAM

#ifdef BOOTLOADER
    . = ALIGN(4);
    loadbuffer = .;
    . += 0x100000 * 4; /* Allow 4 MiB for the rockbox binary */
    loadbufferend = .;
#else

    .audiobuf :
    {
        . = ALIGN(4); /* XXX might need more alignment here */
        audiobuffer = .;
    } > DRAM

    audiobufend = ENDAUDIOADDR;
    codecbuf = ENDAUDIOADDR;
    pluginbuf = ENDCODECADDR;
#endif

    /DISCARD/ :
    {
        *(.eh_frame);
        *(.rel.dyn);
    }
}
