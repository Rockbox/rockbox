ENTRY(start)
OUTPUT_FORMAT(elf32-sh)
INPUT(crt0.o)

#define PLUGINSIZE 0x8000

#ifdef DEBUG
#define DRAMSIZE 0x1f0000 - PLUGINSIZE
#define ORIGADDR 0x09010000

#else
#define DRAMSIZE (MEMORYSIZE * 0x100000) - PLUGINSIZE
#define ORIGADDR 0x09000000
#endif
#define ENDADDR  (ORIGADDR + DRAMSIZE)

MEMORY
{
   DRAM : ORIGIN = ORIGADDR, LENGTH = DRAMSIZE
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
        . = ALIGN(0x4);
    } > DRAM

    .data :
    {
        *(.data)
    } > DRAM

    .rodata :
    {
        *(.rodata)
        *(.rodata.str1.4)
        . = ALIGN(0x4);
    } > DRAM

    .stack :
    {
       *(.stack)
       _stackbegin = .;
       /* We put the copy of the .iram section here to save space */
       _iramcopy = .;
       . += 0x2000;
       _topramcopy = .;
       . += 0x300;
       _stackend = .;
    } > DRAM

    .bss :
    {
       _edata = .;
        *(.bss)
        *(COMMON)
       _end = .;
    } > DRAM

#ifdef DEBUG
    .heap :
    {
        _poolstart = .;
        . = 0x20000;
        _poolend = .;
    } > DRAM
#endif

    .mp3buf :
    {
        _mp3buffer = .;
    } > DRAM

    .mp3end ENDADDR - 0x300:
    {
        _mp3end = .;
    } > DRAM

    .topram : AT ( _topramcopy ) {
        _topramstart = .;
        *(.topcode)
        _topramend = .;
    } > DRAM

    .plugin ENDADDR:
    {
        _pluginbuf = .;
    }

    .iram 0xf000000 : AT ( _iramcopy )
    {
        _iramstart = .;
        *(.icode)
        *(.idata)
        _iramend = .;
    } > IRAM
}
