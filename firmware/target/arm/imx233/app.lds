#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/imx233/crt0.o)

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#define IRAMSIZE IRAM_SIZE

#define DRAMSIZE (DRAM_SIZE - PLUGINSIZE - CODECSIZE - FRAME_SIZE - TTB_SIZE)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (CACHED_DRAM_ADDR + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODEC_SIZE)

/* INIT section is the codec buffer */
#define INITSIZE  CODEC_SIZE
#define INITSTART ENDAUDIOADDR

MEMORY
{
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAMSIZE
    DRAM : ORIGIN = CACHED_DRAM_ADDR, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE
    UDRAM : ORIGIN = UNCACHED_DRAM_ADDR, LENGTH = DRAM_SIZE - TTB_SIZE - FRAME_SIZE
}

SECTIONS
{
    loadaddress = UNCACHED_DRAM_ADDR;
    _loadaddress = UNCACHED_DRAM_ADDR;

    .dramcopystart (NOLOAD) :
    {
        _dramcopystart = .;
    } > DRAM

    .text :
    {
        *(.text*)
        *(.data*)
        *(.rodata*)
        /* remember address of magic section */
        _magicoffset = . - _dramcopystart;
        KEEP(*(.magic*))
    } > DRAM

    _dramtextend = .;

    .itext :
    {
        _iramstart = .; // always 0
        KEEP(*(.vectors));// otherwise there are no references to it and the linker strip it
        *(.icode*)
        *(.irodata*)
        *(.idata*)
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.itext);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.qharray)
        *(.ibss*)
        _iend = .;
    } > IRAM

    .init ENDAUDIOADDR :
    {
        _initstart = .;
        *(.init*)
        _initend = .;
    } AT> DRAM

    _initcopy = LOADADDR(.init);

    /* crt0 needs a temporary stack which does not overlap with init and itext
     * and is in physical memory: put it *after* init and itext. A small one suffices */
    crt0_tmpstack_phys = _dramcopyend + 0x200 - CACHED_DRAM_ADDR + UNCACHED_DRAM_ADDR;

    /* this section will appear last in the binary and only contains the offset
     * to the magic section. It is only useful for the bootloader to locate it
     * and as soon as rockbox starts, this will get overwritten by data or bss.
     * For this reason, we put in a basically non-existent memory (EOF) but still
     * load it in DRAM so that it appears in the binary at the end */
    .endofbinary :
    {
        /* write a magic value and then the offset to the magic section */
        QUAD(0x30636967616d6272) /* 'rbmagic0' in little-endian */
        LONG(_magicoffset)
    } >DRAM

    .dramcopyend (NOLOAD) :
    {
        _dramcopyend = .;
    } > DRAM

    /* the stack overlaps the init and itext region, to reclaim space */
    .stack _dramtextend (NOLOAD) :
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

    .audiobuf (NOLOAD) :
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .codec ENDAUDIOADDR (NOLOAD) :
    {
        audiobufend = .;
        _audiobufend = .;
        codecbuf = .;
        _codecbuf = .;
    } > DRAM

    .plugin ENDADDR (NOLOAD) :
    {
        _pluginbuf = .;
        pluginbuf = .;
    } > DRAM
}
