/* Copyright (C) 2026 Michael McAllister
 * Flash SPL: the BootROM loads the image at X1600_SPL_LOAD_ADDR and enters
 * past its 2 KiB header. USB stage1 uses usbstage1.lds to preserve BootROM
 * state on return. */

#include "config.h"
#include "x1600.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_spl_start)
STARTUP(target/mips/ingenic_x1600/spl-start.o)

MEMORY {
    SPLRAM : ORIGIN = X1600_SPL_EXEC_ADDR,
             LENGTH = X1600_SPL_SIZE
}

SECTIONS
{
    .text :
    {
        *(.startup.spl);
        *(.text*);
        *(.icode*);
        *(.init*);
    } > SPLRAM

    . = ALIGN(4);
    .rodata :
    {
        *(.rodata*);
    } > SPLRAM

    . = ALIGN(4);
    .data :
    {
        *(.data*);
        *(.sdata*);
    } > SPLRAM

    . = ALIGN(4);
    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.sbss*);
        *(.bss*);
        *(COMMON);
        *(.scommon*);
        _bssend = .;
    } > SPLRAM

    /DISCARD/ :
    {
        *(.startup.usbstage1);
        *(.MIPS.abiflags);
        *(.eh_frame);
        *(.rel.dyn);
    }
}
