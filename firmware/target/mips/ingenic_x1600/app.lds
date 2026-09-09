/* Copyright (C) 2026 Michael McAllister
 * Derived from ingenic_x1000/app.lds.
 * Low DRAM holds the vectors and stacks. The bootloader loads above the SPL;
 * the application starts at X1600_DRAM_BASE. */

#include "config.h"
#include "cpu.h"

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(MIPS)
ENTRY(_start)
STARTUP(target/mips/ingenic_x1600/crt0.o)
INPUT(target/mips/exception-mips.o)
INPUT(target/mips/system-mips.o)

#ifdef BOOTLOADER
# undef PLUGIN_BUFFER_SIZE
# undef CODEC_SIZE
# define PLUGIN_BUFFER_SIZE 0
# define CODEC_SIZE 0
# if defined(HAVE_INIT_ATTR)
/* This needs to be fixed for the bootloader */
#  error "bootloader does not support INIT_ATTR"
# endif
/* See the header comment: the SPL occupies low DRAM while it is copying */
# define APP_DRAM_ORIGIN X1600_BOOT_LOAD_ADDR
#else
# define APP_DRAM_ORIGIN X1600_DRAM_BASE
#endif

#define APP_DRAM_LENGTH (X1600_SDRAM_END - APP_DRAM_ORIGIN)

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR (X1600_SDRAM_END - PLUGIN_BUFFER_SIZE - CODEC_SIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDCODECADDR  (ENDAUDIOADDR + CODEC_SIZE)

/* Place init code in the codec buffer */
#define INIT_BASE   ENDAUDIOADDR
#define INIT_SIZE   CODEC_SIZE

MEMORY
{
    IRAM : ORIGIN = X1600_IRAM_BASE, LENGTH = X1600_IRAM_SIZE
    DRAM : ORIGIN = APP_DRAM_ORIGIN, LENGTH = APP_DRAM_LENGTH
    INIT : ORIGIN = INIT_BASE, LENGTH = INIT_SIZE
}

SECTIONS
{
    .startup :
    {
        loadaddress = .;
        _loadaddress = .;
        *(.startup.text);
    } > DRAM

    .text :
    {
        *(.text*);
#ifndef HAVE_INIT_ATTR
        *(.init*);
#endif
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
    } > DRAM

    /*
     * The following sections are loaded after normal DRAM sections
     * but are copied elsewhere by the startup code.
     */
    _noloaddram = ALIGN(4);

    .iram :
    {
        _iramstart = ALIGN(4);
        . = 0x000; /* TLB refill */
        KEEP(*(.vectors.1));
        . = 0x100; /* Cache error */
        KEEP(*(.vectors.2));
        . = 0x180; /* General exception */
        KEEP(*(.vectors.3));
        . = 0x200; /* Interrupt */
        KEEP(*(.vectors.4));
        KEEP(*(.vectors));

        *(.icode*);
        *(.irodata);
        *(.idata);
        _iramend = ALIGN(4);
    } > IRAM AT> DRAM
    _iramcopy = LOADADDR(.iram);

#ifdef HAVE_INIT_ATTR
    .init :
    {
        _initstart = ALIGN(4);
        *(.init*);
        _initend = ALIGN(4);
    } > INIT AT> DRAM
    _initcopy = LOADADDR(.init);
#endif

    /* Sections below have no data */

    . = ALIGN(4);
    .stack (NOLOAD) :
    {
         *(.stack);
         stackbegin = ALIGN(4);
         . += X1600_STACKSIZE;
         stackend = ALIGN(4);
         _irqstackbegin = ALIGN(4);
         . += X1600_IRQSTACKSIZE;
         _irqstackend = ALIGN(4);
    } > IRAM

    .bss _noloaddram (NOLOAD) :
    {
        _bssbegin = ALIGN(4);
        *(.sbss*);
        *(.bss*);
        *(COMMON);
        *(.scommon*);
        _bssend = ALIGN(4);
        _end = .;
    } > DRAM

    .audiobuf :
    {
        . = ALIGN(4);
        audiobuffer = .;
    } > DRAM

    audiobufend = ENDAUDIOADDR;
    codecbuf = ENDAUDIOADDR;
    pluginbuf = ENDCODECADDR;

    /DISCARD/ :
    {
        *(.MIPS.abiflags);
        *(.eh_frame);
        *(.rel.dyn);
    }
}

#if defined(HAVE_NOCROSSREFS_TO) && defined(HAVE_INIT_ATTR)
/* Prevent accidental references to the .init section from other sections */
NOCROSSREFS_TO(.init .text .iram);
#endif
