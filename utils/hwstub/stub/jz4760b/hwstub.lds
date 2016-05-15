#include "config.h"
ENTRY(main)
OUTPUT_ARCH(mips)
STARTUP(jz4760b/crt0.o)

MEMORY
{
    DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
}

SECTIONS
{
    .itext :
    {
        relocstart = .;
        oc_codestart = .;
        *(.init.text*)
        *(.text*)
        *(.icode*)
        *(.data*)
        *(.rodata*)
        . = ALIGN(4);
        _copyend = .;
    } > IRAM

    .bss (NOLOAD) :
    {
        bssbegin = .;
        *(.bss)
        . = ALIGN(4);
        bssend = .;
    } > IRAM

    .stack (NOLOAD) :
    {
        oc_codeend = .;
        oc_stackstart = .;
        . += STACK_SIZE;
        oc_stackend = .;
        oc_bufferstart = .;
    } > IRAM

    .ocend IRAM_ORIG + IRAM_SIZE (NOLOAD) :
    {
        oc_bufferend = .;
    } > IRAM

    /DISCARD/ :
    {
        *(.note.*)
        *(.reginfo*)
        *(.MIPS*)
    }
}
