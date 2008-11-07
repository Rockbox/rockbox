OUTPUT_FORMAT(elf32-sh)

#define DRAMORIG 0x09000000
#define DRAMSIZE (MEMORYSIZE * 0x00100000)
#define IRAMORIG 0x0f000000
#define IRAMSIZE 0x00001000

MEMORY
{
    IRAM : ORIGIN = IRAMORIG, LENGTH = IRAMSIZE
    DRAM : ORIGIN = DRAMORIG, LENGTH = DRAMSIZE
}

SECTIONS
{
    .vectors :
    {
        _loadaddress = .;
        _dramend = . + DRAMSIZE;
        *(.vectors)
        . = ALIGN(0x200);
    } > DRAM

    .text :
    {
        *(.start)
        *(.text)
        . = ALIGN(0x4);
    } > DRAM

    .rodata :
    {
        *(.rodata*)
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > DRAM

    .data :
    {
        *(.data)
        . = ALIGN(0x4);
        _iramcopy = .;
    } > DRAM

    .iram IRAMORIG : AT ( _iramcopy )
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        . = ALIGN(0x4);
        _iramend = .;
    } > IRAM

    .stack :
    {
        _stackbegin = .;
        *(.stack)
        . += 0x0800;
        _stackend = .;
    } > IRAM

    .bss :
    {
        _edata = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(0x4);
        _end = .;
    } > DRAM
}
