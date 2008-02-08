#include "config.h"

ENTRY(start)
#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
INPUT(target/coldfire/crt0.o)
#elif defined(CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
#ifdef CPU_PP
INPUT(target/arm/crt0-pp.o)
#elif CONFIG_CPU==DM320
INPUT(target/arm/tms320dm320/crt0.o)
#elif CONFIG_CPU==S3C2440
INPUT(target/arm/s3c2440/crt0.o)
#elif defined(CPU_TCC780X)
INPUT(target/arm/tcc780x/crt0.o)
#elif CONFIG_CPU == PNX0101
INPUT(target/arm/pnx0101/crt0-pnx0101.o)
#elif CONFIG_CPU == IMX31L
INPUT(target/arm/imx31/crt0.o)
#elif defined(CPU_ARM)
INPUT(target/arm/crt0.o)
#endif
#else
OUTPUT_FORMAT(elf32-sh)
INPUT(target/sh/crt0.o)
#endif

#define PLUGINSIZE PLUGIN_BUFFER_SIZE
#define CODECSIZE CODEC_SIZE

#ifdef DEBUG
#define STUBOFFSET 0x10000
#else
#define STUBOFFSET 0
#endif

#if CONFIG_CPU==S3C2440
#include "s3c2440.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - 0x100 - PLUGINSIZE - STUBOFFSET - CODECSIZE - LCD_BUFFER_SIZE - TTB_SIZE
#elif CONFIG_CPU==IMX31L
#include "imx31l.h"
/* Subtract 1MB for the FRAME/TTB section */
#define DRAMSIZE (MEMORYSIZE * 0x100000 - 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE
#elif CONFIG_CPU==DM320
#include "dm320.h"
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE - LCD_BUFFER_SIZE - TTB_SIZE
#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE - STUBOFFSET - CODECSIZE
#endif

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300)
#define DRAMORIG 0x31000000 + STUBOFFSET
#define IRAMORIG 0x10000000
#define IRAMSIZE 0xc000
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define DRAMORIG 0x31000000 + STUBOFFSET
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x10000
#elif defined(CPU_PP)
#define DRAMORIG 0x00000000 + STUBOFFSET
#define IRAMORIG 0x40000000
#define IRAMSIZE 0xc000
#elif CONFIG_CPU==PNX0101
#define DRAMORIG 0xc00000 + STUBOFFSET
#define IRAM0ORIG 0x000000
#define IRAM0SIZE 0x7000
#define IRAMORIG 0x400000
#define IRAMSIZE 0x7000
#elif CONFIG_CPU==S3C2440
#define DRAMORIG 0x00000100 + STUBOFFSET
#define IRAMORIG DRAMORIG
#define IRAM DRAM
#define IRAMSIZE 0x1000
#elif CONFIG_CPU==DM320
#define DRAMORIG 0x00900000 + STUBOFFSET
#define IRAMORIG 0x00000000
#define IRAMSIZE 0x4000
#elif CONFIG_CPU==IMX31L
#define DRAMORIG (0x0 + STUBOFFSET)
/* #define IRAMORIG 0x1FFFC000 */
#define IRAMORIG DRAMORIG
#define IRAM     DRAM
#define IRAMSIZE 0x4000
#elif defined(CPU_TCC780X)
#define DRAMORIG 0x20000000 + STUBOFFSET
#define ITCMORIG 0x00000000
#define ITCMSIZE 0x1000
#define DTCMORIG 0xA0000000
#define DTCMSIZE 0x2000
#define SRAMORIG 0x10000000
#define SRAMSIZE 0xc000
#else
#define DRAMORIG 0x09000000 + STUBOFFSET
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#endif

/* End of the audio buffer, where the codec buffer starts */
#define ENDAUDIOADDR  (DRAMORIG + DRAMSIZE)

/* Where the codec buffer ends, and the plugin buffer starts */
#define ENDADDR (ENDAUDIOADDR + CODECSIZE)

MEMORY
{
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
#if defined(CPU_TCC780X)
    /* Seperate data & instruction TCMs plus SRAM. */
    ITCM : ORIGIN = ITCMORIG, LENGTH = ITCMSIZE
    DTCM : ORIGIN = DTCMORIG, LENGTH = DTCMSIZE
    SRAM : ORIGIN = SRAMORIG, LENGTH = SRAMSIZE
#elif CONFIG_CPU != S3C2440 && CONFIG_CPU != IMX31L
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
#endif
#if CONFIG_CPU==PNX0101
    IRAM0 : ORIGIN = IRAM0ORIG, LENGTH = IRAM0SIZE
#endif
}

