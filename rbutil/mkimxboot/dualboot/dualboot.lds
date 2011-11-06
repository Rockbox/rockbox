ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

MEMORY
{
    OCRAM : ORIGIN = 0, LENGTH = 0x8000
}

SECTIONS
{
    .text 0 :
    {
        *(.text*)
    } > OCRAM
}
