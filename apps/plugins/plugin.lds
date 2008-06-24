#include "config.h"

/* These output formats should be in the config-files */

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-sh)
#endif

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#if defined(CPU_PP)
#ifdef CPU_PP502x
#define NOCACHE_BASE 	0x10000000
#else
#define NOCACHE_BASE 	0x28000000
#endif /* CPU_* */
#define CACHEALIGN_SIZE 16
#endif /* CPU_PP */

#ifndef NOCACHE_BASE
/* Default to no offset if target doesn't define this */
#define NOCACHE_BASE 0x00000000
#endif

#if CONFIG_CPU==DM320 || CONFIG_CPU==S3C2440
#include "cpu.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - STUBOFFSET - PLUGIN_BUFFER_SIZE - CODEC_SIZE - LCD_BUFFER_SIZE - TTB_SIZE
#elif CONFIG_CPU==IMX31L
#include "imx31l.h"
/* Reserve 1mb for LCD buffer/TTB as in app.lds */
#define DRAMSIZE (MEMORYSIZE * 0x100000 - 0x100000) - PLUGIN_BUFFER_SIZE - STUBOFFSET - CODEC_SIZE
#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - STUBOFFSET - CODEC_SIZE
#endif

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300)
#define ARCH_IRIVER
#endif

#if defined(ARCH_IRIVER) || defined(IAUDIO_M3)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10010000
#define IRAMSIZE 0x10000
#elif CONFIG_CPU == PP5022 || CONFIG_CPU == PP5024
/* PP5022/24 have 128KB of IRAM */
#define DRAMORIG 0x00000000
#define IRAMORIG 0x4000c000
#define IRAMSIZE 0x14000
#elif defined(CPU_PP)
/* all other PP's have 96KB of IRAM */
#define DRAMORIG 0x00000000
#define IRAMORIG 0x4000c000
#define IRAMSIZE 0x0c000
#elif CONFIG_CPU == PNX0101
#define DRAMORIG 0xc00000 + STUBOFFSET
#define IRAMORIG 0x407000
#define IRAMSIZE 0x9000
#elif CONFIG_CPU == S3C2440
#define DRAMORIG 0x0 + STUBOFFSET
#define IRAM DRAM
#define IRAMSIZE 0
#elif CONFIG_CPU == IMX31L
#define DRAMORIG 0x0 + STUBOFFSET
#define IRAM DRAM
#define IRAMSIZE 0
#elif CONFIG_CPU==DM320
#define DRAMORIG 0x00900000 + STUBOFFSET
#define IRAMORIG DRAMORIG
#define IRAMSIZE 0x4000
#define IRAM DRAM
#elif defined(CPU_TCC780X)
#define DRAMORIG 0x20000000
/*#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000*/
#define IRAM DRAM
#define IRAMSIZE 0
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#endif

#define PLUGIN_LENGTH PLUGIN_BUFFER_SIZE

#define CODEC_ORIGIN (DRAMORIG + (DRAMSIZE))
#define PLUGIN_ORIGIN (CODEC_ORIGIN + CODEC_SIZE)

#ifdef CODEC
#define THIS_LENGTH CODEC_SIZE
#define THIS_ORIGIN CODEC_ORIGIN
#else
#define THIS_LENGTH PLUGIN_LENGTH
#define THIS_ORIGIN PLUGIN_ORIGIN
#endif

MEMORY
{
   PLUGIN_RAM : ORIGIN = THIS_ORIGIN, LENGTH = THIS_LENGTH
#if defined(IRAMSIZE) && IRAMSIZE != 0
   PLUGIN_IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
#endif
}

SECTIONS
{
    .header : {
        _plugin_start_addr = .;
        plugin_start_addr = .;
        KEEP(*(.header))
    } > PLUGIN_RAM

    .text :
    {
        *(.text*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
	*(.icode)
#endif
#ifdef CPU_ARM
        *(.glue_7)
        *(.glue_7t)
#endif
    } > PLUGIN_RAM

    .rodata :
    {
        *(.rodata*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
	*(.irodata)
#endif
        . = ALIGN(0x4);
    } > PLUGIN_RAM

    .data :
    {
        *(.data*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
	*(.idata)
#endif
    } > PLUGIN_RAM

#if NOCACHE_BASE != 0
    .ncdata . + NOCACHE_BASE :
    {
        . = ALIGN(CACHEALIGN_SIZE);
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> PLUGIN_RAM
#endif

#if defined(IRAMSIZE)
    iramcopy = . - NOCACHE_BASE;
#endif

    /DISCARD/ :
    {
        *(.eh_frame)
    }

#if defined(IRAMSIZE) && IRAMSIZE != 0
    .iram IRAMORIG : AT ( iramcopy)
    {
        iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        iramend = .;
    } > PLUGIN_IRAM


    .ibss (NOLOAD) :
    {
        iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        iend = .;
    } > PLUGIN_IRAM
#endif

    .bss (NOLOAD) :
    {
    	plugin_bss_start = .;
        *(.bss*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
	*(.ibss)
#endif
        *(COMMON)
        . = ALIGN(0x4);
    } > PLUGIN_RAM
    
#if NOCACHE_BASE != 0
    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
    	. = ALIGN(CACHEALIGN_SIZE);
    	*(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> PLUGIN_RAM
#endif

    /* Restore . */
    .pluginend . - NOCACHE_BASE :
    {
        _plugin_end_addr = .;
        plugin_end_addr = .;
    }

    /* Special trick to avoid a linker error when no other sections are
       left after garbage collection (plugin not for this platform) */
    .comment 0 :
    {
        KEEP(*(.comment))
    }
}
