#include "config.h"

/* These output formats should be in the config-files */

#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
#elif defined(CPU_MIPS)
OUTPUT_FORMAT(elf32-littlemips)
#else
/* We can have an #error here we don't use this file when build sims! */
#error Unknown CPU architecture
#endif

#if defined(CPU_PP)
#ifdef CPU_PP502x
#define NOCACHE_BASE 	0x10000000
#else
#define NOCACHE_BASE 	0x28000000
#endif /* CPU_* */
#define CACHEALIGN_SIZE 16
#endif /* CPU_PP */

#if CONFIG_CPU==IMX31L
/* No fudges! */
#include "imx31l.h"
#define DRAMSIZE ((MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE \
                  - CODEC_SIZE - QHARRAY_SIZE - FRAME_SIZE - TTB_SIZE)

#elif CONFIG_CPU==DM320

#ifndef LCD_NATIVE_WIDTH
#define LCD_NATIVE_WIDTH LCD_WIDTH
#endif

#ifndef LCD_NATIVE_HEIGHT
#define LCD_NATIVE_HEIGHT LCD_HEIGHT
#endif

#ifdef MROBE_500
/* Give this 1 meg to allow it to align to the MMU boundary */
#define LCD_FUDGE       LCD_NATIVE_WIDTH%32
#define LCD_BUFFER_SIZE ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)
#define LCD_TTB_AREA    0x100000*((LCD_BUFFER_SIZE>>19)+1)
#else
/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE        (0x4000)
#define LCD_BUFFER_SIZE (LCD_NATIVE_WIDTH*LCD_NATIVE_HEIGHT*2)
#define LCD_TTB_AREA    (TTB_SIZE + LCD_BUFFER_SIZE)
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - CODEC_SIZE - LCD_TTB_AREA

#elif CONFIG_CPU==S3C2440
#include "cpu.h"
/* must be 16Kb (0x4000) aligned */
#define TTB_SIZE         (0x4000)
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - CODEC_SIZE - LCD_BUFFER_SIZE - TTB_SIZE

#elif CONFIG_CPU==TCC7801
#include "cpu.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - CODEC_SIZE - TTB_SIZE

#elif CONFIG_CPU==AS3525 || CONFIG_CPU==AS3525v2
#include "cpu.h"
#define DRAMORIG DRAM_ORIG
#if defined(AMS_LOWMEM) || (CONFIG_CPU == AS3525v2)
#define DRAMSIZE (DRAM_SIZE - PLUGIN_BUFFER_SIZE - TTB_SIZE)
#else
#define DRAMSIZE (DRAM_SIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE - TTB_SIZE)
#endif
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define ASM
#include "cpu.h"
#define DRAMSIZE (DRAM_SIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE - TTB_SIZE)
#elif CONFIG_CPU==IMX233
#include "cpu.h"
#define DRAMSIZE (DRAM_SIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE - FRAME_SIZE - TTB_SIZE)
#elif CONFIG_CPU == JZ4732 || CONFIG_CPU == JZ4760B
/* fake 'iram' region occupies first 16k */
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - CODEC_SIZE - 0x4000
#elif CONFIG_CPU==X1000
#include "cpu.h"
#define DRAMSIZE (X1000_DRAM_SIZE - PLUGIN_BUFFER_SIZE - CODEC_SIZE)
#endif

 /* default to full RAM (minus codecs&plugins) unless specified otherwise */
#ifndef DRAMSIZE
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGIN_BUFFER_SIZE - CODEC_SIZE
#endif

/* MCF5249 have 96KB of IRAM */
#if CONFIG_CPU == MCF5249
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000

/* MCF5250 have 128KB of IRAM */
#elif CONFIG_CPU == MCF5250
#define DRAMORIG 0x31000000
#define IRAMORIG 0x1000c800
#define IRAMSIZE 0x13800

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
#define DRAMORIG 0xc00000
#define IRAMORIG 0x407000
#define IRAMSIZE 0x9000

#elif CONFIG_CPU == IMX31L || CONFIG_CPU == S3C2440
#define DRAMORIG 0x0
#define IRAM DRAM
#define IRAMSIZE 0

#elif CONFIG_CPU==DM320
#define DRAMORIG CONFIG_SDRAM_START
#define IRAM DRAM
/* The bit of IRAM that is available is used in the core */
#define IRAMSIZE 0

