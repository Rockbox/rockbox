OUTPUT_FORMAT(elf32-sh)

#define PLUGIN_LENGTH 0x8000
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH
#define PLUGIN_ORIGIN (0x09000000 + (DRAMSIZE))

MEMORY
{
   PLUGIN_RAM : ORIGIN = PLUGIN_ORIGIN, LENGTH = PLUGIN_LENGTH
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
        *(.rodata.str1.4)
    } > PLUGIN_RAM
}
