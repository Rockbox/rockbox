#include "config.h"
#include "cpu.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/jz4760b/crt0.o)

MEMORY
{
    /* this section has k0 addresses */
    DRAM_PHYS : ORIGIN = PHYSICAL_DRAM_ADDR + 0x80000000, LENGTH = DRAM_SIZE
    /* the following two sections use virtual addresses */
    DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
}

SECTIONS
{
    .text :
    {
        *(.init.text);
        *(.text*);
        *(.rodata*);
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
        . = ALIGN(4);
    } > DRAM AT> DRAM_PHYS

    _relocstart = LOADADDR(.text);

    .iram :
    {
        _iramstart = .;
        KEEP(*(.vectors.tlb_refill));
        . = 0x100;
        KEEP(*(.vectors.cache_error));
        . = 0x180;
        KEEP(*(.vectors.exception));
        . = 0x200;
        KEEP(*(.vectors.interrupt));
        KEEP(*(.vectors));

        *(.icode);
        *(.irodata);
        *(.idata);
        . = ALIGN(4);
        _iramend = .;
    } > IRAM AT> DRAM_PHYS

    _iramcopy = LOADADDR(.iram);

    .relocend (NOLOAD) :
    {
    } > DRAM AT> DRAM_PHYS
    _relocend = LOADADDR(.relocend);

    .bss (NOLOAD):
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss*);
         *(COMMON);
         *(.scommon*);
         . = ALIGN(4);
         _end = .;
    } > DRAM

    .stack (NOLOAD):
    {
         *(.stack)
         stackbegin = .;
         . += 0x2000;
         stackend = .;
    } > IRAM

    /* buffer to load firmware */
    .buffer (NOLOAD):
    {
        _loadaddress = .;
        _loadaddressend = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.MIPS.abiflags);
    }
}
