#include "config.h"
ENTRY(main)
OUTPUT_ARCH(mips)
STARTUP(atj213x/crt0.o)

#define DRAM_END (DRAM_ORIG + DRAM_SIZE)
MEMORY
{
    DRAM : ORIGIN = DRAM_ORIG, LENGTH = DRAM_SIZE
    IRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
}

SECTIONS
{
  .init.text :
        {
            _irqbase = .;
            relocstart = .;
            oc_codestart = .;
            *(.init.text*)
        } > DRAM

  .exception.text (DRAM_ORIG + 0x200) :
        {
            *(.irq_vector*)
        } > DRAM

  .text :
	{
	    *(.text*)
            *(.icode*)
	} > DRAM

  .data :
	{
            *(.rodata*)
	    *(.data*)
            relocend = .;
	} > DRAM

  .stack (NOLOAD) :
        {
            . = ALIGN(4);
            stackbegin = .;
            oc_stackstart = .;
            . += 0x2000;
            stackend = .;
            irqstackbegin = .;
            . += 0x400;
            irqstackend = .;
            oc_stackend = .;
        } > DRAM

  .bss (NOLOAD) :
	{
            bssbegin = .;
            *(.sbss*)
	    *(.bss*)
	    *(COMMON)
            *(.scommon*)
            bssend = .;
            oc_codeend = .;
            oc_bufferstart = .;
	} > DRAM

  .end DRAM_ORIG+DRAM_SIZE (NOLOAD) :
  {
      oc_bufferend = .;
  } > DRAM
}
