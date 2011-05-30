ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
/* STARTUP(crt0.o) */

/* this is where bootrom loads sdram init code */
MEMORY
{
    IRAM  : ORIGIN = 0x18200E00, LENGTH = 0x00000200
}

SECTIONS
{
    .text : {
      *(.text*)
      *(.glue_7*)
    } > IRAM

    .data : {
      *(.rodata*)
      *(.data*)
    } > IRAM
}
