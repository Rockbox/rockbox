OUTPUT_FORMAT(elf32-sh)

MEMORY
{
   PLUGIN_RAM   : ORIGIN = 0x091f8000, LENGTH = 0x8000
}

SECTIONS
{
    .text : {
        *(.entry)
        *(.text)
    } > PLUGIN_RAM

    .data : {
        *(.data)
    } > PLUGIN_RAM

    .bss : {
        *(.bss)
    } > PLUGIN_RAM

    .rodata : {
        *(.rodata)
    } > PLUGIN_RAM
}
