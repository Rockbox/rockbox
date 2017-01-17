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
        /* exceptions needs to be on a 0x1000 boundary */
        . = ALIGN(0x1000);
        tcsm0_irqbase = .;
        KEEP(*(.exception.tlb_refill))
        *(.exception.tlb_refill)
        . = tcsm0_irqbase + 0x100;
        KEEP(*(.exception.cache_error))
        *(.exception.cache_error)
        . = tcsm0_irqbase + 0x180;
        KEEP(*(.exception.general_exception))
        *(.exception.general_exception)
        . = ALIGN(4);
        relocend = .;
    } > TCSM0

    /* tcsm0_irqbase is the address in the 0xf400xxxx address space, but for
     * EBASE, we want to the corresponding k1seg address, that maps to the
     * physical address of TCSM0 */
    irqbase = tcsm0_irqbase - TCSM0_ORIG + TCSM0_UNCACHED_ADDRESS;

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
