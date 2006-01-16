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

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300)
#define ARCH_IRIVER
#endif

#if CONFIG_CPU==PP5020
#define ARCH_IPOD
#endif

#ifdef ARCH_IRIVER
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
#elif defined(ARCH_IPOD)
#define DRAMORIG 0x00000000
#define IRAMORIG 0x4000c000
#define IRAMSIZE 0xc000
#elif CONFIG_CPU == PNX0101
#define DRAMORIG 0x24000000
#define IRAMORIG 0x408000
#define IRAMSIZE 0x8000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#endif

#define PLUGIN_LENGTH PLUGIN_BUFFER_SIZE
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_LENGTH - CODEC_SIZE
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
#if defined(ARCH_IRIVER) || defined(ARCH_IPOD) || (CONFIG_CPU==PNX0101)
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
        KEEP(*(.entry))
        *(.text*)
#if (CONFIG_CPU==PP5020) || (CONFIG_CPU==PNX0101)
        *(.glue_7)
        *(.glue_7t)
#endif
    } > PLUGIN_RAM

    .rodata : 
    {
        *(.rodata*)
        . = ALIGN(0x4);
    } > PLUGIN_RAM

    .data :
    {
        *(.data*)
#if defined(ARCH_IRIVER) || defined(ARCH_IPOD) || (CONFIG_CPU==PNX0101)
        iramcopy = .;
#endif
    } > PLUGIN_RAM

    /DISCARD/ : 
    {
        *(.eh_frame)
    }

#if defined(ARCH_IRIVER) || defined(ARCH_IPOD) || (CONFIG_CPU==PNX0101)
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

    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
#else
    .bss :
#endif
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        _plugin_end_addr = .;
        plugin_end_addr = .;
    } > PLUGIN_RAM

    /* Special trick to avoid a linker error when no other sections are
       left after garbage collection (plugin not for this platform) */
    .comment 0 : 
    {
        KEEP(*(.comment))
    }
}
