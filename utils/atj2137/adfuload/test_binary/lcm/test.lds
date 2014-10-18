OUTPUT_ARCH(mips)
ENTRY(main)

SECTIONS
{


.text 0xa0000000 :
	{
            relocstart = .;
            *(.init.text*)
	    *(.text*)
	}

        . = ALIGN(0x1000);
        _irqbase = .;

.exception_handlers :
        {
            . += 0x200;
            *(.irq_vector*)
        }

.data :
	{
            *(.rodata*)
	    *(.data*)
            relocend = .;
	}

.bss :
	{
            bssbegin = .;
            *(.sbss*)
	    *(.bss*)
	    *(COMMON)
            *(.scommon*)
            bssend = .;
	}

.stack (NOLOAD) :
        {
            stackbegin = .;
            . += 0x2000;
            stackend = .;
            irqstackbegin = .;
            . += 0x400;
            irqstackend = .;
        }

}
