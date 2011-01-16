/* Will have been included from boot.lds */
ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0-pp502x-bl-usb.o)

#define DRAMORIG        0x01000000 /* Load at 16 MB */
#define DRAMSIZE        0x00100000 /* 1MB for bootloader */
#define MEMEND          (MEMORYSIZE*0x100000) /* From virtual mapping at 0 */
#define NOCACHE_BASE    0x10000000
#ifndef IRAMORIG
#define IRAMORIG 0x40000000
#endif
#define IRAMSIZE 0x20000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M

#define CACHEALIGN_SIZE 16

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
    . = DRAMORIG;
    _loadaddress = . + NOCACHE_BASE;

    .text :
    {
        *(.init.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata)  /* problems without this, dunno why */
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > DRAM

    .data :
    {
        *(.data*)
        . = ALIGN(0x4);
    } > DRAM

    /* .ncdata section is placed at uncached physical alias address and is
     * loaded at the proper cached virtual address - no copying is
     * performed in the init code */
    .ncdata . + NOCACHE_BASE :
    {
        . = ALIGN(CACHEALIGN_SIZE);
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> DRAM

    /DISCARD/ . - NOCACHE_BASE :
    {
        *(.eh_frame)
    } > DRAM

    _noloaddram  = .;

    .ibss IRAMORIG (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .iram _iend :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .loadaddressend :
    {
        _loadaddressend = . + NOCACHE_BASE;
    } AT> DRAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        *(.stack)
        stackbegin = .;
        . += 0x2000;
        stackend = .;
    } > IRAM

    /* .bss and .ncbss are treated as a single section to use one init loop
     * to zero them - note "_edata" and "_end" */
    .bss _noloaddram (NOLOAD) :
    {
        _edata = .;
        *(.bss*)
        *(COMMON)
    } > DRAM

    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
        . = ALIGN(CACHEALIGN_SIZE);
        *(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> DRAM

    /* This will be aligned by preceding alignments */
    .endaddr . - NOCACHE_BASE (NOLOAD) :
    {
        _end = .;
    } > DRAM

    /* Reference to all DRAM after loaded bootloader image */
    .freebuffer _end (NOLOAD) :
    {
        . = ALIGN(4);
        freebuffer = .;
        . = MEMEND-1;
        freebufferend = .;
    }
}
