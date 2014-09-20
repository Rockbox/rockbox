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
            relocstart = .;
            oc_codestart = .;
            KEEP(*(.init.text*))
        } > IRAM

  .icode :
        {
            *(.icode*)
        } > IRAM

  .data :
	{
            *(.rodata*)
	    *(.data*)
            *(.rel.dyn)
	} > IRAM

  .bss (NOLOAD) :
	{
            bssbegin = .;
            *(.sbss*)
	    *(.bss*)
	    *(COMMON)
            *(.scommon*)
            . = ALIGN(4);
            bssend = .;
	} > IRAM

  .exception.tlb_refill (IRAM_ORIG + 0x1000) :
        {
            _irqbase = .;
            KEEP(*(.exception.tlb_refill))
        } > IRAM

  .exception.cache_error (IRAM_ORIG + 0x1100) :
        {
            KEEP(*(.exception.cache_error))
        } > IRAM

  .exception.general_exception (IRAM_ORIG + 0x1180) :
        {
            KEEP(*(.exception.general_exception))
        } > IRAM

  .exception.irq (IRAM_ORIG + 0x1200) :
        {
            KEEP(*(.exception.irq))
        } > IRAM

  .text :
	{
	    *(.text*)
            . = ALIGN(16);
            relocend = .;
	} > IRAM

  .stack (NOLOAD) :
        {
            . = ALIGN(4);
            stackbegin = .;
            oc_codeend = .;
            oc_stackstart = .;
            . += 0x2000;
            stackend = .;

            irqstackbegin = .;
            . += 0x400;
            irqstackend = .;
            oc_stackend = .;
            oc_bufferstart = .;
        } > IRAM

  .end IRAM_ORIG+IRAM_SIZE (NOLOAD) :
  {
      oc_bufferend = .;
  } > IRAM
}
