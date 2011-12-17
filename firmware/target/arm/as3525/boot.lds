#include "config.h"
#include "cpu.h"

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/crt0.o)

#define LOAD_SIZE 0x100000  /* Reserve 1MB for loading the firmware */

MEMORY
{
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
    DRAM : ORIGIN = DRAM_ORIG + LOAD_SIZE, LENGTH = DRAM_SIZE - LOAD_SIZE - TTB_SIZE
}

SECTIONS
{
  . = IRAM_ORIG;

  .text : {
    *(.init.text)
    *(.glue_7)
    *(.glue_7t)
    *(.text*)
    *(.icode*)
  } > IRAM

  .data : {
    *(.data*)
    *(.rodata*)
    _dataend = . ;
   } > IRAM

  .stack  (NOLOAD) :
  {
     *(.stack)
     _stackbegin = .;
     stackbegin = .;
     . += 0x2000;
     _stackend = .;
     stackend = .;
  } > IRAM

  .bss (NOLOAD) : {
     _edata = .;
     *(.bss*);
     *(COMMON)
     _end = .;
   } > DRAM
}
