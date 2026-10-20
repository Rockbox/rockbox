#include "config.h"
ENTRY(start)
OUTPUT_ARCH(mips)
STARTUP(x1000/crt0.o)

MEMORY
{
    DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE
}

SECTIONS
{
    .itext :
    {
        oc_codestart = .;
        *(.init.text*)
        *(.text*)
        *(.icode*)
        *(.data*)
        *(.rodata*)
        /* exceptions needs to be on a 0x1000 boundary */
        . = ALIGN(0x1000);
        irqbase = .;
        KEEP(*(.exception.tlb_refill))
        . = irqbase + 0x100;
        KEEP(*(.exception.cache_error))
        . = irqbase + 0x180;
        KEEP(*(.exception.general_exception))
        . = ALIGN(4);
    } > DRAM

    .bss (NOLOAD) :
    {
        bssbegin = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        bssend = .;
    } > DRAM

    .stack (NOLOAD) :
    {
        oc_codeend = .;
        oc_stackstart = .;
        . += STACK_SIZE;
        oc_stackend = .;
        oc_bufferstart = .;
    } > DRAM

    .ocend DRAM_ORIG + DRAM_SIZE (NOLOAD) :
    {
        oc_bufferend = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.note.*)
        *(.reginfo*)
        *(.MIPS*)
    }
}
