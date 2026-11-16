/* stage1.5 link script, LOAD_ADDR passed with --defsym */
OUTPUT_ARCH(mips)
ENTRY(start)

SECTIONS
{
    . = LOAD_ADDR;
    image_start = .;
    .text :
    {
        KEEP(*(.init.text*))
        *(.text*)
        *(.rodata*)
        *(.data*)
        *(.sdata*)
        . = ALIGN(4);
    }
    image_end = .;
    payload_start = .;

    .bss (NOLOAD) :
    {
        bssbegin = .;
        *(.sbss*)
        *(.bss*)
        *(COMMON)
        *(.scommon*)
        . = ALIGN(4);
        bssend = .;
    }

    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.pdr) *(.gnu.attributes) }
}