#elif defined(CPU_TCC780X)
#define DRAMORIG 0x20000000
#if CONFIG_CPU==TCC7801
#define IRAMORIG 0x1000c000
#define IRAMSIZE 0xc000
#else
#define IRAM DRAM
#define IRAMSIZE 0
#endif

#elif CONFIG_CPU==AS3525 || CONFIG_CPU==AS3525v2
#if defined(AMS_LOWMEM) || (CONFIG_CPU == AS3525v2)
#define IRAMSIZE 0  /* simulates no IRAM since codec is already entirely in IRAM */
#define CODEC_ORIGIN (IRAM_ORIG + IRAM_SIZE - CODEC_SIZE)
#define PLUGIN_ORIGIN (DRAM_ORIG + DRAMSIZE)
#else
#define IRAMORIG (IRAM_ORIG + 0x20000)
#define IRAMSIZE (IRAM_ORIG + IRAM_SIZE - IRAMORIG)
#endif

#elif CONFIG_CPU==S5L8700
/* S5L8700 have 256KB of IRAM */
#define DRAMORIG 0x08000000
#define IRAMORIG (0x00000000 + (48*1024))
#define IRAMSIZE (208*1024)

#elif CONFIG_CPU==S5L8701
/* S5L8701 have 176KB of IRAM */
#define DRAMORIG 0x08000000
#define IRAMORIG (0x00000000 + (48*1024))
#define IRAMSIZE (128*1024)

#elif CONFIG_CPU==S5L8702
/* S5L8702 have 256KB of IRAM */
/* IRAM1 (2nd 128KB block) is not used because it is slower than DRAM */
#define DRAMORIG 0x08000000
#define IRAMORIG (0x00000000 + (48*1024))
#define IRAMSIZE (80*1024)

#elif CONFIG_CPU==S5L8720
/* S5L8720 have 192KB of IRAM */
/* TODO: check if IRAM1 is also slow as in S5L8702 above */
#define DRAMORIG 0x08000000
#define IRAMORIG (0x00000000 + (56*1024))
#define IRAMSIZE (136*1024)

#elif CONFIG_CPU == JZ4732 || CONFIG_CPU == JZ4760B
#define DRAMORIG 0x80004000
#define IRAM DRAM
#define IRAMSIZE 0
/* The bit of IRAM that is available is used in the core */

#elif CONFIG_CPU == X1000
#define DRAMORIG X1000_DRAM_BASE
#define IRAM DRAM
#define IRAMSIZE 0

#elif CONFIG_CPU == RK27XX
#define DRAMORIG 0x60000000
#define IRAM DRAM
#define IRAMSIZE 0

#elif CONFIG_CPU == IMX233
#include "cpu.h"
/* The IRAM is too small and already partly used by the core */
#define DRAMORIG CACHED_DRAM_ADDR
#define IRAM DRAM
#define IRAMSIZE 0

#elif CONFIG_CPU == STM32H743
#include "cpu.h"
#define DRAMORIG STM32_SDRAM1_BASE
#define IRAM     DRAM
#define IRAMSIZE 0

#else
#define DRAMORIG 0x09000000
#endif

#ifndef NOCACHE_BASE
/* Default to no offset if target doesn't define this */
#define NOCACHE_BASE 0x00000000
#endif

#define PLUGIN_LENGTH PLUGIN_BUFFER_SIZE


#ifndef CODEC_ORIGIN /* targets can specify another origin */
#define CODEC_ORIGIN (DRAMORIG + (DRAMSIZE))
#endif

#ifndef PLUGIN_ORIGIN /* targets can specify another origin */
#define PLUGIN_ORIGIN (CODEC_ORIGIN + CODEC_SIZE)
#endif

