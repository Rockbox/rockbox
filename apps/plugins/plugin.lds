#include "config.h"

/* These output formats should be in the config-files */

#if CONFIG_CPU == MCF5249
OUTPUT_FORMAT(elf32-m68k)
#else
OUTPUT_FORMAT(elf32-sh)
#endif

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#ifdef IRIVER_H100
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10010000
#define IRAMSIZE 0x8000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#endif

#ifdef CODEC
#define CODEC_SIZE 0x40000
#else
#define CODEC_SIZE 0
#endif

#if MEMORYSIZE >= 32
#define PLUGIN_LENGTH 0xC0000
#else
#define PLUGIN_LENGTH 0x8000
#endif

#if CODEC_SIZE > 0
#define THIS_LENGTH CODEC_SIZE
#else
#define THIS_LENGTH PLUGIN_LENGTH
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH - CODEC_SIZE
#define PLUGIN_ORIGIN (DRAMORIG + (DRAMSIZE))

MEMORY
{
   PLUGIN_RAM : ORIGIN = PLUGIN_ORIGIN, LENGTH = THIS_LENGTH
#ifdef IRIVER_H100
   PLUGIN_IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
#endif
}

SECTIONS
{
    .text : {
        *(.entry)
        *(.text)
    } > PLUGIN_RAM

    .data : {
        *(.data)
    } > PLUGIN_RAM

    /DISCARD/ : {
        *(.eh_frame)
    }

    .bss : {
        *(.bss)
    } > PLUGIN_RAM

    .rodata : {
        *(.rodata)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
#ifdef IRIVER_H100
        iramcopy = .;
#endif
    } > PLUGIN_RAM

#ifdef IRIVER_H100
    .iram IRAMORIG : AT ( iramcopy)
    {
        iramstart = .;
        *(.icode)
        *(.idata)
        iramend = .;
    } > PLUGIN_IRAM
#endif
}
