ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)

MEMORY
{
   DRAM : ORIGIN = 0x09000000, LENGTH = 0x200000
   IRAM : ORIGIN = 0x0f000000, LENGTH = 0x1000
}

SECTIONS
{
    .vectors :
    {
        *(.resetvectors);
        *(.vectors);
        . = ALIGN(0x200);
        *(.init.text)
    } > DRAM

    .text :
    {
        *(.text)
    } > DRAM

    .data :
    {
        *(.data)
    } > DRAM

    .rodata :
    {
        *(.rodata)
    } > DRAM

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       /* We put the copy of the .iram section here to save space */
       _iramcopy = .;
       . = 0x2000;
       _stackend = .;
    } > DRAM

    .bss :
    {
       _edata = .;
        *(.bss)
        *(COMMON)
       _end = .;
    } > DRAM

    .heap :
    {
        _poolstart = .;
        . = 0x20000;
        _poolend = .;
    } > DRAM

    .mp3buf :
    {
        _mp3buf = .;
    } > DRAM

    .mp3end 0x09200000 :
    {
        _mp3end = .;
    } > DRAM

    .pad 0x0900C800 :
    {
	   LONG(0);
    } > DRAM

    .iram 0xf000000 : AT ( _iramcopy )
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM
}
