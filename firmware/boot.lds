#include "config.h"

ENTRY(start)
#ifdef CPU_COLDFIRE
OUTPUT_FORMAT(elf32-m68k)
INPUT(target/coldfire/crt0.o)
#elif defined (CPU_ARM)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
#ifdef CPU_PP
INPUT(target/arm/crt0-pp-bl.o)
#else
INPUT(target/arm/crt0.o)
#endif
#else
OUTPUT_FORMAT(elf32-sh)
INPUT(target/sh/crt0.o)
#endif

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#ifdef IRIVER_H100_SERIES
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#elif defined(IRIVER_H300_SERIES)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x003f0000
#define FLASHSIZE 4M
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define DRAMORIG 0x31000000
#define IRAMORIG 0x10000000
#define IRAMSIZE 0x20000
#define FLASHORIG 0x00010000
#define FLASHSIZE 4M
#elif CONFIG_CPU == PP5020
#define DRAMORIG 0x10000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#elif CONFIG_CPU == PP5024
#define DRAMORIG 0x10000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#elif CONFIG_CPU == S3C2440
#define DRAMORIG 0x30000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 4K
#define FLASHORIG 0x0000000
#define FLASHSIZE 1M
#elif CONFIG_CPU == PP5002
#define DRAMORIG 0x28000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M
#else
#define DRAMORIG 0x09000000
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x1000
#define FLASHORIG 0x02000000 + ROM_START
#define FLASHSIZE 256K - ROM_START
#endif

#if !defined(CPU_PP) && (CONFIG_CPU!=S3C2440)
MEMORY
{
   DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
   IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
   FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}
#endif

SECTIONS
#ifdef CPU_PP
{
  . = IRAMORIG;

  .text : { 
    *(.init.text)
    *(.text*)
  }

  .data : { 
    *(.icode)
    *(.irodata)
    *(.idata)
    *(.data*)
    _dataend = . ;
   }

  .stack :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
  }

  /* The bss section is too large for IRAM - we just move it 16MB into the
     DRAM */

  . = (DRAMORIG+16*1024*1024);
  .bss : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     _end = .;
   }
}
#elif (CONFIG_CPU==S3C2440)
{
  . = DRAMORIG + 0x1000000;
  .text : {
    *(.init.text)
    *(.text*)
  }
  .data : {
    *(.icode)
    *(.irodata)
    *(.idata)
    *(.data*)
    _dataend = . ;
   }
  .stack :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
  }
  .bss : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     _end = .;
   }
}
#else
{
    .vectors :
    {
#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
        *(.init.text)
#endif
        _datacopy = .;
    } > FLASH

    .data : AT ( _datacopy )
    {
        _datastart = .;
        KEEP(*(.resetvectors));
        *(.resetvectors);
        KEEP(*(.vectors));
        *(.vectors);
        . = ALIGN(0x200);
        *(.icode)
        *(.irodata)
        *(.idata)
        *(.data*)
        . = ALIGN(0x4);
        _dataend = .;         
        . = ALIGN(0x10);  /* Maintain proper alignment for .text section */
    } > IRAM
    
    /* TRICK ALERT! Newer versions of the linker don't allow output sections
       to overlap even if one of them is empty, so advance the location pointer
       "by hand" */
    .text LOADADDR(.data) + SIZEOF(.data) :
    {
        *(.init.text)
        *(.text*)
        . = ALIGN(0x4);
    } > FLASH

    .rodata :
    {
        *(.rodata*)
        . = ALIGN(0x4);
        _iramcopy = .;
    } > FLASH

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       stackbegin = .;
       . += 0x2000;
       _stackend = .;
       stackend = .;
    } > IRAM

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES) \
 || defined(IAUDIO_X5) || defined(IAUDIO_M5)
    .bss DRAMORIG+0x800000:
#else
    .bss :
#endif
    {
       _edata = .;
        *(.ibss)
        *(.bss*)
        *(COMMON)
       _end = .;
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES) \
 || defined(IAUDIO_X5) || defined(IAUDIO_M5)
    } > DRAM
#else
    } > IRAM
#endif

}
#endif
