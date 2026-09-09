/* Copyright (C) 2026 Michael McAllister
 * Returning USB stage1: keep the image above the reserved BootROM window.
 * This load address has worked on the R1; the cache mapping backing it is
 * unverified. See firmware/export/x1600.h. */

#include "config.h"
#include "x1600.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_usb_stage1_start)
STARTUP(target/mips/ingenic_x1600/spl-start.o)

MEMORY {
    STAGE1 : ORIGIN = X1600_USB_STAGE1_ADDR,
             LENGTH = X1600_USB_STAGE1_MAXSIZE
}

SECTIONS
{
    .text :
    {
        *(.startup.usbstage1);
        *(.text*);
        *(.icode*);
        *(.init*);
        . = ALIGN(4);
        *(.rodata*);
        . = ALIGN(4);
        *(.data*);
        *(.sdata*);
    } > STAGE1

    . = ALIGN(4);
    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.sbss*);
        *(.bss*);
        *(COMMON);
        *(.scommon*);
        _bssend = .;
    } > STAGE1

    /DISCARD/ :
    {
        *(.startup.spl);
        *(.MIPS.abiflags);
        *(.eh_frame);
        *(.rel.dyn);
    }

}

/* Keep linker assertions outside the SECTIONS block. */

ASSERT(ADDR(.text) >= X1600_BOOTROM_RESERVED_END,
       "USB stage1 must be linked above the BootROM reserved window");
ASSERT(_bssend - ADDR(.text) <= X1600_USB_STAGE1_MAXSIZE,
       "USB stage1 exceeds the BootROM 20 KiB limit (PM 34.9)");
ASSERT(_bssend <= X1600_USB_STAGE1_STACK,
       "USB stage1 collides with its own private stack at 0x8000e000");
