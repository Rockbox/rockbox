#include "config.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_x1000/crt0.o)

#define DRAMORIG 0x80000000
#define DRAMSIZE (MEMORYSIZE * 0x100000)
#define USED_DRAM 16K

/* TCSM is 16 KiB and is mapped starting at address 0xf4000000.
 *
 * The SPL is loaded to TCSM + 0x1000. The area below that is stack space.
 * The first 2 KiB of SPL is just headers. The code begins at TCSM + 0x1800.
 * The maskrom will jump to that address (via jalr) after loading the SPL.
 */
MEMORY {
    TCSM : ORIGIN = 0xf4001800, LENGTH = 0x2800
    DRAM : ORIGIN = DRAMORIG + DRAMSIZE - USED_DRAM, LENGTH = USED_DRAM
}

SECTIONS
{
    .text :
    {
        *(.init.text);
        *(.text*);
    } > TCSM

    . = ALIGN(4);
    .rodata :
    {
        *(.rodata*);
    } > TCSM

    . = ALIGN(4);
    .data :
    {
        *(.data*);
        *(.sdata*);
    } > TCSM

    . = ALIGN(4);
    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.sbss*);
        *(.bss*);
        *(COMMON);
        *(.scommon*);
        _bssend = .;
    } > TCSM

    .sdram (NOLOAD) :
    {
        *(.sdram);
    } > DRAM
}
