OUTPUT_ARCH(mips)
ENTRY(main)

SECTIONS
{


.text 0x80000000 :
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
            . = ALIGN(4);
            bssbegin = .;
            *(.sbss*)
	    *(.bss*)
	    *(COMMON)
            *(.scommon*)
            bssend = .;
	}

.stack (NOLOAD) :
        {
            . = ALIGN(4);
            stackbegin = .;
            . += 0x2000;
            stackend = .;
        }
}
