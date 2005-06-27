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
        _ovl_start_addr = .;
        *(.header)
    } > OVERLAY_RAM

    .text : {
        *(.entry)
        *(.text)
    } > OVERLAY_RAM

    .data : {
        *(.data)
    } > OVERLAY_RAM

    .bss : {
        *(.bss)
    } > OVERLAY_RAM

    .rodata : {
        *(.rodata)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
        _ovl_end_addr = .;
    } > OVERLAY_RAM
}
