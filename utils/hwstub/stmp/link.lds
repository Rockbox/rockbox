
ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(crt0.o)



MEMORY
{
    OCRAM : ORIGIN = 0, LENGTH = 0x8000
}

SECTIONS
{
    .octext :
    {
        oc_codestart = .;
        *(.text*)
        *(.data*)
        *(.rodata*)
    } > OCRAM

    .bss (NOLOAD) :
    {
        bss_start = .;
        *(.bss)
        bss_end = .;
    } > OCRAM

    .stack (NOLOAD) :
    {
        oc_codeend = .;
        oc_stackstart = .;
        . += 0x1000;
        oc_stackend = .;
        oc_bufferstart = .;
    } > OCRAM

    .ocend (0 + 0x8000) (NOLOAD) :
    {
        oc_bufferend = .;
    } > OCRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }
}
