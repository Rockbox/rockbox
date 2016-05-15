#include "config.h"
ENTRY(main)
OUTPUT_ARCH(mips)
STARTUP(jz4760b/crt0.o)

MEMORY
{
    /* see crt0.S from an an explanation of why TCSM0 is the best choice */
    TCSM0 : ORIGIN = TCSM0_ORIG, LENGTH = TCSM0_SIZE
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
        relocend = .;
    } > TCSM0

    .bss (NOLOAD) :
    {
        bssbegin = .;
        *(.bss)
        . = ALIGN(4);
        bssend = .;
    } > TCSM0

    .stack (NOLOAD) :
    {
        oc_codeend = .;
        oc_stackstart = .;
        . += STACK_SIZE;
        oc_stackend = .;
        oc_bufferstart = .;
    } > TCSM0

    .ocend TCSM0_ORIG + TCSM0_SIZE (NOLOAD) :
    {
        oc_bufferend = .;
    } > TCSM0

    /DISCARD/ :
    {
        *(.note.*)
        *(.reginfo*)
        *(.MIPS*)
    }
}
