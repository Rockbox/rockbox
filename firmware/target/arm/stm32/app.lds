#include "cpu.h"

/*
 * TODO: this is temporary and has not been tested
 */

ENTRY(main)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/stm32/crt0-stm32h7.o)

MEMORY
{
    SRAM_AXI (rwx) : ORIGIN = STM32_SRAM_AXI_BASE,    LENGTH = STM32_SRAM_AXI_SIZE
    DTCM     (rwx) : ORIGIN = STM32_DTCM_BASE,        LENGTH = STM32_DTCM_SIZE
    ITCM     (rwx) : ORIGIN = STM32_ITCM_BASE,        LENGTH = STM32_ITCM_SIZE
    SDRAM    (rwx) : ORIGIN = STM32_SDRAM1_BASE,      LENGTH = MEMORYSIZE * 1024 * 1024
}

/*
 * to control section alignment (only affects on-disk alignment):
 * -Wl,-z,max-page-size=0x1
 */

PHDRS
{
    sram_rx PT_LOAD ;
    sram_ro PT_LOAD ;
    sram_rw PT_LOAD ;
    itcm PT_LOAD ;
    dtcm PT_LOAD ;
    sdram_rx PT_LOAD ;
    sdram_rw PT_LOAD ;
}

SECTIONS
{
    .text :
    {
        loadaddress = .; /* only needed to keep ROLO happy */

        KEEP(*(.bootdata))
        *(.init.text*)
        *(.text*)
    } > SRAM_AXI :sram_rx

    .rodata :
    {
        *(.rodata*)
    } > SRAM_AXI :sram_ro

    .data :
    {
        _databegin = .;
        *(.data*)
        _dataend = .;
    } > SRAM_AXI :sram_rw
    _datacopy = LOADADDR(.data);

    .itext :
    {
        KEEP(*(.vectors.arm))
        KEEP(*(.vectors.platform))
        *(.icode*);
    } > ITCM :itcm

    .stack (NOLOAD) :
    {
        irqstackbegin = .;
        . += 0x400;
        irqstackend = .;

        stackbegin = .;
        . += 0x2000;
        stackend = .;

        *(.stack);
    } > DTCM :dtcm

    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.bss*);
        *(COMMON);
        _bssend = .;
    } > SDRAM :sdram_rw

    audiobuffer = ALIGN(32);
    audiobufend = ORIGIN(SDRAM) + LENGTH(SDRAM) - CODEC_SIZE - PLUGIN_BUFFER_SIZE;
    codecbuf = audiobufend;
    pluginbuf = codecbuf + CODEC_SIZE;
}

EXTERN(__vectors_arm);
EXTERN(__vectors_platform);
ASSERT(DEFINED(__vectors_arm), "ARM exception vectors are not defined.");
ASSERT(DEFINED(__vectors_platform), "Platform exception vectors are not defined.");
