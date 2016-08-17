#include "config.h"
#include "cpu.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/jz4760b/crt0.o)

MEMORY
{
   DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE
   IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
}

SECTIONS
{
    .relocstart (NOLOAD) :
    {
        _relocstart = .;
    } > DRAM

    .text :
    {
        *(.init.text);
        *(.text*);
        . = ALIGN(4);
    } > DRAM

    .rodata :
    {
        *(.rodata*);
        . = ALIGN(4);
    } > DRAM

    .data :
    {
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
        . = ALIGN(4);
    } > DRAM

    .iram IRAM_ORIG:
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
        KEEP(*(.vectors*))
        . = ALIGN(4);
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .relocend (NOLOAD) :
    {
        _relocend = .;
    } > DRAM

    .bss (NOLOAD):
    {
         _edata = .;
         *(.sbss*);
         *(.bss*);
         *(.ibss*);
         *(COMMON);
         *(.scommon*);
         _end = .;
    } > DRAM

    /* put the early crt0 stack in DRAM, after the IRAM content to be relocated,
     * so that it does not overwrite anything. Note that this is the address of
     * the end of the stack */
    crt0_tmpstack_phys = _iramcopy + SIZEOF(.iram) + 0x2000;

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
        /*. = DRAM_ORIG + DRAM_SIZE;*/
        _loadaddressend = .;
    } > DRAM
}
