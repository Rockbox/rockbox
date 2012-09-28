#include "config.h"
#include "cpu.h"

ENTRY(plugin__start)
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
    .text :
    {
        _plugin_start_addr = .;
        plugin_start_addr = .;
        KEEP(*(.header))

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
    . = ALIGN(4);

    } > PLUGIN_RAM

    .data :
    {
        *(.data*)
#if (PLUGIN_IRAMSIZE == 0)
	*(.idata)
#endif
    } > PLUGIN_RAM

#if NOCACHE_BASE != 0
    . = ALIGN(CACHEALIGN_SIZE);

    .ncdata . + NOCACHE_BASE :
    {
        *(.ncdata*)
    } AT> PLUGIN_RAM

    . = ALIGN(CACHEALIGN_SIZE);
#endif

    /DISCARD/ :
    {
        *(.eh_frame)
#ifdef CPU_MIPS
        *(.rel.dyn)
#endif
    }

#if (PLUGIN_IRAMSIZE != 0)
    .iram :
    {
        *(.icode)
        *(.irodata)
        *(.idata)
    } > PLUGIN_IRAM

    .ibss (NOLOAD) :
    {
        *(.ibss)
        . = ALIGN(4);
    } > PLUGIN_IRAM

#endif
    .bss (NOLOAD) :
    {
        *(.bss*)
#if (PLUGIN_IRAMSIZE == 0)
	*(.ibss)
#endif
        . = ALIGN(4);

#if (NOCACHE_BASE == 0)
        _plugin_end_addr = .;
        plugin_end_addr = .;
#endif
    } > PLUGIN_RAM

#if (NOCACHE_BASE != 0)
    .ncbss . + NOCACHE_BASE (NOLOAD) :
    {
        . = ALIGN(CACHEALIGN_SIZE);
    	*(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE);
        _plugin_end_addr = . - NOCACHE_BASE;
        plugin_end_addr = . - NOCACHE_BASE;
    } AT> PLUGIN_RAM
#endif

    /* Special trick to avoid a linker error when no other sections are
       left after garbage collection (plugin not for this platform) */
    .comment 0 :
    {
        KEEP(*(.comment))
    }
}
