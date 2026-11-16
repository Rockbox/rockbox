OUTPUT_ARCH(mips)
ENTRY(start)

SECTIONS
{
    . = 0xb4040000;
    .text : { *(.text*) }
    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.comment) *(.pdr) *(.gnu.attributes) }
}
