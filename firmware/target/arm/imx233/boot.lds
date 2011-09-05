#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx233/crt0.o)

MEMORY
{
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
    DRAM : ORIGIN = CACHED_DRAM_ADDR, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE
    UNCACHED_DRAM : ORIGIN = UNCACHED_DRAM_ADDR, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE
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
        KEEP(*(.vectors));// otherwise there are no references to it and the linker strip it
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

    /* treat .bss and .ncbss as a single section */
    .bss (NOLOAD) :
    {
        _edata = .;
        *(.bss*);
    } > DRAM

    /* align on cache size boundary to avoid mixing cached and noncached stuff */
    .ncbss . - CACHED_DRAM_ADDR + UNCACHED_DRAM_ADDR (NOLOAD) :
    {
        . = ALIGN(CACHEALIGN_SIZE);
        *(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> DRAM

    .bssendadr . - UNCACHED_DRAM_ADDR + CACHED_DRAM_ADDR (NOLOAD) :
    {
        _end = .;
    } > DRAM
}
