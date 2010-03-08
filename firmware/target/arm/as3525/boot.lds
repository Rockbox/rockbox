#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

#ifdef SANSA_CLIPV2
#define RAMORIG 0x0             /* DRAM */
#define RAMSIZE (MEM*0x100000)
#else
#define RAMORIG 0x81000000      /* IRAM */
#define RAMSIZE 0x50000
#endif

MEMORY
{
    RAM : ORIGIN = RAMORIG, LENGTH = RAMSIZE
}

SECTIONS
{
  . = RAMORIG;

  .text : {
    *(.init.text)
    *(.glue_7)
    *(.glue_7t)
    *(.text*)
  } > RAM

  .data : {
    *(.icode)
    *(.irodata)
    *(.idata)
    *(.data*)
    *(.ncdata*)
    *(.rodata*)
    _dataend = . ;
   } > RAM

  .stack  (NOLOAD) :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
  } > RAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(.ibss);
     *(COMMON)
     *(.ncbss*);
     _end = .;
   } > RAM
}