#ifdef CODEC
#define THIS_LENGTH CODEC_SIZE
#define THIS_ORIGIN CODEC_ORIGIN
#elif defined OVERLAY_OFFSET
#define THIS_LENGTH (DRAMSIZE - OVERLAY_OFFSET)
#define THIS_ORIGIN (DRAMORIG + OVERLAY_OFFSET)
#elif defined IMGVDECODER_OFFSET
#define THIS_LENGTH (PLUGIN_LENGTH - IMGVDECODER_OFFSET)
#define THIS_ORIGIN (PLUGIN_ORIGIN + IMGVDECODER_OFFSET)
#else /* plugin */
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
    } > PLUGIN_RAM

    .data :
    {
        *(.data*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
        *(.idata)
#endif
    } > PLUGIN_RAM

#if NOCACHE_BASE != 0
    /*
     * Allocate .ncdata based on the following constraints:
     *
     * 1. the LMA must be allocated at the next free LMA in
     *    PLUGIN_RAM and aligned to a cacheline boundary.
     * 2. the VMA - LMA offset must equal NOCACHE_BASE.
     */
    .ncdata ALIGN(. + NOCACHE_BASE, CACHEALIGN_SIZE) : ALIGN(CACHEALIGN_SIZE)
    {
        *(.ncdata*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> PLUGIN_RAM
#endif

    /DISCARD/ :
    {
        *(.eh_frame)
#ifdef CPU_MIPS
        *(.MIPS.abiflags)
        *(.rel.dyn)
#endif
    }

    .bss (NOLOAD) :
    {
    	plugin_bss_start = .;
    	_plugin_bss_start = .;
        *(.bss*)
#if defined(IRAMSIZE) && IRAMSIZE == 0
        *(.ibss)
#endif
        *(COMMON)
        . = ALIGN(0x4);
    } > PLUGIN_RAM

#if NOCACHE_BASE != 0
    /*
     * .ncbss has the same constraints as the .ncdata section
     * above but there is an extra complication: because it is
     * a NOLOAD section, unlike .ncdata it cannot consume any
     * VMAs or LMAs in PLUGIN_RAM itself.
     *
     * Because free VMA/LMAs in PLUGIN_RAM are unchanged, any
     * subsequent sections will get placed at the end of the
     * .bss section, overlapping .ncbss, which we don't want.
     *
     * One solution is to manually place the next sections to
     * account for the size of .ncbss, but a cleaner solution
     * is to define a dummy section (.ncbss_vma) allocated at
     * the same VMA as .ncbss and equal in size, and placed in
     * the PLUGIN_RAM region. This allocates the correct VMA
     * range in plugin RAM so that the next section will not
     * overlap .ncbss.
     */
    .ncbss ALIGN(. + NOCACHE_BASE, CACHEALIGN_SIZE) (NOLOAD) : ALIGN(CACHEALIGN_SIZE)
    {
    	*(.ncbss*)
        . = ALIGN(CACHEALIGN_SIZE);
    } AT> PLUGIN_RAM

    .ncbss_vma (NOLOAD) : ALIGN(CACHEALIGN_SIZE)
    {
        . += SIZEOF(.ncbss);
    } > PLUGIN_RAM
#endif

    /* Final end of plugin after IRAM setup. The plugin or codec buffer
       is considered unused by the in-RAM image at this point once IRAM
       is copied. */
    .pluginend (NOLOAD) :
    {
        _plugin_end_addr = .;
        plugin_end_addr = .;
    } > PLUGIN_RAM

#if defined(IRAMSIZE) && IRAMSIZE != 0
    .iram IRAMORIG : AT(LOADADDR(.bss))
    {
        iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        iramend = .;
    } > PLUGIN_IRAM
    iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        iend = .;
    } > PLUGIN_IRAM

    loadendaddr = MAX(plugin_end_addr, LOADADDR(.iram) + SIZEOF(.iram));
#else
    loadendaddr = plugin_end_addr;
#endif

    /* This is for ovl_offset.pl and is the highest address that must
       be loaded into the plugin buffer (past the end of last data in
       stored image). */
    .pluginloadend loadendaddr :
    {
    	_plugin_load_end_addr = .;
    	plugin_load_end_addr = .;
    }

    /* Special trick to avoid a linker error when no other sections are
       left after garbage collection (plugin not for this platform) */
    .comment 0 :
    {
        KEEP(*(.comment))
    }
}

#if NOCACHE_BASE != 0
/* Some asserts to make sure nocache sections appear correctly defined */
ASSERT(LOADADDR(.ncdata) == ADDR(.ncdata) - NOCACHE_BASE, ".ncdata has incorrect LMA/VMA address");
ASSERT(LOADADDR(.ncdata) % CACHEALIGN_SIZE == 0,          ".ncdata incorrectly aligned");
ASSERT(LOADADDR(.ncbss) == ADDR(.ncbss) - NOCACHE_BASE,   ".ncbss has incorrect LMA/VMA address");
ASSERT(LOADADDR(.ncbss) % CACHEALIGN_SIZE == 0,           ".ncbss incorrectly aligned");
ASSERT(ADDR(.ncbss_vma) == LOADADDR(.ncbss),              ".ncbss_vma has wrong address");
ASSERT(SIZEOF(.ncbss_vma) == SIZEOF(.ncbss),              ".ncbss_vma has wrong size");
#endif
