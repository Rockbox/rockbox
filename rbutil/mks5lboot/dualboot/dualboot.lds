#define ASM
#include "config.h"
#include "cpu.h"
#include "mks5lboot.h"

ENTRY(_start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

#define BIN_ORIG    DFU_LOADADDR + BIN_OFFSET
#define BIN_SIZE    MAX_PAYLOAD

MEMORY
{
    IRAM : ORIGIN = BIN_ORIG, LENGTH = BIN_SIZE
}

SECTIONS
{
    .text : {
        *(.init.text*)
        *(.text*)
        *(.icode*)
        . = ALIGN(4);
    } > IRAM

    /* include initialized BSS (if any) into DFU image */
    .bss : {
        *(.bss*)
        *(.ibss*)
        *(COMMON)
        . = ALIGN(4);
    } > IRAM

#if 1
    /* reuse pwnage as stack, 0x30c bytes available */
    _exception_stack = BIN_ORIG;
    _supervisor_stack = _exception_stack;
#else
    /* include stack into DFU image */
    .stack : {
       . += 0x400;
       _supervisor_stack = .;
       . += 0x200;
       _exception_stack = .;
    } > IRAM
#endif

    .data : {
        *(.data*)
        *(.rodata*)
        *(.idata*)
        *(.irodata*)
        /* place bootloader IM3 header at the end, mkdfu
           will concatenate the bootloader binary here */
        . = ALIGN(16);
        *(.im3info.data*)
    } > IRAM
}
