OUTPUT_ARCH(mips)
ENTRY(start)

SECTIONS
{
    . = 0xa0080000;
    .params : { KEEP(*(.params)) }
    .text 0xa0080040 :
    {
        KEEP(*(.init.text))
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.sdata*)
        *(.sbss*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    }
    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.pdr) *(.gnu.attributes) }
}
