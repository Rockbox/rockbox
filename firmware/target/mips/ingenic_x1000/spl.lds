#include "config.h"
#include "cpu.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_spl_start)
STARTUP(target/mips/ingenic_x1000/spl-start.o)

MEMORY {
    /* First 4k of TCSM is used by mask ROM for stack + variables,
     * and the next 2k are occupied by SPL header */
    TCSM : ORIGIN = X1000_TCSM_BASE + 0x1800,
           LENGTH = X1000_TCSM_SIZE - 0x1800
}

SECTIONS
{
    /* Mask ROM variables, addresses found by disassembly */
    boot_sel = X1000_TCSM_BASE + 0x1ec;

    .text :
    {
        *(.init.spl);
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

    /DISCARD/ :
    {
        *(.MIPS.abiflags);
        *(.eh_frame);
        *(.rel.dyn);
    }
}
