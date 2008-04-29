#include "config.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0-pp-bl.o)

#define DRAMSIZE (MEMORYSIZE * 0x100000)

#define DRAMORIG 0x10000000
#define IRAMORIG 0x40000000
#define IRAMSIZE 0x18000
#define FLASHORIG 0x001f0000
#define FLASHSIZE 2M

SECTIONS
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

  /* The bss section is too large for IRAM - we just move it 16MB into the
     DRAM */

  . = (DRAMORIG+16*1024*1024);
  .bss : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(.ncbss*);
     _end = .;
   }
}
