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

    .magic 0x18200ff8 : {
      BYTE(0x51); /* R */
      BYTE(0x4B); /* K */
      BYTE(0x32); /* 2 */
      BYTE(0x37); /* 7 */
      BYTE(0x56); /* V */
      BYTE(0x31); /* 1 */
    }
}
