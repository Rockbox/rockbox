#define ASM
#include "config.h"

ENTRY(start)
#ifdef ROCKBOX_LITTLE_ENDIAN
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-bigarm)
#endif
OUTPUT_ARCH(arm)
STARTUP(target/arm/s5l8702/crt0.o)

#ifdef IPOD_NANO2G
#define DRAMORIG 0x08000000 + ((MEMORYSIZE - 1) * 0x100000)
#define DRAMSIZE 0x00100000
#else
#define DRAMORIG 0x08000000
#define DRAMSIZE (DRAM_SIZE - TTB_SIZE)
#endif

#define IRAMORIG 0x22000000
#define IRAMSIZE 256K

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

#define LOAD_AREA IRAM

SECTIONS
{
#ifdef NEEDS_INTVECT_COPYING
  .intvect : {
    _intvectstart = . ;
    *(.intvect)
    _intvectend = _newstart ;  
  } >IRAM AT> LOAD_AREA
  _intvectcopy = LOADADDR(.intvect) ;
#endif

  .text : {
#ifndef NEEDS_INTVECT_COPYING
    *(.intvect)
#endif
    *(.init.text)
    *(.text*)
    *(.glue_7*)
  } > LOAD_AREA

  .rodata : {
    *(.rodata*)
    . = ALIGN(0x4);
  } > LOAD_AREA

  .data : {
    _datastart = . ;
    *(.irodata)
    *(.icode)
    *(.idata)
    *(.data*)
    *(.ncdata*);
    . = ALIGN(0x4);
    _dataend = . ;
   } > IRAM AT> LOAD_AREA
   _datacopy = LOADADDR(.data) ;

  .stack (NOLOAD) :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
     _irqstackbegin = .;
     . += 0x400;
     _irqstackend = .;
     _fiqstackbegin = .;
     . += 0x400;
     _fiqstackend = .;
  } > IRAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(.ncbss*);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
  } > IRAM
}
