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
#define DRAMORIG 0x30000000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#endif

#if MEMORYSIZE >= 32
#define PLUGIN_LENGTH 0xC0000
#else
#define PLUGIN_LENGTH 0x8000
#endif
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH
#define PLUGIN_ORIGIN (DRAMORIG + (DRAMSIZE))

MEMORY
{
   PLUGIN_RAM : ORIGIN = PLUGIN_ORIGIN, LENGTH = PLUGIN_LENGTH
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

    .bss : {
        *(.bss)
    } > PLUGIN_RAM

    .rodata : {
        *(.rodata)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > PLUGIN_RAM
}
