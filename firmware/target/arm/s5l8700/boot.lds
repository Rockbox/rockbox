#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-bigarm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

/* DRAMORIG is in fact 0x8000000 but remapped to 0x0 */
#define DRAMORIG 0x0
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

SECTIONS
{
  /*. = IRAMORIG; */
  /* As long as we don't flash the code, use the DFU load address */
  . = DFULOADADDR;

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
     . += 0x1000;
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
