#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

SECTIONS
{
  /*. = IRAMORIG; */

  .text : {
    *(.init.text)
    *(.text*)
  }

  .data : {
    *(.icode)
    *(.irodata)
    *(.idata)
    *(.data*)
    *(.ncdata*);
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
     *(.ncbss*);
     _end = .;
   }
}
