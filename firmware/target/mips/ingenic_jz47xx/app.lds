#include "config.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_jz47xx/crt0.o)

#define STUBOFFSET    0x4000

#define DRAMORIG      (0x80000000 + STUBOFFSET)
#define DRAMSIZE      (MEMORYSIZE * 0x100000 - STUBOFFSET)
#define IRAMORIG      0x80000000
#define IRAMSIZE      16K

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDCODECADDR  (ENDAUDIOADDR + CODEC_SIZE)

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
    } > DRAM

    /* Set the load address of .iram at the same address as .bss
     * so RAM won't be wasted as .iram in the end will get copied
     * to IRAM. */
    .iram IRAMORIG: AT (_edata)
    {
        _iramstart = .;
        KEEP(*(.vectors.1));
        . = 0x100;
        KEEP(*(.vectors.2));
        . = 0x180;
        KEEP(*(.vectors.3));
        . = 0x200;
        KEEP(*(.vectors.4));
        KEEP(*(.vectors));

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

    .bss (NOLOAD):
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss*); /* Don't put this in IRAM as there's not enough space */
         *(COMMON);
         *(.scommon*);
         _end = .;
    } > DRAM

    .audiobuf :
    {
        . = ALIGN(4);
        audiobuffer = .;
    } > DRAM

    audiobufend = ENDAUDIOADDR;
    codecbuf = ENDAUDIOADDR;
    pluginbuf = ENDCODECADDR;

    /DISCARD/ :
    {
        *(.eh_frame);
        *(.rel.dyn);
    }
}
