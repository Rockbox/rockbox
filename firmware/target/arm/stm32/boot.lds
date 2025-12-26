#include "cpu.h"

ENTRY(reset_handler)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/stm32/crt0-stm32h7.o)

MEMORY
{
    SRAM_AXI (rwx) : ORIGIN = STM32_SRAM_AXI_BASE,    LENGTH = STM32_SRAM_AXI_SIZE
    DTCM     (rwx) : ORIGIN = STM32_DTCM_BASE,        LENGTH = STM32_DTCM_SIZE
    FLASH1   (rx)  : ORIGIN = STM32_FLASH_BANK1_BASE, LENGTH = STM32_FLASH_BANK1_SIZE
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors.arm))
        KEEP(*(.vectors.platform))
        *(.init.text*)
        *(.text*)
        *(.rodata*)
    } > FLASH1

    .data :
    {
        _databegin = .;
        *(.data*)
        _dataend = .;
    } > SRAM_AXI AT> FLASH1
    _datacopy = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.bss*);
        *(COMMON);
        _bssend = .;
    } > SRAM_AXI

    .stack (NOLOAD) : ALIGN(4)
    {
        irqstackbegin = .;
        . += 0x400;
        irqstackend = .;

        stackbegin = .;
        . += 0x2000;
        stackend = .;

        *(.stack);
    } > DTCM
}

EXTERN(__vectors_arm);
EXTERN(__vectors_platform);
ASSERT(DEFINED(__vectors_arm), "ARM exception vectors are not defined.");
ASSERT(DEFINED(__vectors_platform), "Platform exception vectors are not defined.");
