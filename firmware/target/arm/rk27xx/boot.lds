#include "config.h"

ENTRY(start)
#ifdef ROCKBOX_LITTLE_ENDIAN
OUTPUT_FORMAT(elf32-littlearm)
#else
OUTPUT_FORMAT(elf32-bigarm)
#endif
OUTPUT_ARCH(arm)
STARTUP(target/arm/rk27xx/crt0.o)

#define DRAMORIG 0x60000000
#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define IRAMORIG 0x00000000
#define IRAMSIZE 4K

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
}

SECTIONS
{
  .intvect : {
    _intvectstart = . ;
    *(.intvect)
    _intvectend = _newstart ;  
  } > IRAM AT > DRAM
  _intvectcopy = LOADADDR(.intvect) ;

  .text : {
    *(.init.text)
    *(.text*)
    *(.glue_7*)
  } > DRAM

  .data : {
    *(.rodata*)
    *(.data*)
    *(.ncdata*);
    . = ALIGN(0x4);
  } > DRAM

  .idata : {
    _datastart = . ;
    *(.irodata)
    *(.icode)
    *(.idata)
    . = ALIGN(0x4);
    _dataend = . ;
   } > DRAM
   _datacopy = LOADADDR(.idata) ;

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
     *(.ncbss*);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
  } > DRAM
}
