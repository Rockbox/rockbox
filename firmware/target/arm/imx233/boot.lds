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
    .vectors 0 :
    {
        *(.vectors);
        . = ALIGN(0x4);
    } > IRAM

    .itext :
    {
        *(.icode)
        *(.init.text)
        . = ALIGN(0x4);
    } > IRAM

    .idata :
    {
        *(.qharray)
        *(.idata)
        *(.irodata)
        . = ALIGN(0x4);
    } > IRAM

    .ibss :
    {
        *(.ibss)
    } > IRAM

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

    .stack (NOLOAD) :
    {
        *(.stack)
        _stackbegin = .;
        stackbegin = .;
        . += 0x2000;
        _stackend = .;
        stackend = .;
    } > DRAM

    .bss (NOLOAD) :
    {
        _edata = .;
        *(.bss*);
        _end = .;
    } > DRAM
}
