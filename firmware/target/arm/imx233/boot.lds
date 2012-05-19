#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx233/crt0.o)

/* Leave a hole at the beginning of the RAM to load the firmware */
#define RAM_HOLE    1024 * 1024

/* Make a difference between virtual and physical address so that we can use
 * the resulting elf file with the elftosb tools which loads at the *physical*
 * address */

MEMORY
{
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
    DRAM : ORIGIN = CACHED_DRAM_ADDR + RAM_HOLE, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE - RAM_HOLE
    UDRAM : ORIGIN = UNCACHED_DRAM_ADDR + RAM_HOLE, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE - RAM_HOLE
}

SECTIONS
{
    loadaddress = UNCACHED_DRAM_ADDR;
    _loadaddress = UNCACHED_DRAM_ADDR;
    loadaddressend = UNCACHED_DRAM_ADDR + RAM_HOLE;
    _loadaddressend = UNCACHED_DRAM_ADDR + RAM_HOLE;

    .dramcopystart (NOLOAD) :
    {
        _dramcopystart = .;
    } > DRAM
    
    .text :
    {
        *(.text*)
        *(.data*)
        *(.rodata*)
    } > DRAM

    .itext :
    {
        _iramstart = .; // always 0
        *(.vectors)
        KEEP(*(.vectors));// otherwise there are no references to it and the linker strip it
        *(.icode*)
        *(.irodata*)
        *(.idata*)
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.itext);

    .dramcopyend (NOLOAD) :
    {
        _dramcopyend = .;
    } > DRAM

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss*)
        _iend = .;
    } > IRAM

    .stack (NOLOAD) :
    {
        *(.stack)
        stackbegin = .;
        . += 0x2000;
        stackend = .;
    } > DRAM

    /* physical address of the stack */
    stackend_phys = stackend - CACHED_DRAM_ADDR + UNCACHED_DRAM_ADDR;

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

    .bssendadr (NOLOAD) :
    {
        _end = .;
    } > DRAM
}