SECTIONS
{
#if (CONFIG_CPU==DM320)
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
        . = ALIGN(0x200);
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

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .vectors IRAMORIG :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } > IRAM AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

    .iram :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        . = ALIGN(0x4);        
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > IRAM

    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram) + SIZEOF(.vectors):
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

#elif defined(CPU_TCC780X)
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
        . = ALIGN(0x200);
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

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .vectors ITCMORIG :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } > ITCM AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

    .itcm :
    {
        _itcmstart = .;
        *(.icode)
        _itcmend = .;
    } > ITCM AT> DRAM

    _itcmcopy = LOADADDR(.itcm);
    
    .dtcm :
    {
        _dtcmstart = .;
        *(.irodata)
        *(.idata)
        _dtcmend = .;
    } > DTCM AT> DRAM

    _dtcmcopy = LOADADDR(.dtcm);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > SRAM

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > SRAM

    .bss :
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

#elif CONFIG_CPU==S3C2440 || CONFIG_CPU == IMX31L
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
        . = ALIGN(0x200);
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

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .vectors 0x0 :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
    } AT> DRAM

    _vectorscopy = LOADADDR(.vectors);

    .iram :
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > DRAM

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > DRAM

    .bss :
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM

#else /* End CONFIG_CPU */

#if !defined(CPU_ARM)
    .vectors :
    {
        loadaddress = .;
        _loadaddress = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
    } > DRAM

    .text :
    {
#else
    .text :
    {
        loadaddress = .;
        _loadaddress = .;
#endif
        . = ALIGN(0x200);
        *(.init.text)
        *(.text*)
#ifdef CPU_ARM
        *(.glue_7)
        *(.glue_7t)
#endif
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata)  /* problems without this, dunno why */
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);

        /* Pseudo-allocate the copies of the data sections */
        _datacopy = .;
    } > DRAM

    /* TRICK ALERT! For RAM execution, we put the .data section at the
       same load address as the copy. Thus, we don't waste extra RAM
       when we don't actually need the copy.  */
    .data : AT ( _datacopy )
    {
        _datastart = .;
        *(.data*)
        . = ALIGN(0x4);
        _dataend  = .;
    } > DRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    }

#if defined(CPU_ARM)
    .vectors 0x0 :
    {
        _vectorsstart = .;
        *(.vectors);
        _vectorsend = .;
#if CONFIG_CPU==PNX0101
        *(.dmabuf)
    } >IRAM0 AT> DRAM
#else
    } AT> DRAM
#endif

    _vectorscopy = LOADADDR(.vectors);
#endif

#if CONFIG_CPU==PNX0101
    .iram IRAMORIG + SIZEOF(.vectors) :
#else
    .iram IRAMORIG :
#endif
    {
        _iramstart = .;
        *(.icode)
        *(.irodata)
        *(.idata)
        _iramend = .;
    } > IRAM AT> DRAM

    _iramcopy = LOADADDR(.iram);

    .ibss (NOLOAD) :
    {
        _iedata = .;
        *(.ibss)
        . = ALIGN(0x4);
        _iend = .;
    } > IRAM

#if defined(CPU_COLDFIRE) || defined(CPU_ARM)
#ifdef CPU_PP
    .idle_stacks :
    {
       *(.idle_stacks)
#if NUM_CORES > 1
       cpu_idlestackbegin = .;
       . += IDLE_STACK_SIZE;
       cpu_idlestackend = .;
#endif
       cop_idlestackbegin = .;
       . += IDLE_STACK_SIZE;
       cop_idlestackend = .;
    } > IRAM
#endif

    .stack :
    {
       *(.stack)
       stackbegin = .;
       . += 0x2000;
       stackend = .;
    } > IRAM

#else
    /* TRICK ALERT! We want 0x2000 bytes of stack, but we set the section
       size smaller, and allow the stack to grow into the .iram copy */
    .stack ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
    {
       *(.stack)
       _stackbegin = . - SIZEOF(.iram);
       . += 0x2000 - SIZEOF(.iram);
       _stackend = .;
    } > DRAM
#endif

#if defined(CPU_COLDFIRE)
    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram):
#elif defined(CPU_ARM)
    .bss ADDR(.data) + SIZEOF(.data) + SIZEOF(.iram) + SIZEOF(.vectors):
#else
    .bss :
#endif
    {
       _edata = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
       _end = .;
    } > DRAM
    
#endif

    .audiobuf ALIGN(4) :
    {
        _audiobuffer = .;
        audiobuffer = .;
    } > DRAM

    .audiobufend ENDAUDIOADDR:
    {
        audiobufend = .;
        _audiobufend = .;
    } > DRAM

    .codec ENDAUDIOADDR:
    {
        codecbuf = .;
        _codecbuf = .;
    }

    .plugin ENDADDR:
    {
        _pluginbuf = .;
        pluginbuf = .;
    }
}

