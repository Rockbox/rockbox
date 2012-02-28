#include "config.h"

ENTRY(start)
#ifdef ROCKBOX_LITTLE_ENDIAN
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-bigarm)
#endif
OUTPUT_ARCH(arm)
STARTUP(target/arm/rk27xx/crt0.o)

#define DRAMORIG 0x60700000
#define DRAMSIZE (MEMORYSIZE * 0x100000 - 0x700000)

#define IRAMORIG 0x00000000
#define IRAMSIZE 4K

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
  .relocstart (NOLOAD) : {
    _relocstart = .;
  } > DRAM

  .text : {
    *(.init.text)
    *(.text*)
    *(.icode*)
    *(.glue_7*)
  } > DRAM 

  .intvect : {
    _intvectstart = . ;
    KEEP(*(.intvect))
    _intvectend = . ;  
  } > IRAM AT > DRAM
  _intvectcopy = LOADADDR(.intvect) ;

  .rodata : {
    *(.rodata*)
    *(.irodata*)
    . = ALIGN(0x4);
  } > DRAM

  .data : {
    *(.data*)
    *(.idata*)
    . = ALIGN(0x4);
  } > DRAM

  .relocend (NOLOAD) : {
    _relocend = .;
  } > DRAM

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
  } > DRAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
  } > DRAM
}
