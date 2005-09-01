#include "config.h"

/* These output formats should be in the config-files */

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#else
OUTPUT_FORMAT(elf32-sh)
#endif

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300)
#define ARCH_IRIVER
#endif

#ifdef ARCH_IRIVER
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#endif

#define PLUGIN_LENGTH PLUGIN_BUFFER_SIZE
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH - CODEC_SIZE
#define CODEC_ORIGIN (DRAMORIG + (DRAMSIZE))
#define PLUGIN_ORIGIN (CODEC_ORIGIN + CODEC_SIZE)


#ifdef CODEC
#define THIS_LENGTH CODEC_SIZE
#define THIS_ORIGIN CODEC_ORIGIN
#else
#define THIS_LENGTH PLUGIN_LENGTH
#define THIS_ORIGIN PLUGIN_ORIGIN
#endif

MEMORY
{
   PLUGIN_RAM : ORIGIN = THIS_ORIGIN, LENGTH = THIS_LENGTH
#ifdef ARCH_IRIVER
   PLUGIN_IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
#endif
}

SECTIONS
{
    .text : {
        KEEP(*(.entry))
        *(.text*)
    } > PLUGIN_RAM

    .data : {
        *(.data*)
    } > PLUGIN_RAM

    /DISCARD/ : {
        *(.eh_frame)
    }

    .bss : {
        *(.bss*)
    } > PLUGIN_RAM

    .rodata : {
        *(.rodata*)
        . = ALIGN(0x4);
#ifdef ARCH_IRIVER
        iramcopy = .;
#endif
    } > PLUGIN_RAM

#ifdef ARCH_IRIVER
    .iram IRAMORIG : AT ( iramcopy)
    {
        iramstart = .;
        *(.icode)
        *(.idata)
        iramend = .;
    } > PLUGIN_IRAM
#endif
    /* Special trick to avoid a linker error when no other sections are
       left after garbage collection (plugin not for this platform) */
    .comment 0 : {
        KEEP(*(.comment))
    }
}
