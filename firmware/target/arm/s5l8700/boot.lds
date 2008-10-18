#include "config.h"

ENTRY(_start)
OUTPUT_FORMAT(elf32-bigarm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/s5l8700/crt0.o)

/* DRAMORIG is in fact 0x8000000 but remapped to 0x0 */
#define DRAMORIG 0x8000000
#define DRAMSIZE 16M

#define IRAMORIG 0x22000000
#define IRAMSIZE 256K

#ifdef MEIZU_M6SL
#define DFULOADADDR IRAMORIG
#else
#define DFULOADADDR (IRAMORIG+0x20000)
#endif

/* This is not available in all versions of the S5L8700 */
#define FLASHORIG 0x24000000
#define FLASHSIZE 1M

MEMORY
{
    DRAM  : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
    IRAM  : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    FLASH : ORIGIN = FLASHORIG, LENGTH = FLASHSIZE
}
	

SECTIONS
{
  .intvect : {
    _intvectstart = . ;
    *(.intvect)
    _intvectend = _newstart ;  
  } >IRAM AT> FLASH
  _intvectcopy = LOADADDR(.intvect) ;

  .text : {
    *(.init.text)
    *(.text*)
    *(.glue_7*)
  } > FLASH

  .rodata : {
    *(.rodata*)
    . = ALIGN(0x4);
  } > FLASH

  .data : {
    _datastart = . ;
    *(.irodata)
    *(.icode)
    *(.idata)
    *(.data*)
    *(.ncdata*);
    . = ALIGN(0x4);
    _dataend = . ;
   } > IRAM AT> FLASH
   _datacopy = LOADADDR(.data) ;

  .stack :
  {
     *(.stack)
     _stackbegin = .;
     . += 0x2000;
     _stackend = .;
     _irqstackbegin = .;
     . += 0x400;
     _irqstackend = .;
     _fiqstackbegin = .;
     . += 0x400;
     _fiqstackend = .;
  } > IRAM

  .bss : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(.ncbss*);
     *(COMMON);
    . = ALIGN(0x4);
     _end = .;
   } > IRAM
}
