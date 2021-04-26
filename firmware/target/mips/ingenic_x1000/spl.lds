#include "config.h"
#include "cpu.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_x1000/crt0.o)

MEMORY {
    /* First 4k of TCSM is used by mask ROM for stack + variables,
     * and the next 2k are occupied by SPL header */
    TCSM : ORIGIN = X1000_TCSM_BASE + 0x1800,
           LENGTH = X1000_TCSM_SIZE - 0x1800

    /* Small area of DRAM is required for NAND bounce buffers,
     * though not strictly necessary as ECC isn't really practical
     * this early in the boot */
    DRAM : ORIGIN = X1000_DRAM_END - 16K,
           LENGTH = 16K
}

SECTIONS
{
    .text :
    {
        *(.init.text);
        *(.text*);
        *(.icode*);
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

    /DISCARD/ :
    {
        *(.MIPS.abiflags);
        *(.eh_frame);
        *(.rel.dyn);
    }
}
