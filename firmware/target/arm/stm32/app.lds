#include "cpu.h"

ENTRY(crt0_start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/stm32/crt0-stm32h7.o)

MEMORY
{
    DTCM     (rw)  : ORIGIN = STM32_DTCM_BASE,     LENGTH = STM32_DTCM_SIZE
    ITCM     (rx)  : ORIGIN = STM32_ITCM_BASE,     LENGTH = STM32_ITCM_SIZE
    SRAM_AXI (rw)  : ORIGIN = STM32_SRAM_AXI_BASE, LENGTH = STM32_SRAM_AXI_SIZE
    SDRAM    (rwx) : ORIGIN = STM32_SDRAM1_BASE,   LENGTH = MEMORYSIZE * 1024 * 1024
}

PHDRS
{
    itcm PT_LOAD;
    sdram PT_LOAD;
}

SECTIONS
{
    .itext :
    {
        KEEP(*(.vectors.arm));
        KEEP(*(.vectors.platform));
        *(.icode*);
    } > ITCM :itcm

    .stack (NOLOAD) : ALIGN(8)
    {
        irqstackbegin = .;
        . += 0x400;
        irqstackend = .;

        stackbegin = .;
        . += 0x2000;
        stackend = .;
    } > DTCM :NONE

    .framebuffer (NOLOAD) :
    {
        *(.framebuffer);
    } > SRAM_AXI :NONE

    .text :
    {
        loadaddress = .; /* only needed to keep ROLO happy */

        *(.init*);
        *(.text*);
    } > SDRAM :sdram

    .rodata :
    {
        *(.rodata*);
    } > SDRAM :sdram

    .data :
    {
        *(.data*);
    } > SDRAM :sdram

    .bss (NOLOAD) :
    {
        _bssbegin = .;
        *(.bss*);
        *(COMMON);
        _bssend = .;
    } > SDRAM :sdram

    audiobuffer = ALIGN(32);
    audiobufend = ORIGIN(SDRAM) + LENGTH(SDRAM) - CODEC_SIZE - PLUGIN_BUFFER_SIZE;
    codecbuf = audiobufend;
    pluginbuf = codecbuf + CODEC_SIZE;
}

EXTERN(__vectors_arm);
EXTERN(__vectors_platform);
ASSERT(DEFINED(__vectors_arm), "ARM exception vectors are not defined.");
ASSERT(DEFINED(__vectors_platform), "Platform exception vectors are not defined.");
