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
    } > DRAM

    . = ALIGN(4);

    .rodata :
    {
        *(.rodata*);
    } > DRAM

    . = ALIGN(4);

    .data :
    {
        *(.data*);
        *(.sdata*);
        *(.rel.dyn);
    } > DRAM

    . = ALIGN(4);

    .iram IRAM_ORIG:
    {
        _iramstart = .;
        KEEP(*(.vectors.1));
        . = 0x100;
        KEEP(*(.vectors.2));
        . = 0x180;
        KEEP(*(.vectors.3));
        . = 0x200;
        KEEP(*(.vectors.4));
        KEEP(*(.vectors));

        *(.icode);
        *(.irodata);
        *(.idata);
        KEEP(*(.vectors*))
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    . = ALIGN(4);

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
