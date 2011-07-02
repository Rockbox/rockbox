#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx233/crt0.o)

MEMORY
{
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
    DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE - TTB_SIZE
}

SECTIONS
{
    .text :
    {
        *(.text*)
    } > DRAM

    .data :
    {
        *(.data*)
        *(.rodata*)
        _dataend = . ;
    } > DRAM

    .iram :
    {
        _iramstart = .; // always 0
        *(.vectors)
        KEEP(*(.vectors));// otherwise there are no reference to it and the linker strip it
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .stack (NOLOAD) :
    {
        *(.stack)
        stackbegin = .;
        . += 0x2000;
        stackend = .;
    } > DRAM

    .bss (NOLOAD) :
    {
        _edata = .;
        *(.bss*);
        _end = .;
    } > DRAM
}
