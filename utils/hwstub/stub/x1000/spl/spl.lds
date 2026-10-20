OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)
ENTRY(_spl_start)

/* The boot ROM loads the SPL to TCSM + 0x1000 and runs it at TCSM + 0x1800,
 * past the header that mkspl-x1000 adds. */
MEMORY
{
    TCSM : ORIGIN = 0xf4001800, LENGTH = 0x2800
}

SECTIONS
{
    .text :
    {
        *(.startup.spl);
        *(.text*);
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
        *(.reginfo*);
        *(.note.*);
        *(.eh_frame);
        *(.rel.dyn);
    }
}
