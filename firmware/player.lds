ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)
SECTIONS
{
    .vectors 0x09000000 :
    {
        *(.resetvectors);
        *(.vectors);
        . = ALIGN(0x200);
        *(.init.text)
    }

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .rodata :
    {
        *(.rodata)
    }

    .bss :
    {
       _edata = .;
        *(.bss)
        *(COMMON)
       _end = .;
    }

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       . = 0x2000;
       _stackend = .;
    }

    .heap :
    {
        _poolstart = .;
        . = 0x20000;
        _poolend = .;
    }

    .mp3buf :
    {
        _mp3buf = .;
    }

    .mp3end 0x09200000 :
    {
        _mp3end = .;
    }

    .pad 0x0900C800 :
    {
	LONG(0);
    }
}
