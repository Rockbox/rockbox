#include "config.h"

/* These output formats should be in the config-files */

#if CONFIG_CPU == MCF5249
OUTPUT_FORMAT(elf32-m68k)
#else
OUTPUT_FORMAT(elf32-sh)
#endif

#define PLUGIN_LENGTH 0x8000
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH
#define PLUGIN_ORIGIN (0x09000000 + (DRAMSIZE))

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
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > PLUGIN_RAM
}
