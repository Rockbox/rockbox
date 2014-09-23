#include "config.h"
ENTRY(main)
OUTPUT_ARCH(mips)
STARTUP(atj213x/crt0.o)

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
        } > IRAM

  .exception.text (IRAM_ORIG + 0x200) :
        {
            *(.irq_vector*)
        } > IRAM

  .text :
	{
	    *(.text*)
            *(.icode*)
	} > IRAM

  .data :
	{
            *(.rodata*)
	    *(.data*)
            *(.rel.dyn)
            relocend = .;
	} > IRAM

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
        } > IRAM

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
	} > IRAM

  .end IRAM_ORIG+IRAM_SIZE (NOLOAD) :
  {
      oc_bufferend = .;
  } > IRAM
}
