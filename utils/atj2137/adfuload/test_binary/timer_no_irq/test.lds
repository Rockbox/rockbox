OUTPUT_ARCH(mips)
ENTRY(main)

SECTIONS
{


.text 0xa0001000 :
	{
            relocstart = .;
            *(.init.text*)
	    *(.text*)
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
        }
}
