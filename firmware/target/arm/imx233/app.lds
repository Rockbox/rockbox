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

    .dramcopyend (NOLOAD) :
    {
        _dramcopyend = .;
    } > DRAM

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
    }
}
