#include "config.h"

/* linker script for rockboy as an overlay,
 * only used/ necessary for SH-based archos targets */

OUTPUT_FORMAT(elf32-sh)

#define DRAMORIG 0x09000000
#define PLUGIN_LENGTH PLUGIN_BUFFER_SIZE

#define OVERLAY_LENGTH 0x68000
#define OVERLAY_ORIGIN (DRAMORIG + (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH - OVERLAY_LENGTH)

MEMORY
{
   OVERLAY_RAM : ORIGIN = OVERLAY_ORIGIN, LENGTH = OVERLAY_LENGTH
}

SECTIONS
{
    .header : {
        _plugin_start_addr = .;
        KEEP(*(.header))
    } > OVERLAY_RAM

    .text : {
        *(.text*)
    } > OVERLAY_RAM

    .rodata : {
        *(.rodata*)
    } > OVERLAY_RAM

    .data : {
        *(.data*)
    } > OVERLAY_RAM

    .bss : {
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        _plugin_end_addr = .;
    } > OVERLAY_RAM
}
