#include "config.h"
#include "cpu.h"

ENTRY(header)
/* These output formats should be in the config-files */

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
#elif defined(CPU_SH)
OUTPUT_FORMAT(elf32-sh)
#elif defined(CPU_MIPS)
OUTPUT_FORMAT(elf32-littlemips)
#else
/* We can have an #error here we don't use this file when build sims! */
#error Unknown CPU architecture
#endif

#ifndef NOCACHE_BASE
#define NOCACHE_BASE 0
#endif

MEMORY
{
/*   PLUGIN_RAM : ORIGIN = THIS_ORIGIN, LENGTH = THIS_LENGTH */
     PLUGIN_RAM : ORIGIN = 0, LENGTH = 0x7fffffff
#if (PLUGIN_IRAMSIZE != 0)
/*   PLUGIN_IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE */
     PLUGIN_IRAM : ORIGIN = 0x80000000, LENGTH = PLUGIN_IRAMSIZE
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
#if (PLUGIN_IRAMSIZE == 0)
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
#if (PLUGIN_IRAMSIZE == 0)
		*(.irodata)
#endif
        . = ALIGN(0x4);
    } > PLUGIN_RAM

    .data :
    {
        *(.data*)
#if (PLUGIN_IRAMSIZE == 0)
		*(.idata)
#endif
    } > PLUGIN_RAM

#if NOCACHE_BASE != 0
    .ncdata . + NOCACHE_BASE :
    {
        . = ALIGN(CACHEALIGN_SIZE);
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE);
/* EABI currently needs iramcopy defined here, otherwise .iram can sometimes
   have an incorrect load address, breaking codecs. */
#if (PLUGIN_IRAMSIZE != 0)
        iramcopy = . - NOCACHE_BASE;
#endif
    } AT> PLUGIN_RAM
/* This definition is used when NOCACHE_BASE is 0. The address offset bug only
   seems to occur when the empty .ncdata is present. */
#elif (PLUGIN_IRAMSIZE != 0)
    iramcopy = .;
#endif

    /DISCARD/ :
    {
        *(.eh_frame)
#ifdef CPU_MIPS
        *(.rel.dyn)
#endif
    }

#if (PLUGIN_IRAMSIZE != 0)
/*    .iram IRAMORIG : AT ( iramcopy) */
    .iram :
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
    	_plugin_bss_start = .;
        *(.bss*)
#if (PLUGIN_IRAMSIZE == 0)
		*(.ibss)
#endif
        *(COMMON)
        . = ALIGN(0x4);
    } > PLUGIN_RAM

/* make sure this section is not empty */    
#if NOCACHE_BASE != 0
    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
    	. = ALIGN(CACHEALIGN_SIZE);
    	*(.ncbss*)
        LONG(-1);
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
