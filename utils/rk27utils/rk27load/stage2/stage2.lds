ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
/* STARTUP(crt0.o) */

MEMORY
{
    DRAM  : ORIGIN = 0x60000000, LENGTH = 0x01000000
    IRAM  : ORIGIN = 0x00000000, LENGTH = 0x00002000
}

SECTIONS
{
    .init.text : {
      *(.init.text)
    } > DRAM

    .text : {
      *(.text*)
      *(.glue_7*)
    } > IRAM AT > DRAM

    .data : {
      *(.data*)
    } > IRAM AT > DRAM

    _relocstart = LOADADDR(.text);
    _relocend = LOADADDR(.data) + SIZEOF(.data);

    .stack (NOLOAD) : {
      . = ALIGN(0x100);
      *(.stack)
      stackbegin = .;
      . += 0x200;
      stackend = .;
      irqstackbegin = .;
      . += 0x200;
      irqstackend = .;
    } > IRAM
}
