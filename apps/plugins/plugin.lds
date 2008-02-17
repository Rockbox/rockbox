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

#if CONFIG_CPU==S3C2440
#include "s3c2440.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - 0x100 - PLUGIN_BUFFER_SIZE - STUBOFFSET - CODEC_SIZE - LCD_BUFFER_SIZE - TTB_SIZE
#elif CONFIG_CPU==DM320
#include "dm320.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - STUBOFFSET - CODEC_SIZE - LCD_BUFFER_SIZE - TTB_SIZE
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

#ifdef ARCH_IRIVER
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10010000
#define IRAMSIZE 0x10000
#elif defined(CPU_PP)
#define DRAMORIG 0x00000000
#define IRAMORIG 0x4000c000
#define IRAMSIZE 0xc000
#elif CONFIG_CPU == PNX0101
#define DRAMORIG 0xc00000 + STUBOFFSET
#define IRAMORIG 0x407000
#define IRAMSIZE 0x9000
#elif CONFIG_CPU == S3C2440
#define DRAMORIG 0x100 + STUBOFFSET
#define IRAMORIG DRAMORIG
#define IRAMSIZE 4K
#define IRAM DRAM
#elif CONFIG_CPU == IMX31L
#define DRAMORIG 0x0
#define IRAMORIG 0x1FFFC000
#define IRAMSIZE 0x4000
#elif CONFIG_CPU==DM320
#define DRAMORIG 0x00900000 + STUBOFFSET
#define IRAMORIG DRAMORIG
#define IRAMSIZE 0x4000
#define IRAM DRAM
#elif defined(CPU_TCC780X)
#define DRAMORIG 0x20000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
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
#if defined(IRAMSIZE)
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
#ifdef CPU_ARM
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
#if defined(IRAMSIZE)
        iramcopy = .;
#endif
    } > PLUGIN_RAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

#if defined(IRAMSIZE)
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
