/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Marcoen Hirschberg, Bart van Adrichem
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __S5L87XX_H__
#define __S5L87XX_H__

#ifndef ASM
#include <stdint.h>
#endif

#define REG16_PTR_T          volatile uint16_t *
#define REG32_PTR_T          volatile uint32_t *
#define VOID_PTR_PTR_T       void* volatile*
#define CONST_VOID_PTR_PTR_T const void* volatile*

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define CACHEALIGN_BITS (4) /* 2^4 = 16 bytes */
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define CACHEALIGN_BITS (5) /* 2^5 = 32 bytes */
#endif

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define DRAM_ORIG 0x08000000
#define IRAM_ORIG 0x22000000

#define IRAM0_ORIG  IRAM_ORIG
#define IRAM0_SIZE  0x20000
#define IRAM1_ORIG  (IRAM0_ORIG + IRAM0_SIZE)

#if CONFIG_CPU==S5L8702
#define IRAM1_SIZE  0x20000
#elif CONFIG_CPU==S5L8720
#define IRAM1_SIZE  0x10000
#endif

#define DRAM_SIZE   (MEMORYSIZE * 0x100000)
#define IRAM_SIZE   (IRAM0_SIZE + IRAM1_SIZE)

#define TTB_SIZE        0x4000
#define TTB_BASE_ADDR   (DRAM_ORIG + DRAM_SIZE - TTB_SIZE)
#endif

/* Base address of the memory-mapped I/O */
#define IO_BASE 0x38000000

/* 04. CALMADM2E */

/* Following registers are mapped on IO Area in data memory area of Calm. */
#define ADM_BASE 0x3F000000

#define CONFIG0                 (*(REG16_PTR_T)(ADM_BASE))            /* configuration/control register 0 */
#define CONFIG1                 (*(REG16_PTR_T)(ADM_BASE + 0x02))     /* configuration/control register 1*/
#define COMMUN                  (*(REG16_PTR_T)(ADM_BASE + 0x04))     /* Communication Control Register */
#define DDATA0                  (*(REG16_PTR_T)(ADM_BASE + 0x06))     /* Communication data from host to ADM */
#define DDATA1                  (*(REG16_PTR_T)(ADM_BASE + 0x08))     /* Communication data from host to ADM */
#define DDATA2                  (*(REG16_PTR_T)(ADM_BASE + 0x0A))     /* Communication data from host to ADM */
#define DDATA3                  (*(REG16_PTR_T)(ADM_BASE + 0x0C))     /* Communication data from host to ADM */
#define DDATA4                  (*(REG16_PTR_T)(ADM_BASE + 0x0E))     /* Communication data from host to ADM */
#define DDATA5                  (*(REG16_PTR_T)(ADM_BASE + 0x10))     /* Communication data from host to ADM */
#define DDATA6                  (*(REG16_PTR_T)(ADM_BASE + 0x12))     /* Communication data from host to ADM */
#define DDATA7                  (*(REG16_PTR_T)(ADM_BASE + 0x14))     /* Communication data from host to ADM */
#define UDATA0                  (*(REG16_PTR_T)(ADM_BASE + 0x16))     /* Communication data from ADM to host */
#define UDATA1                  (*(REG16_PTR_T)(ADM_BASE + 0x18))     /* Communication data from ADM to host */
#define UDATA2                  (*(REG16_PTR_T)(ADM_BASE + 0x1A))     /* Communication data from ADM to host */
#define UDATA3                  (*(REG16_PTR_T)(ADM_BASE + 0x1C))     /* Communication data from ADM to host */
#define UDATA4                  (*(REG16_PTR_T)(ADM_BASE + 0x1E))     /* Communication data from ADM to host */
#define UDATA5                  (*(REG16_PTR_T)(ADM_BASE + 0x20))     /* Communication data from ADM to host */
#define UDATA6                  (*(REG16_PTR_T)(ADM_BASE + 0x22))     /* Communication data from ADM to host */
#define UDATA7                  (*(REG16_PTR_T)(ADM_BASE + 0x24))     /* Communication data from ADM to host */
#define IBASE_H                 (*(REG16_PTR_T)(ADM_BASE + 0x26))     /* Higher half of start address for ADM instruction area */
#define IBASE_L                 (*(REG16_PTR_T)(ADM_BASE + 0x28))     /* Lower half of start address for ADM instruction area */
#define DBASE_H                 (*(REG16_PTR_T)(ADM_BASE + 0x2A))     /* Higher half of start address for CalmRISC data area */
#define DBASE_L                 (*(REG16_PTR_T)(ADM_BASE + 0x2C))     /* Lower half of start address for CalmRISC data area */
#define XBASE_H                 (*(REG16_PTR_T)(ADM_BASE + 0x2E))     /* Higher half of start address for Mac X area */
#define XBASE_L                 (*(REG16_PTR_T)(ADM_BASE + 0x30))     /* Lower half of start address for Mac X area */
#define YBASE_H                 (*(REG16_PTR_T)(ADM_BASE + 0x32))     /* Higher half of start address for Mac Y area */
#define YBASE_L                 (*(REG16_PTR_T)(ADM_BASE + 0x34))     /* Lower half of start address for Mac Y area */
#define S0BASE_H                (*(REG16_PTR_T)(ADM_BASE + 0x36))     /* Higher half of start address for sequential buffer 0 area */
#define S0BASE_L                (*(REG16_PTR_T)(ADM_BASE + 0x38))     /* Lower half of start address for sequential buffer 0 area */
#define S1BASE_H                (*(REG16_PTR_T)(ADM_BASE + 0x3A))     /* Higher half of start address for sequential buffer 1 area */
#define S1BASE_L                (*(REG16_PTR_T)(ADM_BASE + 0x3C))     /* Lower half of start address for sequential buffer 1 area */
#define CACHECON                (*(REG16_PTR_T)(ADM_BASE + 0x3E))     /* Cache Control Register */
#define CACHESTAT               (*(REG16_PTR_T)(ADM_BASE + 0x40))     /* Cache status register */
#define SBFCON                  (*(REG16_PTR_T)(ADM_BASE + 0x42))     /* Sequential Buffer Control Register */
#define SBFSTAT                 (*(REG16_PTR_T)(ADM_BASE + 0x44))     /* Sequential Buffer Status Register */
#define SBL0OFF_H               (*(REG16_PTR_T)(ADM_BASE + 0x46))     /* Higher bits of Offset register of sequential block 0 area */
#define SBL0OFF_L               (*(REG16_PTR_T)(ADM_BASE + 0x48))     /* Lower bits of Offset register of sequential block 0 area */
#define SBL1OFF_H               (*(REG16_PTR_T)(ADM_BASE + 0x4A))     /* Higher bits of Offset register of sequential block 1 area */
#define SBL1OFF_L               (*(REG16_PTR_T)(ADM_BASE + 0x4C))     /* Lower bits of Offset register of sequential block 1 area */
#define SBL0BEGIN_H             (*(REG16_PTR_T)(ADM_BASE + 0x4E))     /* Higher bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL0BEGIN_L             (*(REG16_PTR_T)(ADM_BASE + 0x50))     /* Lower bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL1BEGIN_H             (*(REG16_PTR_T)(ADM_BASE + 0x52))     /* Higher bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL1BEGIN_L             (*(REG16_PTR_T)(ADM_BASE + 0x54))     /* Lower bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL0END_H               (*(REG16_PTR_T)(ADM_BASE + 0x56))     /* Lower bits of End Offset of sequential block 0 area in ring mode */
#define SBL0END_L               (*(REG16_PTR_T)(ADM_BASE + 0x58))     /* Higher bits of End Offset of sequential block 0 area in ring mode */
#define SBL1END_H               (*(REG16_PTR_T)(ADM_BASE + 0x5A))     /* Lower bits of End Offset of sequential block 1 area in ring mode */
#define SBL1END_L               (*(REG16_PTR_T)(ADM_BASE + 0x5C))     /* Higher bits of End Offset of sequential block 1 area in ring mode */

/* Following registers are components of SFRS of the target system */
#define ADM_SFR_BASE 0x39000000

#define ADM_CONFIG              (*(REG32_PTR_T)(ADM_SFR_BASE))            /* Configuration/Control Register */
#define ADM_COMMUN              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x04))     /* Communication Control Register */
#define ADM_DDATA0              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x10))     /* Communication data from host to ADM */
#define ADM_DDATA1              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x14))     /* Communication data from host to ADM */
#define ADM_DDATA2              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x18))     /* Communication data from host to ADM */
#define ADM_DDATA3              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x1C))     /* Communication data from host to ADM */
#define ADM_DDATA4              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x20))     /* Communication data from host to ADM */
#define ADM_DDATA5              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x24))     /* Communication data from host to ADM */
#define ADM_DDATA6              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x28))     /* Communication data from host to ADM */
#define ADM_DDATA7              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x2C))     /* Communication data from host to ADM */
#define ADM_UDATA0              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x30))     /* Communication data from ADM to host */
#define ADM_UDATA1              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x34))     /* Communication data from ADM to host */
#define ADM_UDATA2              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x38))     /* Communication data from ADM to host */
#define ADM_UDATA3              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x3C))     /* Communication data from ADM to host */
#define ADM_UDATA4              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x40))     /* Communication data from ADM to host */
#define ADM_UDATA5              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x44))     /* Communication data from ADM to host */
#define ADM_UDATA6              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x48))     /* Communication data from ADM to host */
#define ADM_UDATA7              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x4C))     /* Communication data from ADM to host */
#define ADM_IBASE               (*(REG32_PTR_T)(ADM_SFR_BASE + 0x50))     /* Start Address for ADM Instruction Area */
#define ADM_DBASE               (*(REG32_PTR_T)(ADM_SFR_BASE + 0x54))     /* Start Address for CalmRISC Data Area */
#define ADM_XBASE               (*(REG32_PTR_T)(ADM_SFR_BASE + 0x58))     /* Start Address for Mac X Area */
#define ADM_YBASE               (*(REG32_PTR_T)(ADM_SFR_BASE + 0x5C))     /* Start Address for Mac Y Area */
#define ADM_S0BASE              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x60))     /* Start Address for Sequential Block 0 Area */
#define ADM_S1BASE              (*(REG32_PTR_T)(ADM_SFR_BASE + 0x64))     /* Start Address for Sequential Block 1 Area */

/* 05. CLOCK & POWER MANAGEMENT */
#define CLK_BASE 0x3C500000

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define CLKCON                  (*(REG32_PTR_T)(CLK_BASE))            /* Clock control register */
#define PLL0PMS                 (*(REG32_PTR_T)(CLK_BASE + 0x04))     /* PLL PMS value register */
#define PLL1PMS                 (*(REG32_PTR_T)(CLK_BASE + 0x08))     /* PLL PMS value register */
#define PLL2PMS                 (*(REG32_PTR_T)(CLK_BASE + 0x0C))     /* PLL PMS value register  - S5L8701 only? */
#define CLKCON3                 (*(REG32_PTR_T)(CLK_BASE + 0x10))     /* Clock control register 3 */
#define PLL0LCNT                (*(REG32_PTR_T)(CLK_BASE + 0x14))     /* PLL0 lock count register */
#define PLL1LCNT                (*(REG32_PTR_T)(CLK_BASE + 0x18))     /* PLL1 lock count register */
#define PLL2LCNT                (*(REG32_PTR_T)(CLK_BASE + 0x1C))     /* PLL2 lock count register - S5L8701 only? */
#define PLLLOCK                 (*(REG32_PTR_T)(CLK_BASE + 0x20))     /* PLL lock status register */
#define PLLCON                  (*(REG32_PTR_T)(CLK_BASE + 0x24))     /* PLL control register */
#define PWRCON                  (*(REG32_PTR_T)(CLK_BASE + 0x28))     /* Clock power control register */
#define PWRMODE                 (*(REG32_PTR_T)(CLK_BASE + 0x2C))     /* Power mode control register */
#define SWRCON                  (*(REG32_PTR_T)(CLK_BASE + 0x30))     /* Software reset control register */
#define RSTSR                   (*(REG32_PTR_T)(CLK_BASE + 0x34))     /* Reset status register */
#define DSPCLKMD                (*(REG32_PTR_T)(CLK_BASE + 0x38))     /* DSP clock mode register */
#define CLKCON2                 (*(REG32_PTR_T)(CLK_BASE + 0x3C))     /* Clock control register 2 */
#define PWRCONEXT               (*(REG32_PTR_T)(CLK_BASE + 0x40))     /* Clock power control register 2 */
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define CLKCON0      (*((REG32_PTR_T)(CLK_BASE)))
#define CLKCON1      (*((REG32_PTR_T)(CLK_BASE + 0x04)))
#define CLKCON2      (*((REG32_PTR_T)(CLK_BASE + 0x08)))
#define CLKCON3      (*((REG32_PTR_T)(CLK_BASE + 0x0C)))
#define CLKCON4      (*((REG32_PTR_T)(CLK_BASE + 0x10)))
#define CLKCON5      (*((REG32_PTR_T)(CLK_BASE + 0x14)))

#define CLKCON0_SDR_DISABLE_BIT (1 << 31)
#if CONFIG_CPU == S5L8720
#define CLKCON0_UNK30_BIT       (1 << 30)
#endif

/* CPU/AHB/APB real_divisor =
   xDIV_EN_BIT ? 2*(reg_value+1) : 1 */
#define CLKCON1_CDIV_POS        24
#define CLKCON1_CDIV_MSK        0x1f
#define CLKCON1_CDIV_EN_BIT     (1 << 30)

#define CLKCON1_HDIV_POS        16
#define CLKCON1_HDIV_MSK        0x1f
#define CLKCON1_HDIV_EN_BIT     (1 << 22)

#define CLKCON1_PDIV_POS        8
#define CLKCON1_PDIV_MSK        0x1f
#define CLKCON1_PDIV_EN_BIT     (1 << 14)

/* AHB/APB ratio: must be written when HDIV and/or PDIV
   are modified, real_ratio = reg_value + 1 */
#define CLKCON1_HPRAT_POS       0
#define CLKCON1_HPRAT_MSK       0x3f

/* TBC: this bit selects a clock routed (at least) to all I2S modules
 * (AUDAUX_Clk, see i2s-s5l8702.h), it can be selected as a source
 * for CODEC_CLK (MCLK), on iPod Classic AUDAUX_Clk is:
 *  0 -> 12 MHz (TBC: OSC0 ???)
 *  1 -> 24 MHz (TBC: 2*OSC0 ???)
 */
#define CLKCON5_AUDAUXCLK_BIT   (1 << 31)

#define PLLPMS(i)    (*((REG32_PTR_T)(CLK_BASE + 0x20 + ((i) * 4))))
#define PLL0PMS      PLLPMS(0)
#define PLL1PMS      PLLPMS(1)
#define PLL2PMS      PLLPMS(2)

/*
 *  PLLnPMS
 */
#define PLLPMS_PDIV_POS         24      /* pre-divider */
#define PLLPMS_PDIV_MSK         0x3f
#define PLLPMS_MDIV_POS         8       /* main divider */
#define PLLPMS_MDIV_MSK         0xff
#define PLLPMS_SDIV_POS         0       /* post-divider (2^S) */
#define PLLPMS_SDIV_MSK         0x7

#define PLLCNT(i)    (*((REG32_PTR_T)(CLK_BASE + 0x30 + ((i) * 4))))
#define PLLCNT_MSK   0x3fffff
#define PLL0LCNT     PLLCNT(0)
#define PLL1LCNT     PLLCNT(1)
#define PLL2LCNT     PLLCNT(2)
#if CONFIG_CPU == S5L8720
#define PLLUNK3C     (*((REG32_PTR_T)(CLK_BASE + 0x3C)))
#endif
#define PLLLOCK      (*((REG32_PTR_T)(CLK_BASE + 0x40)))

/* Start status:
   0 -> in progress, 1 -> locked */
#define PLLLOCK_LCK_BIT(n)      (1 << (n))

/* Lock status for Divisor Mode (DM):
   0 -> DM unlocked, 1 -> DM locked */
#define PLLLOCK_DMLCK_BIT(n)    (1 << (4 + (n)))

#define PLLMODE      (*((REG32_PTR_T)(CLK_BASE + 0x44)))

/* Enable PLL0,1,2:
   0 -> turned off, 1 -> turned on */
#define PLLMODE_EN_BIT(n)       (1 << (n))

/* Select PMS mode for PLL0,1:
   0 -> mutiply mode (MM), 1 -> divide mode (DM) */
#define PLLMODE_PMSMOD_BIT(n)   (1 << (4 + (n)))

/* Select DMOSC for PLL2:
   0 -> DMOSC_STD, 1 -> DMOSC_ALT */
#define PLLMODE_PLL2DMOSC_BIT   (1 << 6)

/* Select oscilator for CG16_SEL_OSC source:
   0 -> S5L8702_OSC0, 1 -> S5L8702_OSC1 */
#define PLLMODE_OSCSEL_BIT      (1 << 8)

/* Select PLLxClk (a.k.a. "slow mode", see s3c2440-DS) for PLL0,1,2:
   O -> S5L8702_OSC1, 1 -> PLLxFreq */
#define PLLMODE_PLLOUT_BIT(n)   (1 << (16 + (n)))

/* s5l8702 only uses PWRCON0 and PWRCON1 */
#define PWRCON(i)    (*((REG32_PTR_T)(CLK_BASE \
                                           + ((i) == 4 ? 0x6C : \
                                             ((i) == 3 ? 0x68 : \
                                             ((i) == 2 ? 0x58 : \
                                             ((i) == 1 ? 0x4C : \
                                                         0x48)))))))

/* TBC: ATM i am assuming that PWRCON_AHB/APB registers are clockgates
 * for SoC internal controllers sitting on AHB/APB buses, this is based
 * on other similar SoC documentation and experimental results for many
 * (not all) s5l8702 controllers.
 */
#define PWRCON_AHB  PWRCON(0)
#define PWRCON_APB  PWRCON(1)

/* SW Reset Control Register */
#define SWRCON      (*((REG32_PTR_T)(CLK_BASE + 0x50)))
/* Reset Status Register */
#define RSTSR       (*((REG32_PTR_T)(CLK_BASE + 0x54)))
#define RSTSR_WDR_BIT   (1 << 2)
#define RSTSR_SWR_BIT   (1 << 1)
#define RSTSR_HWR_BIT   (1 << 0)

#if CONFIG_CPU == S5L8702
#define PLLMOD2     (*((REG32_PTR_T)(CLK_BASE + 0x60)))

/* Selects ALTOSCx for PLL0,1,2 when DMOSC == DMOSC_ALT:
   0 -> S5L8702_ALTOSC0, 1 -> S5L8702_ALTOSC1 */
#define PLLMOD2_ALTOSC_BIT(n)   (1 << (n))

/* Selects DMOSC for PLL0,1:
   0 -> DMOSC_STD, 1 -> DMOSC_ALT */
#define PLLMOD2_DMOSC_BIT(n)    (1 << (4 + (n)))

#elif CONFIG_CPU==S5L8720
#define PLLUNK64    (*((REG32_PTR_T)(CLK_BASE + 0x64))) // used by efi_ClockAndReset
#define CLKCON6     (*((REG32_PTR_T)(CLK_BASE + 0x70)))
#endif /* CONFIG_CPU==S5L8720 */
#endif /* CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720 */



#if CONFIG_CPU==S5L8700
#define    CLOCKGATE_UARTC      8
#elif CONFIG_CPU==S5L8701
#define    CLOCKGATE_UARTC0     8
#define    CLOCKGATE_UARTC1     9
#define    CLOCKGATE_UARTC2     13
#elif CONFIG_CPU==S5L8702
#define CLOCKGATE_SHA       0
#define CLOCKGATE_LCD       1
#define CLOCKGATE_USBOTG    2
#define CLOCKGATE_SMx       3
#define CLOCKGATE_SM1       4
#define CLOCKGATE_ATA       5
#define CLOCKGATE_NAND      8
#define CLOCKGATE_SDCI      9
#define CLOCKGATE_AES       10
#define CLOCKGATE_NANDECC   12
#define CLOCKGATE_DMAC0     25
#define CLOCKGATE_DMAC1     26
#define CLOCKGATE_ROM       30

#define CLOCKGATE_RTC       32
#define CLOCKGATE_CWHEEL    33
#define CLOCKGATE_SPI0      34
#define CLOCKGATE_USBPHY    35
#define CLOCKGATE_I2C0      36
#define CLOCKGATE_TIMER     37
#define CLOCKGATE_I2C1      38
#define CLOCKGATE_I2S0      39
#define CLOCKGATE_UARTC     41
#define CLOCKGATE_I2S1      42
#define CLOCKGATE_SPI1      43
#define CLOCKGATE_GPIO      44
#define CLOCKGATE_CHIPID    46
#define CLOCKGATE_I2S2      47
#define CLOCKGATE_SPI2      48
#elif CONFIG_CPU==S5L8720
/* PWRCON0 - 18 gates (0..17) */
#define CLOCKGATE_SHA       0
#define CLOCKGATE_LCD       1       // TBC
#define CLOCKGATE_USBOTG    2
#define CLOCKGATE_SMx       3       // TBC
#define CLOCKGATE_SM1       4       // TBC
#define CLOCKGATE_NAND      5
#define CLOCKGATE_AES       7
#define CLOCKGATE_NANDECC   9
#define CLOCKGATE_DMAC0     11      // TBC
#define CLOCKGATE_DMAC1     12      // TBC
#define CLOCKGATE_ROM       13

#define CLOCKGATE_UNK14     14      // this could also be DMAC0
// #define CLOCKGATE_UNK15     15
// #define CLOCKGATE_UNK16     16
// #define CLOCKGATE_UNK17     17

/* PWRCON1 - 32 gates (32..63)*/
// #define CLOCKGATE_RTC       32
#define CLOCKGATE_CWHEEL    33      // TBC
#define CLOCKGATE_SPI0      34
#define CLOCKGATE_USBPHY    35
#define CLOCKGATE_I2C0      36
#define CLOCKGATE_TIMERA    37

#define CLOCKGATE_I2C1      38      // TBC
// #define CLOCKGATE_I2S0      39
#define CLOCKGATE_UARTC     41      // TBC
// #define CLOCKGATE_I2S1      42
#define CLOCKGATE_SPI1      43
#define CLOCKGATE_GPIO      44      // TBC
#define CLOCKGATE_UNK45     45      // it is used in peicore, also in s5l8702, it is related to 0x3d00_0000 (secure boot ???)
#define CLOCKGATE_CHIPID    46

#define CLOCKGATE_SPI2      47
// // #define CLOCKGATE_I2S2      47
// // #define CLOCKGATE_SPI2      48

// #define CLOCKGATE_UNK50     50
// #define CLOCKGATE_UNK52     52
// #define CLOCKGATE_UNK54     54

#define CLOCKGATE_TIMERB    55
#define CLOCKGATE_TIMERE    56
#define CLOCKGATE_TIMERF    57
#define CLOCKGATE_TIMERG    58
#define CLOCKGATE_TIMERH    59
#define CLOCKGATE_TIMERI    60

#define CLOCKGATE_UNK61     61
#define CLOCKGATE_UNK62     62
#define CLOCKGATE_UNK63     63


/* PWRCON2 - 7 gates (64..70) */
#define CLOCKGATE_SPI3      65      // TBC
#define CLOCKGATE_UNK66     66
// #define CLOCKGATE_UNK67     67
#define CLOCKGATE_SPI4      68      // TBC
#define CLOCKGATE_TIMERC    69
#define CLOCKGATE_TIMERD    70


/* PWRCON3 - 15 gates (96..110) */
#define CLOCKGATE_UNK104    104     // it is used by peicore, related to 0x3880_0000
#define CLOCKGATE_UNK14_2   107

// #define CLOCKGATE_UNK109    109      // it hangs if we disable it


/* PWRCON4 - 24 gates (128..151) */
#define CLOCKGATE_TIMERA_2  128
#define CLOCKGATE_TIMERB_2  129
#define CLOCKGATE_TIMERE_2  130
#define CLOCKGATE_TIMERF_2  131
#define CLOCKGATE_TIMERG_2  132
#define CLOCKGATE_TIMERH_2  133
#define CLOCKGATE_TIMERI_2  134

#define CLOCKGATE_UARTC_2   135     // TBC

#define CLOCKGATE_UNK61_2   136
#define CLOCKGATE_UNK62_2   137
#define CLOCKGATE_UNK63_2   138

#define CLOCKGATE_I2C0_2    139
#define CLOCKGATE_I2C1_2    140

#define CLOCKGATE_SPI0_2    141
#define CLOCKGATE_SPI1_2    142
#define CLOCKGATE_SPI2_2    143

#define CLOCKGATE_LCD_2     144     // TBC

// #define CLOCKGATE_UNK145    145

#define CLOCKGATE_SPI3_2    147     // TBC
#define CLOCKGATE_SPI4_2    148     // TBC

#define CLOCKGATE_UNK66_2   149

#define CLOCKGATE_TIMERC_2  150
#define CLOCKGATE_TIMERD_2  151
#endif

#if CONFIG_CPU == S5L8702 || CONFIG_CPU == S5L8720
/* CG16_x: for readability and debug, these gates are defined as
 * 16-bit registers, on HW they are really halves of 32-bit registers.
 * Some functionallity is not available on all CG16 gates (when so,
 * related bits are read-only and fixed to 0).
 *
 *                CLKCONx   DIV1    DIV2    UNKOSC   UNK14
 *  CG16_SYS      0L        +
 *  CG16_2L       2L        +               +(TBC)   +(TBC)
 *  CG16_SVID     2H        +               +(TBC)
 *  CG16_AUD0     3L        +       +
 *  CG16_AUD1     3H        +       +
 *  CG16_AUD2     4L        +       +
 *  CG16_RTIME    4H        +       +       +
 *  CG16_5L       5L        +
 *
 * Not all gates are fully tested, this information is mainly based
 * on experimental test using emCORE:
 *  - CG16_SYS and CG16_RTIME were tested mainly using time benchs.
 *  - EClk is used as a fixed clock (not depending on CPU/AHB/APB
 *    settings) for the timer controller. MIU_Clk is used by the MIU
 *    controller to generate the DRAM refresh signals.
 *  - AUDxClk are a source selection for I2Sx modules, so they can
 *    can be scaled and routed to the I2S GPIO ports, where they
 *    were sampled (using emCORE) to inspect how they behave.
 *  - CG16_SVID seem to be used for external video, this info is
 *    based on OF diagnostics reverse engineering.
 *  - CG16_2L and CG16_5L usage is unknown.
 */
#define CG16_SYS        (*((REG16_PTR_T)(CLK_BASE)))
#if CONFIG_CPU == S5L8702
#define CG16_2L         (*((REG16_PTR_T)(CLK_BASE + 0x08)))
#elif CONFIG_CPU == S5L8720
#define CG16_LCD        (*((REG16_PTR_T)(CLK_BASE + 0x08)))
#endif
#define CG16_SVID       (*((REG16_PTR_T)(CLK_BASE + 0x0A)))
#define CG16_AUD0       (*((REG16_PTR_T)(CLK_BASE + 0x0C)))
#define CG16_AUD1       (*((REG16_PTR_T)(CLK_BASE + 0x0E)))
#define CG16_AUD2       (*((REG16_PTR_T)(CLK_BASE + 0x10)))
#define CG16_RTIME      (*((REG16_PTR_T)(CLK_BASE + 0x12)))
#define CG16_5L         (*((REG16_PTR_T)(CLK_BASE + 0x14)))
#if CONFIG_CPU == S5L8720
#define CG16_6L         (*((REG16_PTR_T)(CLK_BASE + 0x70)))
#endif

/* CG16 output frequency =
   !DISABLE_BIT * SEL_x frequency / DIV1+1 / DIV2+1 */
#define CG16_DISABLE_BIT    (1 << 15)   /* mask clock output */
#define CG16_UNK14_BIT      (1 << 14)   /* writable on CG16_2L */

#define CG16_SEL_POS        12          /* source clock selection */
#define CG16_SEL_MSK        0x3
#define CG16_SEL_OSC        0
#define CG16_SEL_PLL0       1
#define CG16_SEL_PLL1       2
#define CG16_SEL_PLL2       3

#define CG16_UNKOSC_BIT     (1 << 11)

#define CG16_DIV2_POS       4           /* 2nd divisor */
#define CG16_DIV2_MSK       0xf

#define CG16_DIV1_POS       0           /* 1st divisor */
#define CG16_DIV1_MSK       0xf

/* SM1 */
#define SM1_BASE 0x38500000

/* TBC: Clk_SM1 = HClk / (SM1_DIV[3:0] + 1) */
#define SM1_DIV     (*((REG32_PTR_T)(SM1_BASE + 0x1000)))
#endif

/* 06. INTERRUPT CONTROLLER UNIT */
#define INT_BASE 0x39C00000

#define SRCPND                  (*(REG32_PTR_T)(INT_BASE))            /* Indicates the interrupt request status. */
#define INTMOD                  (*(REG32_PTR_T)(INT_BASE + 0x04))     /* Interrupt mode register. */
#define INTMSK                  (*(REG32_PTR_T)(INT_BASE + 0x08))     /* Determines which interrupt source is masked.
                                                                The masked interrupt source will not be serviced. */

#if CONFIG_CPU==S5L8700
#define    INTMSK_TIMERA        (1<<5)
#define    INTMSK_TIMERB        (1<<7)
#define    INTMSK_TIMERC        (1<<8)
#define    INTMSK_TIMERD        (1<<9)
#define    INTMSK_UART0         (1<<22)
#define    INTMSK_UART1         (1<<14)
#elif CONFIG_CPU==S5L8701
#define    INTMSK_EINTG0        (1<<1)
#define    INTMSK_EINTG1        (1<<2)
#define    INTMSK_EINTG2        (1<<3)
#define    INTMSK_EINTG3        (1<<4)
#define    INTMSK_TIMERA        (1<<5)
#define    INTMSK_TIMERB        (1<<5)
#define    INTMSK_TIMERC        (1<<5)
#define    INTMSK_TIMERD        (1<<5)
#define    INTMSK_ECC           (1<<19)
#define    INTMSK_USB_OTG       (1<<16)
#define    INTMSK_UART0         (0)     /* (AFAIK) no IRQ to ICU, uses EINTG0 */
#define    INTMSK_UART1         (1<<12)
#define    INTMSK_UART2         (1<<7)
#endif

#define PRIORITY                (*(REG32_PTR_T)(INT_BASE + 0x0C))     /* IRQ priority control register */
#define INTPND                  (*(REG32_PTR_T)(INT_BASE + 0x10))     /* Indicates the interrupt request status. */
#define INTOFFSET               (*(REG32_PTR_T)(INT_BASE + 0x14))     /* Indicates the IRQ interrupt request source */

#if CONFIG_CPU==S5L8700
#define EINTPOL                 (*(REG32_PTR_T)(INT_BASE + 0x18))     /* Indicates external interrupt polarity */
#define EINTPEND                (*(REG32_PTR_T)(INT_BASE + 0x1C))     /* Indicates whether external interrupts are pending. */
#define EINTMSK                 (*(REG32_PTR_T)(INT_BASE + 0x20))     /* Indicates whether external interrupts are masked */
#elif CONFIG_CPU==S5L8701
/*
 * s5l8701 GPIO (External) Interrupt Controller.
 *
 * At first glance it looks very similar to gpio-s5l8702, but
 * not fully tested, so this information could be wrong.
 *
 *  Group0[31:10] Not used
 *        [9]     UART0 IRQ
 *        [8]     VBUS
 *        [7:0]   PDAT1
 *  Group1[31:0]  PDAT5:PDAT4:PDAT3:PDAT2
 *  Group2[31:0]  PDAT11:PDAT10:PDAT7:PDAT6
 *  Group3[31:0]  PDAT15:PDAT14:PDAT13:PDAT12
 */
#define GPIOIC_INTLEVEL(g)      (*(REG32_PTR_T)(INT_BASE + 0x18 + 4*(g)))
#define GPIOIC_INTSTAT(g)       (*(REG32_PTR_T)(INT_BASE + 0x28 + 4*(g)))
#define GPIOIC_INTEN(g)         (*(REG32_PTR_T)(INT_BASE + 0x38 + 4*(g)))
#define GPIOIC_INTTYPE(g)       (*(REG32_PTR_T)(INT_BASE + 0x48 + 4*(g)))
#endif

/* 07. MEMORY INTERFACE UNIT (MIU) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define MIU_BASE 0x38200000
#elif CONFIG_CPU==S5L8702
#define MIU_BASE 0x38100000
#elif CONFIG_CPU==S5L8720
#define MIU_BASE 0x3d700000
#endif

#define MIU_REG(off)    (*((REG32_PTR_T)(MIU_BASE + (off))))

/* SDRAM */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
#define MIUCON                  (*(REG32_PTR_T)(MIU_BASE))            /* External Memory configuration register */
#define MIUCOM                  (*(REG32_PTR_T)(MIU_BASE + 0x04))     /* Command and status register */
#define MIUAREF                 (*(REG32_PTR_T)(MIU_BASE + 0x08))     /* Auto-refresh control register */
#define MIUMRS                  (*(REG32_PTR_T)(MIU_BASE + 0x0C))     /* SDRAM Mode Register Set Value Register */
#define MIUSDPARA               (*(REG32_PTR_T)(MIU_BASE + 0x10))     /* SDRAM parameter register */
#elif CONFIG_CPU==S5L8720
#define MIUCOM                  (*(REG32_PTR_T)(MIU_BASE + 0x104))    /* Command and status register */
#define MIUMRS                  (*(REG32_PTR_T)(MIU_BASE + 0x110))    /* SDRAM Mode Register Set Value Register */

#define UNK3E000000_BASE        0x3e000000
#define UNK3E000008             (*(REG32_PTR_T)(UNK3E000000_BASE + 0x08))
#endif

/* DDR */
#define MEMCONF                 (*(REG32_PTR_T)(MIU_BASE + 0x20))     /* External Memory configuration register */
#define USRCMD                  (*(REG32_PTR_T)(MIU_BASE + 0x24))     /* Command and Status register */
#define AREF                    (*(REG32_PTR_T)(MIU_BASE + 0x28))     /* Auto-refresh control register */
#define MRS                     (*(REG32_PTR_T)(MIU_BASE + 0x2C))     /* DRAM mode register set value register */
#define DPARAM                  (*(REG32_PTR_T)(MIU_BASE + 0x30))     /* DRAM parameter register (Unit of ‘tXXX’ : tCK */
#define SMEMCONF                (*(REG32_PTR_T)(MIU_BASE + 0x34))     /* Static memory mode register set value register */
#define MIUS01PARA              (*(REG32_PTR_T)(MIU_BASE + 0x38))     /* SRAM0, SRAM1 static memory parameter register (In S5L8700, SRAM0 is Nor Flash) */
#define MIUS23PARA              (*(REG32_PTR_T)(MIU_BASE + 0x3C))     /* SRAM2 and SRAM3 static memory parameter register */

#define MIUORG                  (*(REG32_PTR_T)(MIU_BASE + 0x40))     /* SDR/DDR selection */
#define MIUDLYDQS               (*(REG32_PTR_T)(MIU_BASE + 0x44))     /* DQS/DQS-rst delay parameter */
#define MIUDLYCLK               (*(REG32_PTR_T)(MIU_BASE + 0x48))     /* SDR/DDR Clock delay parameter */
#define MIU_DSS_SEL_B           (*(REG32_PTR_T)(MIU_BASE + 0x4C))     /* SSTL2 Drive Strength parameter for Bi-direction signal */
#define MIU_DSS_SEL_O           (*(REG32_PTR_T)(MIU_BASE + 0x50))     /* SSTL2 Drive Strength parameter for Output signal */
#define MIU_DSS_SEL_C           (*(REG32_PTR_T)(MIU_BASE + 0x54))     /* SSTL2 Drive Strength parameter for Clock signal */
#define PAD_DSS_SEL_NOR         (*(REG32_PTR_T)(MIU_BASE + 0x58))     /* Wide range I/O Drive Strength parameter for NOR interface */
#define PAD_DSS_SEL_ATA         (*(REG32_PTR_T)(MIU_BASE + 0x5C))     /* Wide range I/O Drive Strength parameter for ATA interface */
#define SSTL2_PAD_ON            (*(REG32_PTR_T)(MIU_BASE + 0x60))     /* SSTL2 pad ON/OFF select */

/* 08. IODMA CONTROLLER */
#define DMA_BASE 0x38400000

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define DMA0_BASE 0x38200000
#endif

#if CONFIG_CPU==S5L8702
#define DMA1_BASE 0x39900000
#elif CONFIG_CPU==S5L8720
#define DMA1_BASE 0x38700000
#endif

#define DMABASE0                (*(REG32_PTR_T)(DMA_BASE))            /* Base address register for channel 0 */
#define DMACON0                 (*(REG32_PTR_T)(DMA_BASE + 0x04))     /* Configuration register for channel 0 */
#define DMATCNT0                (*(REG32_PTR_T)(DMA_BASE + 0x08))     /* Transfer count register for channel 0 */
#define DMACADDR0               (*(REG32_PTR_T)(DMA_BASE + 0x0C))     /* Current memory address register for channel 0 */
#define DMACTCNT0               (*(REG32_PTR_T)(DMA_BASE + 0x10))     /* Current transfer count register for channel 0 */
#define DMACOM0                 (*(REG32_PTR_T)(DMA_BASE + 0x14))     /* Channel 0 command register */
#define DMANOFF0                (*(REG32_PTR_T)(DMA_BASE + 0x18))     /* Channel 0 offset2 register */
#define DMABASE1                (*(REG32_PTR_T)(DMA_BASE + 0x20))     /* Base address register for channel 1 */
#define DMACON1                 (*(REG32_PTR_T)(DMA_BASE + 0x24))     /* Configuration register for channel 1 */
#define DMATCNT1                (*(REG32_PTR_T)(DMA_BASE + 0x28))     /* Transfer count register for channel 1 */
#define DMACADDR1               (*(REG32_PTR_T)(DMA_BASE + 0x2C))     /* Current memory address register for channel 1 */
#define DMACTCNT1               (*(REG32_PTR_T)(DMA_BASE + 0x30))     /* Current transfer count register for channel 1 */
#define DMACOM1                 (*(REG32_PTR_T)(DMA_BASE + 0x34))     /* Channel 1 command register */
#define DMABASE2                (*(REG32_PTR_T)(DMA_BASE + 0x40))     /* Base address register for channel 2 */
#define DMACON2                 (*(REG32_PTR_T)(DMA_BASE + 0x44))     /* Configuration register for channel 2 */
#define DMATCNT2                (*(REG32_PTR_T)(DMA_BASE + 0x48))     /* Transfer count register for channel 2 */
#define DMACADDR2               (*(REG32_PTR_T)(DMA_BASE + 0x4C))     /* Current memory address register for channel 2 */
#define DMACTCNT2               (*(REG32_PTR_T)(DMA_BASE + 0x50))     /* Current transfer count register for channel 2 */
#define DMACOM2                 (*(REG32_PTR_T)(DMA_BASE + 0x54))     /* Channel 2 command register */
#define DMABASE3                (*(REG32_PTR_T)(DMA_BASE + 0x60))     /* Base address register for channel 3 */
#define DMACON3                 (*(REG32_PTR_T)(DMA_BASE + 0x64))     /* Configuration register for channel 3 */
#define DMATCNT3                (*(REG32_PTR_T)(DMA_BASE + 0x68))     /* Transfer count register for channel 3 */
#define DMACADDR3               (*(REG32_PTR_T)(DMA_BASE + 0x6C))     /* Current memory address register for channel 3 */
#define DMACTCNT3               (*(REG32_PTR_T)(DMA_BASE + 0x70))     /* Current transfer count register for channel 3 */
#define DMACOM3                 (*(REG32_PTR_T)(DMA_BASE + 0x74))     /* Channel 3 command register */

#if CONFIG_CPU==S5L8700
#define DMAALLST                (*(REG32_PTR_T)(DMA_BASE + 0x100))     /* All channel status register */
#elif CONFIG_CPU==S5L8701
#define DMABASE4                (*(REG32_PTR_T)(DMA_BASE + 0x80))     /* Base address register for channel 4 */
#define DMACON4                 (*(REG32_PTR_T)(DMA_BASE + 0x84))     /* Configuration register for channel 4 */
#define DMATCNT4                (*(REG32_PTR_T)(DMA_BASE + 0x88))     /* Transfer count register for channel 4 */
#define DMACADDR4               (*(REG32_PTR_T)(DMA_BASE + 0x8C))     /* Current memory address register for channel 4 */
#define DMACTCNT4               (*(REG32_PTR_T)(DMA_BASE + 0x90))     /* Current transfer count register for channel 4 */
#define DMACOM4                 (*(REG32_PTR_T)(DMA_BASE + 0x94))     /* Channel 4 command register */
#define DMABASE5                (*(REG32_PTR_T)(DMA_BASE + 0xA0))     /* Base address register for channel 5 */
#define DMACON5                 (*(REG32_PTR_T)(DMA_BASE + 0xA4))     /* Configuration register for channel 5 */
#define DMATCNT5                (*(REG32_PTR_T)(DMA_BASE + 0xA8))     /* Transfer count register for channel 5 */
#define DMACADDR5               (*(REG32_PTR_T)(DMA_BASE + 0xAC))     /* Current memory address register for channel 5 */
#define DMACTCNT5               (*(REG32_PTR_T)(DMA_BASE + 0xB0))     /* Current transfer count register for channel 5 */
#define DMACOM5                 (*(REG32_PTR_T)(DMA_BASE + 0xB4))     /* Channel 5 command register */
#define DMABASE6                (*(REG32_PTR_T)(DMA_BASE + 0xC0))     /* Base address register for channel 6 */
#define DMACON6                 (*(REG32_PTR_T)(DMA_BASE + 0xC4))     /* Configuration register for channel 6 */
#define DMATCNT6                (*(REG32_PTR_T)(DMA_BASE + 0xC8))     /* Transfer count register for channel 6 */
#define DMACADDR6               (*(REG32_PTR_T)(DMA_BASE + 0xCC))     /* Current memory address register for channel 6 */
#define DMACTCNT6               (*(REG32_PTR_T)(DMA_BASE + 0xD0))     /* Current transfer count register for channel 6 */
#define DMACOM6                 (*(REG32_PTR_T)(DMA_BASE + 0xD4))     /* Channel 6 command register */
#define DMABASE7                (*(REG32_PTR_T)(DMA_BASE + 0xE0))     /* Base address register for channel 7 */
#define DMACON7                 (*(REG32_PTR_T)(DMA_BASE + 0xE4))     /* Configuration register for channel 7 */
#define DMATCNT7                (*(REG32_PTR_T)(DMA_BASE + 0xE8))     /* Transfer count register for channel 7 */
#define DMACADDR7               (*(REG32_PTR_T)(DMA_BASE + 0xEC))     /* Current memory address register for channel 7 */
#define DMACTCNT7               (*(REG32_PTR_T)(DMA_BASE + 0xF0))     /* Current transfer count register for channel 7 */
#define DMACOM7                 (*(REG32_PTR_T)(DMA_BASE + 0xF4))     /* Channel 7 command register */
#define DMABASE8                (*(REG32_PTR_T)(DMA_BASE + 0x100))    /* Base address register for channel 8 */
#define DMACON8                 (*(REG32_PTR_T)(DMA_BASE + 0x104))    /* Configuration register for channel 8 */
#define DMATCNT8                (*(REG32_PTR_T)(DMA_BASE + 0x108))    /* Transfer count register for channel 8 */
#define DMACADDR8               (*(REG32_PTR_T)(DMA_BASE + 0x10C))    /* Current memory address register for channel 8 */
#define DMACTCNT8               (*(REG32_PTR_T)(DMA_BASE + 0x110))    /* Current transfer count register for channel 8 */
#define DMACOM8                 (*(REG32_PTR_T)(DMA_BASE + 0x114))    /* Channel 8 command register */
#define DMAALLST                (*(REG32_PTR_T)(DMA_BASE + 0x180))    /* All channel status register */
#endif

#define DMACON_DEVICE_SHIFT    30
#define DMACON_DIRECTION_SHIFT 29
#define DMACON_DATA_SIZE_SHIFT 22
#define DMACON_BURST_LEN_SHIFT 19
#define DMACOM_START           4
#define DMACOM_CLEARBOTHDONE   7
#define DMAALLST_WCOM0         (1 << 0)
#define DMAALLST_HCOM0         (1 << 1)
#define DMAALLST_DMABUSY0      (1 << 2)
#define DMAALLST_HOLD_SKIP     (1 << 3)
#define DMAALLST_WCOM1         (1 << 4)
#define DMAALLST_HCOM1         (1 << 5)
#define DMAALLST_DMABUSY1      (1 << 6)
#define DMAALLST_WCOM2         (1 << 8)
#define DMAALLST_HCOM2         (1 << 9)
#define DMAALLST_DMABUSY2      (1 << 10)
#define DMAALLST_WCOM3         (1 << 12)
#define DMAALLST_HCOM3         (1 << 13)
#define DMAALLST_DMABUSY3      (1 << 14)
#define DMAALLST_CHAN0_MASK    (0xF << 0)
#define DMAALLST_CHAN1_MASK    (0xF << 4)
#define DMAALLST_CHAN2_MASK    (0xF << 8)
#define DMAALLST_CHAN3_MASK    (0xF << 12)

/* 10. REAL TIMER CLOCK (RTC) */
#define RTC_BASE 0x3D200000

#define RTCCON                  (*(REG32_PTR_T)(RTC_BASE))            /* RTC Control Register */
#define RTCRST                  (*(REG32_PTR_T)(RTC_BASE + 0x04))     /* RTC Round Reset Register */
#define RTCALM                  (*(REG32_PTR_T)(RTC_BASE + 0x08))     /* RTC Alarm Control Register */
#define ALMSEC                  (*(REG32_PTR_T)(RTC_BASE + 0x0C))     /* Alarm Second Data Register */
#define ALMMIN                  (*(REG32_PTR_T)(RTC_BASE + 0x10))     /* Alarm Minute Data Register */
#define ALMHOUR                 (*(REG32_PTR_T)(RTC_BASE + 0x14))     /* Alarm Hour Data Register */
#define ALMDATE                 (*(REG32_PTR_T)(RTC_BASE + 0x18))     /* Alarm Date Data Register */
#define ALMDAY                  (*(REG32_PTR_T)(RTC_BASE + 0x1C))     /* Alarm Day of Week Data Register */
#define ALMMON                  (*(REG32_PTR_T)(RTC_BASE + 0x20))     /* Alarm Month Data Register */
#define ALMYEAR                 (*(REG32_PTR_T)(RTC_BASE + 0x24))     /* Alarm Year Data Register */
#define BCDSEC                  (*(REG32_PTR_T)(RTC_BASE + 0x28))     /* BCD Second Register */
#define BCDMIN                  (*(REG32_PTR_T)(RTC_BASE + 0x2C))     /* BCD Minute Register */
#define BCDHOUR                 (*(REG32_PTR_T)(RTC_BASE + 0x30))     /* BCD Hour Register */
#define BCDDATE                 (*(REG32_PTR_T)(RTC_BASE + 0x34))     /* BCD Date Register */
#define BCDDAY                  (*(REG32_PTR_T)(RTC_BASE + 0x38))     /* BCD Day of Week Register */
#define BCDMON                  (*(REG32_PTR_T)(RTC_BASE + 0x3C))     /* BCD Month Register */
#define BCDYEAR                 (*(REG32_PTR_T)(RTC_BASE + 0x40))     /* BCD Year Register */
#define RTCIM                   (*(REG32_PTR_T)(RTC_BASE + 0x44))     /* RTC Interrupt Mode Register */
#define RTCPEND                 (*(REG32_PTR_T)(RTC_BASE + 0x48))     /* RTC Interrupt Pending Register */

/* 09. WATCHDOG TIMER*/
#define WDT_BASE 0x3C800000

#define WDTCON                  (*(REG32_PTR_T)(WDT_BASE))            /* Control Register */
#define WDTCNT                  (*(REG32_PTR_T)(WDT_BASE + 0x04))     /* 11-bits internal counter */

/* 11. 16 BIT TIMER */
#define TIMER_BASE 0x3C700000

#define TACON                   (*(REG32_PTR_T)(TIMER_BASE))            /* Control Register for timer A */
#define TACMD                   (*(REG32_PTR_T)(TIMER_BASE + 0x04))     /* Command Register for timer A */
#define TADATA0                 (*(REG32_PTR_T)(TIMER_BASE + 0x08))     /* Data0 Register */
#define TADATA1                 (*(REG32_PTR_T)(TIMER_BASE + 0x0C))     /* Data1 Register */
#define TAPRE                   (*(REG32_PTR_T)(TIMER_BASE + 0x10))     /* Pre-scale register */
#define TACNT                   (*(REG32_PTR_T)(TIMER_BASE + 0x14))     /* Counter register */
#define TBCON                   (*(REG32_PTR_T)(TIMER_BASE + 0x20))     /* Control Register for timer B */
#define TBCMD                   (*(REG32_PTR_T)(TIMER_BASE + 0x24))     /* Command Register for timer B */
#define TBDATA0                 (*(REG32_PTR_T)(TIMER_BASE + 0x28))     /* Data0 Register */
#define TBDATA1                 (*(REG32_PTR_T)(TIMER_BASE + 0x2C))     /* Data1 Register */
#define TBPRE                   (*(REG32_PTR_T)(TIMER_BASE + 0x30))     /* Pre-scale register */
#define TBCNT                   (*(REG32_PTR_T)(TIMER_BASE + 0x34))     /* Counter register */
#define TCCON                   (*(REG32_PTR_T)(TIMER_BASE + 0x40))     /* Control Register for timer C */
#define TCCMD                   (*(REG32_PTR_T)(TIMER_BASE + 0x44))     /* Command Register for timer C */
#define TCDATA0                 (*(REG32_PTR_T)(TIMER_BASE + 0x48))     /* Data0 Register */
#define TCDATA1                 (*(REG32_PTR_T)(TIMER_BASE + 0x4C))     /* Data1 Register */
#define TCPRE                   (*(REG32_PTR_T)(TIMER_BASE + 0x50))     /* Pre-scale register */
#define TCCNT                   (*(REG32_PTR_T)(TIMER_BASE + 0x54))     /* Counter register */
#define TDCON                   (*(REG32_PTR_T)(TIMER_BASE + 0x60))     /* Control Register for timer D */
#define TDCMD                   (*(REG32_PTR_T)(TIMER_BASE + 0x64))     /* Command Register for timer D */
#define TDDATA0                 (*(REG32_PTR_T)(TIMER_BASE + 0x68))     /* Data0 Register */
#define TDDATA1                 (*(REG32_PTR_T)(TIMER_BASE + 0x6C))     /* Data1 Register */
#define TDPRE                   (*(REG32_PTR_T)(TIMER_BASE + 0x70))     /* Pre-scale register */
#define TDCNT                   (*(REG32_PTR_T)(TIMER_BASE + 0x74))     /* Counter register */
/* Timer I: 64-bit 5usec timer */
#define TICNTH                  (*(REG32_PTR_T)(TIMER_BASE + 0x80))
#define TICNTL                  (*(REG32_PTR_T)(TIMER_BASE + 0x84))
#define TIUNK08                 (*(REG32_PTR_T)(TIMER_BASE + 0x88))
#define TIUNK0C                 (*(REG32_PTR_T)(TIMER_BASE + 0x8C))      // TBC: DATA0H
#define TIUNK10                 (*(REG32_PTR_T)(TIMER_BASE + 0x90))      //      DATA0L
#define TIUNK14                 (*(REG32_PTR_T)(TIMER_BASE + 0x94))      //      DATA1H
#define TIUNK18                 (*(REG32_PTR_T)(TIMER_BASE + 0x98))      //      DATA1L

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define TIMER_FREQ  (1843200 * 4 * 26 / 1 / 4) /* 47923200 Hz */

#define FIVE_USEC_TIMER         (((uint64_t)TICNTH << 32) | TICNTL)
#define USEC_TIMER              (FIVE_USEC_TIMER * 5)
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/*
 * 16/32-bit timers:
 *
 * - Timers A..D: 16-bit counter, very similar to 16-bit timers described
 *   in S5L8700 DS, it seems that the timers C and D are disabled or not
 *   implemented.
 *
 * - Timers E..H: 32-bit counter, they are like 16-bit timers, but the
 *   interrupt status for all 32-bit timers is located in TSTAT register.
 *
 *   TSTAT:
 *    [26]: TIMERE INTOVF
 *    [25]: TIMERE INT1
 *    [24]: TIMERE INT0
 *    [18]: TIMERF INTOVF
 *    [17]: TIMERF INT1
 *    [16]: TIMERF INT0
 *    [10]: TIMERG INTOVF
 *    [9]:  TIMERG INT1
 *    [8]:  TIMERG INT0
 *    [2]:  TIMERH INTOVF
 *    [1]:  TIMERH INT1
 *    [0]:  TIMERH INT0
 *
 * - Clock source configuration:
 *
 *   TCON[10:8] (Tx_CS)     TCON[6]=0           TCON[6]=1
 *   ------------------     ---------           ---------
 *   000                    PCLK / 2            ECLK / 2
 *   001                    PCLK / 4            ECLK / 4
 *   010                    PCLK / 16           ECLK / 16
 *   011                    PCLK / 64           ECLK / 64
 *   10x (timers E..H)      PCLK                ECLK
 *   10x (timers A..D)      Ext. Clock 0        Ext. Clock 0
 *   11x                    Ext. Clock 1        Ext. Clock 1
 *
 *   On Classic:
 *    Ext. Clock 0: not connected or disabled
 *    Ext. Clock 1: 32768 Hz, external OSC1
 *    ECLK: 12 MHz, external OSC0
 */
#define TIMER_FREQ  12000000    /* ECLK */

#define TECON        (*((REG32_PTR_T)(TIMER_BASE + 0xA0)))
#define TECMD        (*((REG32_PTR_T)(TIMER_BASE + 0xA4)))
#define TEDATA0      (*((REG32_PTR_T)(TIMER_BASE + 0xA8)))
#define TEDATA1      (*((REG32_PTR_T)(TIMER_BASE + 0xAC)))
#define TEPRE        (*((REG32_PTR_T)(TIMER_BASE + 0xB0)))
#define TECNT        (*((REG32_PTR_T)(TIMER_BASE + 0xB4)))
#define TFCON        (*((REG32_PTR_T)(TIMER_BASE + 0xC0)))
#define TFCMD        (*((REG32_PTR_T)(TIMER_BASE + 0xC4)))
#define TFDATA0      (*((REG32_PTR_T)(TIMER_BASE + 0xC8)))
#define TFDATA1      (*((REG32_PTR_T)(TIMER_BASE + 0xCC)))
#define TFPRE        (*((REG32_PTR_T)(TIMER_BASE + 0xD0)))
#define TFCNT        (*((REG32_PTR_T)(TIMER_BASE + 0xD4)))
#define TGCON        (*((REG32_PTR_T)(TIMER_BASE + 0xE0)))
#define TGCMD        (*((REG32_PTR_T)(TIMER_BASE + 0xE4)))
#define TGDATA0      (*((REG32_PTR_T)(TIMER_BASE + 0xE8)))
#define TGDATA1      (*((REG32_PTR_T)(TIMER_BASE + 0xEC)))
#define TGPRE        (*((REG32_PTR_T)(TIMER_BASE + 0xF0)))
#define TGCNT        (*((REG32_PTR_T)(TIMER_BASE + 0xF4)))
#define THCON        (*((REG32_PTR_T)(TIMER_BASE + 0x100)))
#define THCMD        (*((REG32_PTR_T)(TIMER_BASE + 0x104)))
#define THDATA0      (*((REG32_PTR_T)(TIMER_BASE + 0x108)))
#define THDATA1      (*((REG32_PTR_T)(TIMER_BASE + 0x10C)))
#define THPRE        (*((REG32_PTR_T)(TIMER_BASE + 0x110)))
#define THCNT        (*((REG32_PTR_T)(TIMER_BASE + 0x114)))
#define TSTAT        (*((REG32_PTR_T)(TIMER_BASE + 0x118)))

#define USEC_TIMER   TECNT
#endif

/* 12. NAND FLASH CONTROLER */
#if CONFIG_CPU==S5L8700
#define FMC_BASE 0x3C200000
#elif CONFIG_CPU==S5L8701
#define FMC_BASE 0x39400000
#endif

#define FMCTRL0                 (*(REG32_PTR_T)(FMC_BASE + 0x0000))     /* Control Register0 */
#define FMCTRL1                 (*(REG32_PTR_T)(FMC_BASE + 0x0004))     /* Control Register1 */
#define FMCMD                   (*(REG32_PTR_T)(FMC_BASE + 0x0008))     /* Command Register */
#define FMADDR0                 (*(REG32_PTR_T)(FMC_BASE + 0x000C))     /* Address Register0 */
#define FMADDR1                 (*(REG32_PTR_T)(FMC_BASE + 0x0010))     /* Address Register1 */
#define FMADDR2                 (*(REG32_PTR_T)(FMC_BASE + 0x0014))     /* Address Register2 */
#define FMADDR3                 (*(REG32_PTR_T)(FMC_BASE + 0x0018))     /* Address Register3 */
#define FMADDR4                 (*(REG32_PTR_T)(FMC_BASE + 0x001C))     /* Address Register4 */
#define FMADDR5                 (*(REG32_PTR_T)(FMC_BASE + 0x0020))     /* Address Register5 */
#define FMADDR6                 (*(REG32_PTR_T)(FMC_BASE + 0x0024))     /* Address Register6 */
#define FMADDR7                 (*(REG32_PTR_T)(FMC_BASE + 0x0028))     /* Address Register7 */
#define FMANUM                  (*(REG32_PTR_T)(FMC_BASE + 0x002C))     /* Address Counter Register */
#define FMDNUM                  (*(REG32_PTR_T)(FMC_BASE + 0x0030))     /* Data Counter Register */
#define FMDATAW0                (*(REG32_PTR_T)(FMC_BASE + 0x0034))     /* Write Data Register0 */
#define FMDATAW1                (*(REG32_PTR_T)(FMC_BASE + 0x0038))     /* Write Data Register1 */
#define FMDATAW2                (*(REG32_PTR_T)(FMC_BASE + 0x003C))     /* Write Data Register2 */
#define FMDATAW3                (*(REG32_PTR_T)(FMC_BASE + 0x0040))     /* Write Data Register3 */
#define FMCSTAT                 (*(REG32_PTR_T)(FMC_BASE + 0x0048))     /* Status Register */
#define FMSYND0                 (*(REG32_PTR_T)(FMC_BASE + 0x004C))     /* Hamming Syndrome0 */
#define FMSYND1                 (*(REG32_PTR_T)(FMC_BASE + 0x0050))     /* Hamming Syndrome1 */
#define FMSYND2                 (*(REG32_PTR_T)(FMC_BASE + 0x0054))     /* Hamming Syndrome2 */
#define FMSYND3                 (*(REG32_PTR_T)(FMC_BASE + 0x0058))     /* Hamming Syndrome3 */
#define FMSYND4                 (*(REG32_PTR_T)(FMC_BASE + 0x005C))     /* Hamming Syndrome4 */
#define FMSYND5                 (*(REG32_PTR_T)(FMC_BASE + 0x0060))     /* Hamming Syndrome5 */
#define FMSYND6                 (*(REG32_PTR_T)(FMC_BASE + 0x0064))     /* Hamming Syndrome6 */
#define FMSYND7                 (*(REG32_PTR_T)(FMC_BASE + 0x0068))     /* Hamming Syndrome7 */
#define FMFIFO                  (*(REG32_PTR_T)(FMC_BASE + 0x0080))     /* WRITE/READ FIFO FIXME */
#define RSCRTL                  (*(REG32_PTR_T)(FMC_BASE + 0x0100))     /* Reed-Solomon Control Register */
#define RSPARITY0_0             (*(REG32_PTR_T)(FMC_BASE + 0x0110))     /* On-the-fly Parity Register0[31:0] */
#define RSPARITY0_1             (*(REG32_PTR_T)(FMC_BASE + 0x0114))     /* On-the-fly Parity Register0[63:32] */
#define RSPARITY0_2             (*(REG32_PTR_T)(FMC_BASE + 0x0118))     /* On-the-fly Parity Register0[71:64] */
#define RSPARITY1_0             (*(REG32_PTR_T)(FMC_BASE + 0x0120))     /* On-the-fly Parity Register1[31:0] */
#define RSPARITY1_1             (*(REG32_PTR_T)(FMC_BASE + 0x0124))     /* On-the-fly Parity Register1[63:32] */
#define RSPARITY1_2             (*(REG32_PTR_T)(FMC_BASE + 0x0128))     /* On-the-fly Parity Register1[71:64] */
#define RSPARITY2_0             (*(REG32_PTR_T)(FMC_BASE + 0x0130))     /* On-the-fly Parity Register2[31:0] */
#define RSPARITY2_1             (*(REG32_PTR_T)(FMC_BASE + 0x0134))     /* On-the-fly Parity Register2[63:32] */
#define RSPARITY2_2             (*(REG32_PTR_T)(FMC_BASE + 0x0138))     /* On-the-fly Parity Register2[71:64] */
#define RSPARITY3_0             (*(REG32_PTR_T)(FMC_BASE + 0x0140))     /* On-the-fly Parity Register3[31:0] */
#define RSPARITY3_1             (*(REG32_PTR_T)(FMC_BASE + 0x0144))     /* On-the-fly Parity Register3[63:32] */
#define RSPARITY3_2             (*(REG32_PTR_T)(FMC_BASE + 0x0148))     /* On-the-fly Parity Register3[71:64] */
#define RSSYND0_0               (*(REG32_PTR_T)(FMC_BASE + 0x0150))     /* On-the-fly Synd Register0[31:0] */
#define RSSYND0_1               (*(REG32_PTR_T)(FMC_BASE + 0x0154))     /* On-the-fly Synd Register0[63:32] */
#define RSSYND0_2               (*(REG32_PTR_T)(FMC_BASE + 0x0158))     /* On-the-fly Synd Register0[71:64] */
#define RSSYND1_0               (*(REG32_PTR_T)(FMC_BASE + 0x0160))     /* On-the-fly Synd Register1[31:0] */
#define RSSYND1_1               (*(REG32_PTR_T)(FMC_BASE + 0x0164))     /* On-the-fly Synd Register1[63:32] */
#define RSSYND1_2               (*(REG32_PTR_T)(FMC_BASE + 0x0168))     /* On-the-fly Synd Register1[71:64] */
#define RSSYND2_0               (*(REG32_PTR_T)(FMC_BASE + 0x0170))     /* On-the-fly Synd Register2[31:0] */
#define RSSYND2_1               (*(REG32_PTR_T)(FMC_BASE + 0x0174))     /* On-the-fly Synd Register2[63:32] */
#define RSSYND2_2               (*(REG32_PTR_T)(FMC_BASE + 0x0178))     /* On-the-fly Synd Register2[71:64] */
#define RSSYND3_0               (*(REG32_PTR_T)(FMC_BASE + 0x0180))     /* On-the-fly Synd Register3[31:0] */
#define RSSYND3_1               (*(REG32_PTR_T)(FMC_BASE + 0x0184))     /* On-the-fly Synd Register3[63:32] */
#define RSSYND3_2               (*(REG32_PTR_T)(FMC_BASE + 0x0188))     /* On-the-fly Synd Register3[71:64] */
#define FLAGSYND                (*(REG32_PTR_T)(FMC_BASE + 0x0190))     /* On-the-fly ECC Result Flag */
#define FMCTRL0_ENABLEDMA       (1 << 10)
#define FMCTRL0_UNK1            (1 << 11)
#define FMCTRL1_DOTRANSADDR     (1 << 0)
#define FMCTRL1_DOREADDATA      (1 << 1)
#define FMCTRL1_DOWRITEDATA     (1 << 2)
#define FMCTRL1_CLEARWFIFO      (1 << 6)
#define FMCTRL1_CLEARRFIFO      (1 << 7)
#define FMCSTAT_RBBDONE         (1 << 0)
#define FMCSTAT_CMDDONE         (1 << 1)
#define FMCSTAT_ADDRDONE        (1 << 2)
#define FMCSTAT_TRANSDONE       (1 << 3)
#define FMCSTAT_BANK0READY      (1 << 4)
#define FMCSTAT_BANK1READY      (1 << 5)
#define FMCSTAT_BANK2READY      (1 << 6)
#define FMCSTAT_BANK3READY      (1 << 7)

/* 13. SECURE DIGITAL CARD INTERFACE (SDCI) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define SDCI_BASE 0x3C300000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define SDCI_BASE 0x38b00000
#endif

#define SDCI_CTRL               (*(REG32_PTR_T)(SDCI_BASE))            /* Control Register */
#define SDCI_DCTRL              (*(REG32_PTR_T)(SDCI_BASE + 0x04))     /* Data Control Register */
#define SDCI_CMD                (*(REG32_PTR_T)(SDCI_BASE + 0x08))     /* Command Register */
#define SDCI_ARGU               (*(REG32_PTR_T)(SDCI_BASE + 0x0C))     /* Argument Register */
#define SDCI_STATE              (*(REG32_PTR_T)(SDCI_BASE + 0x10))     /* State Register */
#define SDCI_STAC               (*(REG32_PTR_T)(SDCI_BASE + 0x14))     /* Status Clear Register */
#define SDCI_DSTA               (*(REG32_PTR_T)(SDCI_BASE + 0x18))     /* Data Status Register */
#define SDCI_FSTA               (*(REG32_PTR_T)(SDCI_BASE + 0x1C))     /* FIFO Status Register */
#define SDCI_RESP0              (*(REG32_PTR_T)(SDCI_BASE + 0x20))     /* Response0 Register */
#define SDCI_RESP1              (*(REG32_PTR_T)(SDCI_BASE + 0x24))     /* Response1 Register */
#define SDCI_RESP2              (*(REG32_PTR_T)(SDCI_BASE + 0x28))     /* Response2 Register */
#define SDCI_RESP3              (*(REG32_PTR_T)(SDCI_BASE + 0x2C))     /* Response3 Register */
#define SDCI_CLKDIV             (*(REG32_PTR_T)(SDCI_BASE + 0x30))     /* Clock Divider Register */
#define SDIO_CSR                (*(REG32_PTR_T)(SDCI_BASE + 0x34))     /* SDIO Control & Status Register */
#define SDCI_IRQ                (*(REG32_PTR_T)(SDCI_BASE + 0x38))     /* Interrupt Source Register */
#define SDCI_IRQ_MASK           (*(REG32_PTR_T)(SDCI_BASE + 0x3c))

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define SDCI_DATA               (*(REG32_PTR_T)(SDCI_BASE + 0x40))
#define SDCI_DMAADDR         (*(VOID_PTR_PTR_T)(SDCI_BASE + 0x44))
#define SDCI_DMASIZE            (*(REG32_PTR_T)(SDCI_BASE + 0x48))
#define SDCI_DMACOUNT           (*(REG32_PTR_T)(SDCI_BASE + 0x4c))
#define SDCI_RESET              (*(REG32_PTR_T)(SDCI_BASE + 0x6c))
#endif

#define SDCI_CTRL_SDCIEN BIT(0)
#define SDCI_CTRL_CARD_TYPE_MASK BIT(1)
#define SDCI_CTRL_CARD_TYPE_SD 0
#define SDCI_CTRL_CARD_TYPE_MMC BIT(1)
#define SDCI_CTRL_BUS_WIDTH_MASK BITRANGE(2, 3)
#define SDCI_CTRL_BUS_WIDTH_1BIT 0
#define SDCI_CTRL_BUS_WIDTH_4BIT BIT(2)
#define SDCI_CTRL_BUS_WIDTH_8BIT BIT(3)
#define SDCI_CTRL_DMA_EN BIT(4)
#define SDCI_CTRL_L_ENDIAN BIT(5)
#define SDCI_CTRL_DMA_REQ_CON_MASK BIT(6)
#define SDCI_CTRL_DMA_REQ_CON_NEMPTY 0
#define SDCI_CTRL_DMA_REQ_CON_FULL BIT(6)
#define SDCI_CTRL_CLK_SEL_MASK BIT(7)
#define SDCI_CTRL_CLK_SEL_PCLK 0
#define SDCI_CTRL_CLK_SEL_SDCLK BIT(7)
#define SDCI_CTRL_BIT_8 BIT(8)
#define SDCI_CTRL_BIT_14 BIT(14)

#define SDCI_DCTRL_TXFIFORST BIT(0)
#define SDCI_DCTRL_RXFIFORST BIT(1)
#define SDCI_DCTRL_TRCONT_MASK BITRANGE(4, 5)
#define SDCI_DCTRL_TRCONT_TX BIT(4)
#define SDCI_DCTRL_BUS_TEST_MASK BITRANGE(6, 7)
#define SDCI_DCTRL_BUS_TEST_TX BIT(6)
#define SDCI_DCTRL_BUS_TEST_RX BIT(7)

#define SDCI_CDIV_CLKDIV_MASK BITRANGE(0, 7)
#define SDCI_CDIV_CLKDIV(x) ((x) >> 1)
#define SDCI_CDIV_CLKDIV_2 BIT(0)
#define SDCI_CDIV_CLKDIV_4 BIT(1)
#define SDCI_CDIV_CLKDIV_8 BIT(2)
#define SDCI_CDIV_CLKDIV_16 BIT(3)
#define SDCI_CDIV_CLKDIV_32 BIT(4)
#define SDCI_CDIV_CLKDIV_64 BIT(5)
#define SDCI_CDIV_CLKDIV_128 BIT(6)
#define SDCI_CDIV_CLKDIV_256 BIT(7)

#define SDCI_CMD_CMD_NUM_MASK BITRANGE(0, 5)
#define SDCI_CMD_CMD_NUM_SHIFT 0
#define SDCI_CMD_CMD_NUM(x) (x)
#define SDCI_CMD_CMD_TYPE_MASK BITRANGE(6, 7)
#define SDCI_CMD_CMD_TYPE_BC 0
#define SDCI_CMD_CMD_TYPE_BCR BIT(6)
#define SDCI_CMD_CMD_TYPE_AC BIT(7)
#define SDCI_CMD_CMD_TYPE_ADTC (BIT(6) | BIT(7))
#define SDCI_CMD_CMD_RD_WR BIT(8)
#define SDCI_CMD_RES_TYPE_MASK BITRANGE(16, 18)
#define SDCI_CMD_RES_TYPE_NONE 0
#define SDCI_CMD_RES_TYPE_R1 BIT(16)
#define SDCI_CMD_RES_TYPE_R2 BIT(17)
#define SDCI_CMD_RES_TYPE_R3 (BIT(16) | BIT(17))
#define SDCI_CMD_RES_TYPE_R4 BIT(18)
#define SDCI_CMD_RES_TYPE_R5 (BIT(16) | BIT(18))
#define SDCI_CMD_RES_TYPE_R6 (BIT(17) | BIT(18))
#define SDCI_CMD_RES_BUSY BIT(19)
#define SDCI_CMD_RES_SIZE_MASK BIT(20)
#define SDCI_CMD_RES_SIZE_48 0
#define SDCI_CMD_RES_SIZE_136 BIT(20)
#define SDCI_CMD_NCR_NID_MASK BIT(21)
#define SDCI_CMD_NCR_NID_NCR 0
#define SDCI_CMD_NCR_NID_NID BIT(21)
#define SDCI_CMD_CMDSTR BIT(31)

#define SDCI_STATE_DAT_STATE_MASK BITRANGE(0, 3)
#define SDCI_STATE_DAT_STATE_IDLE 0
#define SDCI_STATE_DAT_STATE_DAT_RCV BIT(0)
#define SDCI_STATE_DAT_STATE_CRC_RCV BIT(1)
#define SDCI_STATE_DAT_STATE_DAT_END (BIT(0) | BIT(1))
#define SDCI_STATE_DAT_STATE_DAT_SET BIT(2)
#define SDCI_STATE_DAT_STATE_DAT_OUT (BIT(0) | BIT(2))
#define SDCI_STATE_DAT_STATE_CRC_TIME (BIT(1) | BIT(2))
#define SDCI_STATE_DAT_STATE_CRC_OUT (BIT(0) | BIT(1) | BIT(2))
#define SDCI_STATE_DAT_STATE_ENDB_OUT BIT(3)
#define SDCI_STATE_DAT_STATE_ENDB_STOD (BIT(0) | BIT(3))
#define SDCI_STATE_DAT_STATE_DAT_CRCR (BIT(1) | BIT(3))
#define SDCI_STATE_DAT_STATE_CARD_PRG (BIT(0) | BIT(1) | BIT(3))
#define SDCI_STATE_DAT_STATE_DAT_BUSY (BIT(2) | BIT(3))
#define SDCI_STATE_CMD_STATE_MASK (BIT(4) | BIT(5) | BIT(6))
#define SDCI_STATE_CMD_STATE_CMD_IDLE 0
#define SDCI_STATE_CMD_STATE_CMD_CMDO BIT(4)
#define SDCI_STATE_CMD_STATE_CMD_CRCO BIT(5)
#define SDCI_STATE_CMD_STATE_CMD_TOUT (BIT(4) | BIT(5))
#define SDCI_STATE_CMD_STATE_CMD_RESR BIT(6)
#define SDCI_STATE_CMD_STATE_CMD_INTV (BIT(4) | BIT(6))

#define SDCI_STAC_CLR_CMDEND BIT(2)
#define SDCI_STAC_CLR_BIT_3 BIT(3)
#define SDCI_STAC_CLR_RESEND BIT(4)
#define SDCI_STAC_CLR_DATEND BIT(6)
#define SDCI_STAC_CLR_DAT_CRCEND BIT(7)
#define SDCI_STAC_CLR_CRC_STAEND BIT(8)
#define SDCI_STAC_CLR_RESTOUTE BIT(15)
#define SDCI_STAC_CLR_RESENDE BIT(16)
#define SDCI_STAC_CLR_RESINDE BIT(17)
#define SDCI_STAC_CLR_RESCRCE BIT(18)
#define SDCI_STAC_CLR_WR_DATCRCE BIT(22)
#define SDCI_STAC_CLR_RD_DATCRCE BIT(23)
#define SDCI_STAC_CLR_RD_DATENDE0 BIT(24)
#define SDCI_STAC_CLR_RD_DATENDE1 BIT(25)
#define SDCI_STAC_CLR_RD_DATENDE2 BIT(26)
#define SDCI_STAC_CLR_RD_DATENDE3 BIT(27)
#define SDCI_STAC_CLR_RD_DATENDE4 BIT(28)
#define SDCI_STAC_CLR_RD_DATENDE5 BIT(29)
#define SDCI_STAC_CLR_RD_DATENDE6 BIT(30)
#define SDCI_STAC_CLR_RD_DATENDE7 BIT(31)

#define SDCI_DSTA_CMDRDY BIT(0)
#define SDCI_DSTA_CMDPRO BIT(1)
#define SDCI_DSTA_CMDEND BIT(2)
#define SDCI_DSTA_RESPRO BIT(3)
#define SDCI_DSTA_RESEND BIT(4)
#define SDCI_DSTA_DATPRO BIT(5)
#define SDCI_DSTA_DATEND BIT(6)
#define SDCI_DSTA_DAT_CRCEND BIT(7)
#define SDCI_DSTA_CRC_STAEND BIT(8)
#define SDCI_DSTA_DAT_BUSY BIT(9)
#define SDCI_DSTA_SDCLK_HOLD BIT(12)
#define SDCI_DSTA_DAT0_STATUS BIT(13)
#define SDCI_DSTA_WP_DECT_INPUT BIT(14)
#define SDCI_DSTA_RESTOUTE BIT(15)
#define SDCI_DSTA_RESENDE BIT(16)
#define SDCI_DSTA_RESINDE BIT(17)
#define SDCI_DSTA_RESCRCE BIT(18)
#define SDCI_DSTA_WR_CRC_STATUS_MASK BITRANGE(19, 21)
#define SDCI_DSTA_WR_CRC_STATUS_OK BIT(20)
#define SDCI_DSTA_WR_CRC_STATUS_TXERR (BIT(19) | BIT(21))
#define SDCI_DSTA_WR_CRC_STATUS_CARDERR (BIT(19) | BIT(20) | BIT(21))
#define SDCI_DSTA_WR_DATCRCE BIT(22)
#define SDCI_DSTA_RD_DATCRCE BIT(23)
#define SDCI_DSTA_RD_DATENDE0 BIT(24)
#define SDCI_DSTA_RD_DATENDE1 BIT(25)
#define SDCI_DSTA_RD_DATENDE2 BIT(26)
#define SDCI_DSTA_RD_DATENDE3 BIT(27)
#define SDCI_DSTA_RD_DATENDE4 BIT(28)
#define SDCI_DSTA_RD_DATENDE5 BIT(29)
#define SDCI_DSTA_RD_DATENDE6 BIT(30)
#define SDCI_DSTA_RD_DATENDE7 BIT(31)

#define SDCI_FSTA_RX_FIFO_EMPTY BIT(0)
#define SDCI_FSTA_RX_FIFO_FULL BIT(1)
#define SDCI_FSTA_TX_FIFO_EMPTY BIT(2)
#define SDCI_FSTA_TX_FIFO_FULL BIT(3)

#define SDCI_SDIO_CSR_SDIO_RW_EN BIT(0)
#define SDCI_SDIO_CSR_SDIO_INT_EN BIT(1)
#define SDCI_SDIO_CSR_SDIO_RW_REQ BIT(2)
#define SDCI_SDIO_CSR_SDIO_RW_STOP BIT(3)
#define SDCI_SDIO_CSR_SDIO_INT_PERIOD_MASK BIT(4)
#define SDCI_SDIO_CSR_SDIO_INT_PERIOD_MORE 0
#define SDCI_SDIO_CSR_SDIO_INT_PERIOD_XACT BIT(4)

#define SDCI_IRQ_DAT_DONE_INT BIT(0)
#define SDCI_IRQ_IOCARD_IRQ_INT BIT(1)
#define SDCI_IRQ_READ_WAIT_INT BIT(2)

#define SDCI_IRQ_MASK_MASK_DAT_DONE_INT BIT(0)
#define SDCI_IRQ_MASK_MASK_IOCARD_IRQ_INT BIT(1)
#define SDCI_IRQ_MASK_MASK_READ_WAIT_INT BIT(2)

/* 14. MEMORY STICK HOST CONTROLLER */
#define MS_BASE 0x3C600000
#define MS_CMD_BASE (MS_BASE + 0x1000)

#define MSPRE                   (*(REG32_PTR_T)(MS_BASE))                /* Prescaler Register */
#define MSINTEN                 (*(REG32_PTR_T)(MS_BASE + 0x04))         /* Interrupt Enable Register */
#define MSCMD                   (*(REG32_PTR_T)(MS_CMD_BASE))            /* Command Register */
#define MSFIFO                  (*(REG32_PTR_T)(MS_CMD_BASE + 0x08))     /* Receive/Transmit Register */
#define MSPP                    (*(REG32_PTR_T)(MS_CMD_BASE + 0x10))     /* Parallel Port Control/Data Register */
#define MSCTRL2                 (*(REG32_PTR_T)(MS_CMD_BASE + 0x14))     /* Control Register 2 */
#define MSACD                   (*(REG32_PTR_T)(MS_CMD_BASE + 0x18))     /* ACD Command Register */

/* 15. SPDIF TRANSMITTER (SPDIFOUT) */
#define SPD_BASE 0x3CB00000

#define SPDCLKCON               (*(REG32_PTR_T)(SPD_BASE))            /* Clock Control Register */
#define SPDCON                  (*(REG32_PTR_T)(SPD_BASE + 0x04))     /* Control Register 0020 */
#define SPDBSTAS                (*(REG32_PTR_T)(SPD_BASE + 0x08))     /* Burst Status Register */
#define SPDCSTAS                (*(REG32_PTR_T)(SPD_BASE + 0x0C))     /* Channel Status Register 0x2000 8000 */
#define SPDDAT                  (*(REG32_PTR_T)(SPD_BASE + 0x10))     /* SPDIFOUT Data Buffer */
#define SPDCNT                  (*(REG32_PTR_T)(SPD_BASE + 0x14))     /* Repetition Count Register */

/* 16. REED-SOLOMON ECC CODEC */
#define ECC_BASE 0x39E00000

#define ECC_DATA_PTR            (*(REG32_PTR_T)(ECC_BASE + 0x04))     /* Data Area Start Pointer */
#define ECC_SPARE_PTR           (*(REG32_PTR_T)(ECC_BASE + 0x08))     /* Spare Area Start Pointer */
#define ECC_CTRL                (*(REG32_PTR_T)(ECC_BASE + 0x0C))     /* ECC Control Register */
#define ECC_RESULT              (*(REG32_PTR_T)(ECC_BASE + 0x10))     /* ECC Result */
#define ECC_UNK1                (*(REG32_PTR_T)(ECC_BASE + 0x14))     /* No idea what this is, but the OFW uses it on S5L8701 */
#define ECC_EVAL0               (*(REG32_PTR_T)(ECC_BASE + 0x20))     /* Error Eval0 Poly */
#define ECC_EVAL1               (*(REG32_PTR_T)(ECC_BASE + 0x24))     /* Error Eval1 Poly */
#define ECC_LOC0                (*(REG32_PTR_T)(ECC_BASE + 0x28))     /* Error Loc0 Poly */
#define ECC_LOC1                (*(REG32_PTR_T)(ECC_BASE + 0x2C))     /* Error Loc1 Poly */
#define ECC_PARITY0             (*(REG32_PTR_T)(ECC_BASE + 0x30))     /* Encode Parity0 Poly */
#define ECC_PARITY1             (*(REG32_PTR_T)(ECC_BASE + 0x34))     /* Encode Pariyt1 Poly */
#define ECC_PARITY2             (*(REG32_PTR_T)(ECC_BASE + 0x38))     /* Encode Parity2 Poly */
#define ECC_INT_CLR             (*(REG32_PTR_T)(ECC_BASE + 0x40))     /* Interrupt Clear Register */
#define ECC_SYND0               (*(REG32_PTR_T)(ECC_BASE + 0x44))     /* Syndrom0 Poly */
#define ECC_SYND1               (*(REG32_PTR_T)(ECC_BASE + 0x48))     /* Syndrom1 Poly */
#define ECC_SYND2               (*(REG32_PTR_T)(ECC_BASE + 0x4C))     /* Syndrom2 Poly */
#define ECCCTRL_STARTDECODING  (1 << 0)
#define ECCCTRL_STARTENCODING  (1 << 1)
#define ECCCTRL_STARTDECNOSYND (1 << 2)

/* 17. IIS Tx/Rx INTERFACE */
#define I2S_BASE              0x3CA00000
#define I2S_INTERFACE1_OFFSET   0x300000
#define I2S_INTERFACE2_OFFSET   0xA00000

#define I2SCLKCON               (*(REG32_PTR_T)(I2S_BASE))            /* Clock Control Register */
#define I2STXCON                (*(REG32_PTR_T)(I2S_BASE + 0x04))     /* Tx configuration Register */
#define I2STXCOM                (*(REG32_PTR_T)(I2S_BASE + 0x08))     /* Tx command Register */
#define I2STXDB0                (*(REG32_PTR_T)(I2S_BASE + 0x10))     /* Tx data buffer */
#define I2SRXCON                (*(REG32_PTR_T)(I2S_BASE + 0x30))     /* Rx configuration Register */
#define I2SRXCOM                (*(REG32_PTR_T)(I2S_BASE + 0x34))     /* Rx command Register */
#define I2SRXDB                 (*(REG32_PTR_T)(I2S_BASE + 0x38))     /* Rx data buffer */
#define I2SSTATUS               (*(REG32_PTR_T)(I2S_BASE + 0x3C))     /* status register */

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define I2SCLKDIV               (*(REG32_PTR_T)(I2S_BASE + 0x40))

#define I2SCLKGATE(i)   ((i) == 2 ? CLOCKGATE_I2S2 : \
                         (i) == 1 ? CLOCKGATE_I2S1 : \
                                    CLOCKGATE_I2S0)
#endif

/* 18. IIC BUS INTERFACE */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define IIC_BASE 0x3C900000

#define IICCON                  (*(REG32_PTR_T)(IIC_BASE))            /* Control Register */
#define IICSTAT                 (*(REG32_PTR_T)(IIC_BASE + 0x04))     /* Control/Status Register */
#define IICADD                  (*(REG32_PTR_T)(IIC_BASE + 0x08))     /* Bus Address Register */
#define IICDS                   (*(REG32_PTR_T)(IIC_BASE + 0x0C))
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/*
 *  s5l8702 I2C controller is similar to s5l8700, known differences are:
 *
 *  - IICCON[5] is not used in s5l8702.
 *  - IICCON[13:8] are used to enable interrupts.
 *     IICSTA2[13:8] are used to read the status and write-clear interrupts.
 *    Known interrupts:
 *     [13] STOP on bus (TBC)
 *     [12] START on bus (TBC)
 *     [8] byte transmited or received in Master mode (not tested in Slave)
 *  - IICCON[4] does not clear interrupts, it is enabled when a byte is
 *    transmited or received, in Master mode the tx/rx of the next byte
 *    starts when it is written as "1".
 */
#define IIC_BASE 0x3C600000

#define IICCON(bus)     (*((REG32_PTR_T)(IIC_BASE + 0x300000 * (bus))))
#define IICSTAT(bus)    (*((REG32_PTR_T)(IIC_BASE + 0x04 + 0x300000 * (bus))))
#define IICADD(bus)     (*((REG32_PTR_T)(IIC_BASE + 0x08 + 0x300000 * (bus))))
#define IICDS(bus)      (*((REG32_PTR_T)(IIC_BASE + 0x0C + 0x300000 * (bus))))
#define IICUNK10(bus)   (*((REG32_PTR_T)(IIC_BASE + 0x10 + 0x300000 * (bus))))
#define IICUNK14(bus)   (*((REG32_PTR_T)(IIC_BASE + 0x14 + 0x300000 * (bus))))
#define IICUNK18(bus)   (*((REG32_PTR_T)(IIC_BASE + 0x18 + 0x300000 * (bus))))
#define IICSTA2(bus)    (*((REG32_PTR_T)(IIC_BASE + 0x20 + 0x300000 * (bus))))

#define I2CCLKGATE(i)   ((i) == 1 ? CLOCKGATE_I2C1 : \
                                    CLOCKGATE_I2C0)
#endif

#if CONFIG_CPU == S5L8720
#define I2CCLKGATE_2(i) ((i) == 1 ? CLOCKGATE_I2C1_2 : \
                                    CLOCKGATE_I2C0_2)
#endif

/* 19. SPI (SERIAL PERHIPERAL INTERFACE) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define SPI_BASE 0x3CD00000

#define SPCLKCON                (*(REG32_PTR_T)(SPI_BASE))            /* Clock Control Register */
#define SPCON                   (*(REG32_PTR_T)(SPI_BASE + 0x04))     /* Control Register */
#define SPSTA                   (*(REG32_PTR_T)(SPI_BASE + 0x08))     /* Status Register */
#define SPPIN                   (*(REG32_PTR_T)(SPI_BASE + 0x0C))     /* Pin Control Register */
#define SPTDAT                  (*(REG32_PTR_T)(SPI_BASE + 0x10))     /* Tx Data Register */
#define SPRDAT                  (*(REG32_PTR_T)(SPI_BASE + 0x14))     /* Rx Data Register */
#define SPPRE                   (*(REG32_PTR_T)(SPI_BASE + 0x18))     /* Baud Rate Prescaler Register */
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU == S5L8720
#define SPIBASE(i)      ((i) == 2 ? 0x3d200000 : \
                         (i) == 1 ? 0x3ce00000 : \
                                    0x3c300000)
#define SPICLKGATE(i)   ((i) == 2 ? CLOCKGATE_SPI2 : \
                         (i) == 1 ? CLOCKGATE_SPI1 : \
                                    CLOCKGATE_SPI0)
#define SPIDMA(i)       ((i) == 2 ? 0xd : \
                         (i) == 1 ? 0xf : \
                                    0x5)
#define SPICTRL(i)    (*((REG32_PTR_T)(SPIBASE(i))))
#define SPISETUP(i)   (*((REG32_PTR_T)(SPIBASE(i) + 0x4)))
#define SPISTATUS(i)  (*((REG32_PTR_T)(SPIBASE(i) + 0x8)))
#define SPIPIN(i)     (*((REG32_PTR_T)(SPIBASE(i) + 0xc)))
#define SPITXDATA(i)  (*((REG32_PTR_T)(SPIBASE(i) + 0x10)))
#define SPIRXDATA(i)  (*((REG32_PTR_T)(SPIBASE(i) + 0x20)))
#define SPICLKDIV(i)  (*((REG32_PTR_T)(SPIBASE(i) + 0x30)))
#define SPIRXLIMIT(i) (*((REG32_PTR_T)(SPIBASE(i) + 0x34)))
#define SPIDD(i)      (*((REG32_PTR_T)(SPIBASE(i) + 0x38)))  /* TBC */
#endif

#if CONFIG_CPU == S5L8720
#define SPIUNK3C(i)   (*((REG32_PTR_T)(SPIBASE(i) + 0x3c)))
#define SPIUNK40(i)   (*((REG32_PTR_T)(SPIBASE(i) + 0x40)))
#define SPIUNK4C(i)   (*((REG32_PTR_T)(SPIBASE(i) + 0x4c)))

#define SPICLKGATE_2(i) ((i) == 2 ? CLOCKGATE_SPI2_2 : \
                         (i) == 1 ? CLOCKGATE_SPI1_2 : \
                                    CLOCKGATE_SPI0_2)
#endif

/* 20. ADC CONTROLLER */
#define ADC_BASE 0x3CE00000

#define ADCCON                  (*(REG32_PTR_T)(ADC_BASE))            /* ADC Control Register */
#define ADCTSC                  (*(REG32_PTR_T)(ADC_BASE + 0x04))     /* ADC Touch Screen Control Register */
#define ADCDLY                  (*(REG32_PTR_T)(ADC_BASE + 0x08))     /* ADC Start or Interval Delay Register */
#define ADCDAT0                 (*(REG32_PTR_T)(ADC_BASE + 0x0C))     /* ADC Conversion Data Register */
#define ADCDAT1                 (*(REG32_PTR_T)(ADC_BASE + 0x10))     /* ADC Conversion Data Register */
#define ADCUPDN                 (*(REG32_PTR_T)(ADC_BASE + 0x14))     /* Stylus Up or Down Interrpt Register */

/* 21. USB 2.0 FUNCTION CONTROLER SPECIAL REGISTER */
#define USB_FC_BASE 0x38800000

#define USB_IR                  (*(REG32_PTR_T)(USB_FC_BASE))            /* Index Register */
#define USB_EIR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x04))     /* Endpoint Interrupt Register */
#define USB_EIER                (*(REG32_PTR_T)(USB_FC_BASE + 0x08))     /* Endpoint Interrupt Enable Register */
#define USB_FAR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x0C))     /* Function Address Register */
#define USB_FNR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x10))     /* Frame Number Register */
#define USB_EDR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x14))     /* Endpoint Direction Register */
#define USB_TR                  (*(REG32_PTR_T)(USB_FC_BASE + 0x18))     /* Test Register */
#define USB_SSR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x1C))     /* System Status Register */
#define USB_SCR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x20))     /* System Control Register */
#define USB_EP0SR               (*(REG32_PTR_T)(USB_FC_BASE + 0x24))     /* EP0 Status Register */
#define USB_EP0CR               (*(REG32_PTR_T)(USB_FC_BASE + 0x28))     /* EP0 Control Register */
#define USB_ESR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x2C))     /* Endpoints Status Register */
#define USB_ECR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x30))     /* Endpoints Control Register */
#define USB_BRCR                (*(REG32_PTR_T)(USB_FC_BASE + 0x34))     /* Byte Read Count Register */
#define USB_BWCR                (*(REG32_PTR_T)(USB_FC_BASE + 0x38))     /* Byte Write Count Register */
#define USB_MPR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x3C))     /* Max Packet Register */
#define USB_MCR                 (*(REG32_PTR_T)(USB_FC_BASE + 0x40))     /* Master Control Register */
#define USB_MTCR                (*(REG32_PTR_T)(USB_FC_BASE + 0x44))     /* Master Transfer Counter Register */
#define USB_MFCR                (*(REG32_PTR_T)(USB_FC_BASE + 0x48))     /* Master FIFO Counter Register */
#define USB_MTTCR1              (*(REG32_PTR_T)(USB_FC_BASE + 0x4C))     /* Master Total Transfer Counter1 Register */
#define USB_MTTCR2              (*(REG32_PTR_T)(USB_FC_BASE + 0x50))     /* Master Total Transfer Counter2 Register */
#define USB_EP0BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x60))     /* EP0 Buffer Register */
#define USB_EP1BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x64))     /* EP1 Buffer Register */
#define USB_EP2BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x68))     /* EP2 Buffer Register */
#define USB_EP3BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x6C))     /* EP3 Buffer Register */
#define USB_EP4BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x70))     /* EP4 Buffer Register */
#define USB_EP5BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x74))     /* EP5 Buffer Register */
#define USB_EP6BR               (*(REG32_PTR_T)(USB_FC_BASE + 0x78))     /* EP6 Buffer Register */
#define USB_MICR                (*(REG32_PTR_T)(USB_FC_BASE + 0x84))     /* Master Interface Counter Register */
#define USB_MBAR1               (*(REG32_PTR_T)(USB_FC_BASE + 0x88))     /* Memory Base Address Register1 */
#define USB_MBAR2               (*(REG32_PTR_T)(USB_FC_BASE + 0x8C))     /* Memory Base Address Register2 */
#define USB_MCAR1               (*(REG32_PTR_T)(USB_FC_BASE + 0x94))     /* Memory Current Address Register1 */
#define USB_MCAR2               (*(REG32_PTR_T)(USB_FC_BASE + 0x98))     /* Memory Current Address Register2 */

/* 22. USB 1.1 HOST CONTROLLER SPECIAL REGISTER */
#define USB_HC_BASE 0x38600000

#define HcRevision              (*(REG32_PTR_T)(USB_HC_BASE))
#define HcControl               (*(REG32_PTR_T)(USB_HC_BASE + 0x04))
#define HcCommandStatus         (*(REG32_PTR_T)(USB_HC_BASE + 0x08))
#define HcInterruptStatus       (*(REG32_PTR_T)(USB_HC_BASE + 0x0C))
#define HcInterruptEnable       (*(REG32_PTR_T)(USB_HC_BASE + 0x10))
#define HcInterruptDisable      (*(REG32_PTR_T)(USB_HC_BASE + 0x14))
#define HcHCCA                  (*(REG32_PTR_T)(USB_HC_BASE + 0x18))
#define HcPeriodCurrentED       (*(REG32_PTR_T)(USB_HC_BASE + 0x1C))
#define HcControlHeadED         (*(REG32_PTR_T)(USB_HC_BASE + 0x20))
#define HcControlCurrentED      (*(REG32_PTR_T)(USB_HC_BASE + 0x24))
#define HcBulkHeadED            (*(REG32_PTR_T)(USB_HC_BASE + 0x28))
#define HcBulkCurrentED         (*(REG32_PTR_T)(USB_HC_BASE + 0x2C))
#define HcDoneHead              (*(REG32_PTR_T)(USB_HC_BASE + 0x30))
#define HcFmInterval            (*(REG32_PTR_T)(USB_HC_BASE + 0x34))
#define HcFmRemaining           (*(REG32_PTR_T)(USB_HC_BASE + 0x38))
#define HcFmNumber              (*(REG32_PTR_T)(USB_HC_BASE + 0x3C))
#define HcPeriodicStart         (*(REG32_PTR_T)(USB_HC_BASE + 0x40))
#define HcLSThreshold           (*(REG32_PTR_T)(USB_HC_BASE + 0x44))
#define HcRhDescriptorA         (*(REG32_PTR_T)(USB_HC_BASE + 0x48))
#define HcRhDescriptorB         (*(REG32_PTR_T)(USB_HC_BASE + 0x4C))
#define HcRhStatus              (*(REG32_PTR_T)(USB_HC_BASE + 0x50))
#define HcRhPortStatus          (*(REG32_PTR_T)(USB_HC_BASE + 0x54))

/* 23. USB 2.0 PHY CONTROL */
#define USB_PHY_BASE 0x3C400000

#define PHYCTRL                 (*(REG32_PTR_T)(USB_PHY_BASE))        /* USB2.0 PHY Control Register */
#define PHYPWR                  (*(REG32_PTR_T)(USB_PHY_BASE + 0x04)) /* USB2.0 PHY Power Control Register */
#define URSTCON                 (*(REG32_PTR_T)(USB_PHY_BASE + 0x08)) /* USB Reset Control Register */
#define UCLKCON                 (*(REG32_PTR_T)(USB_PHY_BASE + 0x10)) /* USB Clock Control Register */

/* 24. GPIO PORT CONTROL */
#define GPIO_BASE 0x3CF00000

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define GPIO_OFFSET_BITS 4
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define GPIO_OFFSET_BITS 5
#endif

#define PCON(i)       (*((REG32_PTR_T)(GPIO_BASE + ((i) << GPIO_OFFSET_BITS))))
#define PDAT(i)       (*((REG32_PTR_T)(GPIO_BASE + 0x04 + ((i) << GPIO_OFFSET_BITS))))
#define PUNA(i)       (*((REG32_PTR_T)(GPIO_BASE + 0x08 + ((i) << GPIO_OFFSET_BITS))))
#define PUNB(i)       (*((REG32_PTR_T)(GPIO_BASE + 0x0c + ((i) << GPIO_OFFSET_BITS))))

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define PUNC(i)       (*((REG32_PTR_T)(GPIO_BASE + 0x10 + ((i) << GPIO_OFFSET_BITS))))
#endif

#define PCON0                   PCON(0)      /* Configures the pins of port 0 */
#define PDAT0                   PDAT(0)      /* The data register for port 0 */
#define PCON1                   PCON(1)      /* Configures the pins of port 1 */
#define PDAT1                   PDAT(1)      /* The data register for port 1 */
#define PCON2                   PCON(2)      /* Configures the pins of port 2 */
#define PDAT2                   PDAT(2)      /* The data register for port 2 */
#define PCON3                   PCON(3)      /* Configures the pins of port 3 */
#define PDAT3                   PDAT(3)      /* The data register for port 3 */
#define PCON4                   PCON(4)      /* Configures the pins of port 4 */
#define PDAT4                   PDAT(4)      /* The data register for port 4 */
#define PCON5                   PCON(5)      /* Configures the pins of port 5 */
#define PDAT5                   PDAT(5)      /* The data register for port 5 */
#define PUNK5                   PUNB(5)      /* Unknown thing for port 5 */
#define PCON6                   PCON(6)      /* Configures the pins of port 6 */
#define PDAT6                   PDAT(6)      /* The data register for port 6 */
#define PCON7                   PCON(7)      /* Configures the pins of port 7 */
#define PDAT7                   PDAT(7)      /* The data register for port 7 */
#define PUNK8                   PUNB(8)      /* Unknown thing for port 8 */
#define PCON10                  PCON(10)     /* Configures the pins of port 10 */
#define PDAT10                  PDAT(10)     /* The data register for port 10 */
#define PCON11                  PCON(11)     /* Configures the pins of port 11 */
#define PDAT11                  PDAT(11)     /* The data register for port 11 */
#define PCON12                  PCON(12)     /* Configures the pins of port 12 */
#define PDAT12                  PDAT(12)     /* The data register for port 12 */
#define PCON13                  PCON(13)     /* Configures the pins of port 13 */
#define PDAT13                  PDAT(13)     /* The data register for port 13 */
#define PCON14                  PCON(14)     /* Configures the pins of port 14 */
#define PDAT14                  PDAT(14)     /* The data register for port 14 */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
#define PCON15                  PCON(15)     /* Configures the pins of port 15 */
#define PDAT15                  PDAT(15)     /* Configures the pins of port 15 */
#define PUNK15                  PUNB(15)     /* Unknown thing for port 15 */
#endif

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define PCON_ASRAM              PCON15                           /* Configures the pins of port nor flash */
#define PCON_SDRAM              PDAT15                           /* Configures the pins of port sdram */
#elif CONFIG_CPU==S5L8702
#define GPIO_N_GROUPS 16
#define GPIOCMD       (*((REG32_PTR_T)(GPIO_BASE + 0x200)))
#define GPIOUNK380    (*((REG32_PTR_T)(GPIO_BASE + 0x380)))
#elif CONFIG_CPU==S5L8720
#define GPIO_N_GROUPS 15
#define GPIOCMD       (*((REG32_PTR_T)(GPIO_BASE + 0x1e0)))
#define GPIOUNK388    (*((REG32_PTR_T)(GPIO_BASE + 0x388)))
#define GPIOUNK384    (*((REG32_PTR_T)(GPIO_BASE + 0x384)))
#endif

/* 25. UART */
#if CONFIG_CPU==S5L8700
/* s5l8700 UC87XX HW: 1 UARTC, 2 ports */
#define S5L8700_N_PORTS         2

#define UARTC_BASE_ADDR         0x3CC00000
#define UARTC_N_PORTS           2
#define UARTC_PORT_OFFSET       0x8000
#elif CONFIG_CPU==S5L8701
/* s5l8701 UC87XX HW: 3 UARTC, 1 port per UARTC */
#define S5L8701_N_UARTC         3

#define UARTC0_BASE_ADDR        0x3CC00000
#define UARTC0_N_PORTS          1
#define UARTC0_PORT_OFFSET      0x0
#define UARTC1_BASE_ADDR        0x3CC08000
#define UARTC1_N_PORTS          1
#define UARTC1_PORT_OFFSET      0x0
#define UARTC2_BASE_ADDR        0x3CC0C000
#define UARTC2_N_PORTS          1
#define UARTC2_PORT_OFFSET      0x0
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/* s5l8702/s5l8720 UC87XX HW: 1 UARTC, 4 ports */
#define UARTC_BASE_ADDR     0x3CC00000
#define UARTC_N_PORTS       4
#define UARTC_PORT_OFFSET   0x4000
#if CONFIG_CPU == S5L8720
#define UARTC_DMA_BASE_ADDR   0x3DB00000
#define UARTC_DMA_PORT_OFFSET 0x100000
#endif
#endif

/* 26. LCD INTERFACE CONTROLLER */
#if CONFIG_CPU==S5L8700
#define LCD_BASE 0x3C100000
#elif CONFIG_CPU==S5L8701
#define LCD_BASE 0x38600000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define LCD_BASE 0x38300000
#endif

#define LCD_CON                 (*(REG32_PTR_T)(LCD_BASE))         /* Control register. */
#define LCD_WCMD                (*(REG32_PTR_T)(LCD_BASE + 0x04))  /* Write command register. */
#define LCD_RCMD                (*(REG32_PTR_T)(LCD_BASE + 0x0C))  /* Read command register. */
#define LCD_RDATA               (*(REG32_PTR_T)(LCD_BASE + 0x10))  /* Read data register. */
#define LCD_DBUFF               (*(REG32_PTR_T)(LCD_BASE + 0x14))  /* Read Data buffer */
#define LCD_INTCON              (*(REG32_PTR_T)(LCD_BASE + 0x18))  /* Interrupt control register */
#define LCD_STATUS              (*(REG32_PTR_T)(LCD_BASE + 0x1C))  /* LCD Interface status 0106 */
#define LCD_PHTIME              (*(REG32_PTR_T)(LCD_BASE + 0x20))  /* Phase time register 0060 */
#define LCD_RST_TIME            (*(REG32_PTR_T)(LCD_BASE + 0x24))  /* Reset active period 07FF */
#define LCD_DRV_RST             (*(REG32_PTR_T)(LCD_BASE + 0x28))  /* Reset drive signal */
#define LCD_WDATA               (*(REG32_PTR_T)(LCD_BASE + 0x40))  /* Write data register (0x40...0x5C) FIXME */

/* 27. CLCD CONTROLLER */
#define LCDBASE 0x39200000

#define LCDCON1     (*(REG32_PTR_T)(LCDBASE))        /* LCD control 1 register */
#define LCDCON2     (*(REG32_PTR_T)(LCDBASE + 0x04)) /* LCD control 2 register */
#define LCDTCON1    (*(REG32_PTR_T)(LCDBASE + 0x08)) /* LCD time control 1 register */
#define LCDTCON2    (*(REG32_PTR_T)(LCDBASE + 0x0C)) /* LCD time control 2 register */
#define LCDTCON3    (*(REG32_PTR_T)(LCDBASE + 0x10)) /* LCD time control 3 register */
#define LCDOSD1     (*(REG32_PTR_T)(LCDBASE + 0x14)) /* LCD OSD control 1 register */
#define LCDOSD2     (*(REG32_PTR_T)(LCDBASE + 0x18)) /* LCD OSD control 2 register */
#define LCDOSD3     (*(REG32_PTR_T)(LCDBASE + 0x1C)) /* LCD OSD control 3 register */
#define LCDB1SADDR1 (*(REG32_PTR_T)(LCDBASE + 0x20)) /* Frame buffer start address register for Back-Ground buffer 1 */
#define LCDB2SADDR1 (*(REG32_PTR_T)(LCDBASE + 0x24)) /* Frame buffer start address register for Back-Ground buffer 2 */
#define LCDF1SADDR1 (*(REG32_PTR_T)(LCDBASE + 0x28)) /* Frame buffer start address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR1 (*(REG32_PTR_T)(LCDBASE + 0x2C)) /* Frame buffer start address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR2 (*(REG32_PTR_T)(LCDBASE + 0x30)) /* Frame buffer end address register for Back-Ground buffer 1 */
#define LCDB2SADDR2 (*(REG32_PTR_T)(LCDBASE + 0x34)) /* Frame buffer end address register for Back-Ground buffer 2 */
#define LCDF1SADDR2 (*(REG32_PTR_T)(LCDBASE + 0x38)) /* Frame buffer end address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR2 (*(REG32_PTR_T)(LCDBASE + 0x3C)) /* Frame buffer end address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR3 (*(REG32_PTR_T)(LCDBASE + 0x40)) /* Virtual screen address set for Back-Ground buffer 1 */
#define LCDB2SADDR3 (*(REG32_PTR_T)(LCDBASE + 0x44)) /* Virtual screen address set for Back-Ground buffer 2 */
#define LCDF1SADDR3 (*(REG32_PTR_T)(LCDBASE + 0x48)) /* Virtual screen address set for Fore-Ground(OSD) buffer 1 */
#define LCDF2SADDR3 (*(REG32_PTR_T)(LCDBASE + 0x4C)) /* Virtual screen address set for Fore-Ground(OSD) buffer 2 */
#define LCDINTCON   (*(REG32_PTR_T)(LCDBASE + 0x50)) /* Indicate the LCD interrupt control register */
#define LCDKEYCON   (*(REG32_PTR_T)(LCDBASE + 0x54)) /* Color key control register */
#define LCDCOLVAL   (*(REG32_PTR_T)(LCDBASE + 0x58)) /* Color key value ( transparent value) register */
#define LCDBGCON    (*(REG32_PTR_T)(LCDBASE + 0x5C)) /* Back-Ground color control */
#define LCDFGCON    (*(REG32_PTR_T)(LCDBASE + 0x60)) /* Fore-Ground color control */
#define LCDDITHMODE (*(REG32_PTR_T)(LCDBASE + 0x64)) /* Dithering mode register. */

/* 28. ATA CONTROLLER */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define ATA_BASE 0x38E00000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define ATA_BASE 0x38700000
#endif

#define ATA_CONTROL             (*(REG32_PTR_T)(ATA_BASE))            /* Enable and clock down status */
#define ATA_STATUS              (*(REG32_PTR_T)(ATA_BASE + 0x04))     /* Status */
#define ATA_COMMAND             (*(REG32_PTR_T)(ATA_BASE + 0x08))     /* Command */
#define ATA_SWRST               (*(REG32_PTR_T)(ATA_BASE + 0x0C))     /* Software reset */
#define ATA_IRQ                 (*(REG32_PTR_T)(ATA_BASE + 0x10))     /* Interrupt sources */
#define ATA_IRQ_MASK            (*(REG32_PTR_T)(ATA_BASE + 0x14))     /* Interrupt mask */
#define ATA_CFG                 (*(REG32_PTR_T)(ATA_BASE + 0x18))     /* Configuration for ATA interface */
#define ATA_MDMA_TIME           (*(REG32_PTR_T)(ATA_BASE + 0x28))
#define ATA_PIO_TIME            (*(REG32_PTR_T)(ATA_BASE + 0x2C))     /* PIO timing */
#define ATA_UDMA_TIME           (*(REG32_PTR_T)(ATA_BASE + 0x30))     /* UDMA timing */
#define ATA_XFR_NUM             (*(REG32_PTR_T)(ATA_BASE + 0x34))     /* Transfer number */
#define ATA_XFR_CNT             (*(REG32_PTR_T)(ATA_BASE + 0x38))     /* Current transfer count */
#define ATA_TBUF_START       (*(VOID_PTR_PTR_T)(ATA_BASE + 0x3C))     /* Start address of track buffer */
#define ATA_TBUF_SIZE           (*(REG32_PTR_T)(ATA_BASE + 0x40))     /* Size of track buffer */
#define ATA_SBUF_START       (*(VOID_PTR_PTR_T)(ATA_BASE + 0x44))     /* Start address of Source buffer1 */
#define ATA_SBUF_SIZE           (*(REG32_PTR_T)(ATA_BASE + 0x48))     /* Size of source buffer1 */
#define ATA_CADR_TBUF        (*(VOID_PTR_PTR_T)(ATA_BASE + 0x4C))     /* Current write address of track buffer */
#define ATA_CADR_SBUF        (*(VOID_PTR_PTR_T)(ATA_BASE + 0x50))     /* Current read address of source buffer */
#define ATA_PIO_DTR             (*(REG32_PTR_T)(ATA_BASE + 0x54))     /* PIO device data register */
#define ATA_PIO_FED             (*(REG32_PTR_T)(ATA_BASE + 0x58))     /* PIO device Feature/Error register */
#define ATA_PIO_SCR             (*(REG32_PTR_T)(ATA_BASE + 0x5C))     /* PIO sector count register */
#define ATA_PIO_LLR             (*(REG32_PTR_T)(ATA_BASE + 0x60))     /* PIO device LBA low register */
#define ATA_PIO_LMR             (*(REG32_PTR_T)(ATA_BASE + 0x64))     /* PIO device LBA middle register */
#define ATA_PIO_LHR             (*(REG32_PTR_T)(ATA_BASE + 0x68))     /* PIO device LBA high register */
#define ATA_PIO_DVR             (*(REG32_PTR_T)(ATA_BASE + 0x6C))     /* PIO device register */
#define ATA_PIO_CSD             (*(REG32_PTR_T)(ATA_BASE + 0x70))     /* PIO device command/status register */
#define ATA_PIO_DAD             (*(REG32_PTR_T)(ATA_BASE + 0x74))     /* PIO control/alternate status register */
#define ATA_PIO_READY           (*(REG32_PTR_T)(ATA_BASE + 0x78))     /* PIO data read/write ready */
#define ATA_PIO_RDATA           (*(REG32_PTR_T)(ATA_BASE + 0x7C))     /* PIO read data from device register */
#define ATA_BUS_FIFO_STATUS     (*(REG32_PTR_T)(ATA_BASE + 0x80))     /* Reserved */
#define ATA_FIFO_STATUS         (*(REG32_PTR_T)(ATA_BASE + 0x84))     /* Reserved */
#define ATA_DMA_ADDR         (*(VOID_PTR_PTR_T)(ATA_BASE + 0x88))

/* 29. CHIP ID */
#define CHIPID_BASE 0x3D100000

#define CHIPID_REG_ONE (*(REG32_PTR_T)(CHIPID_BASE))        /* Receive the first 32 bits from a fuse box */
#define CHIPID_REG_TWO (*(REG32_PTR_T)(CHIPID_BASE + 0x04)) /* Receive the other 8 bits from a fuse box */

#if CONFIG_CPU == S5L8720
#define CHIPID_INFO    (*(REG32_PTR_T)(CHIPID_BASE + 0x08))
#endif

/*
The following peripherals are not present in the Samsung S5L8700 datasheet.
Information for them was gathered solely by reverse-engineering Apple's firmware.
*/

/* Hardware AES crypto unit - S5L8701+ */
#if CONFIG_CPU==S5L8701
#define AES_BASE 0x39800000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define AES_BASE 0x38c00000
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define AESCONTROL             (*((REG32_PTR_T)(AES_BASE)))
#define AESGO                  (*((REG32_PTR_T)(AES_BASE + 0x04)))
#define AESUNKREG0             (*((REG32_PTR_T)(AES_BASE + 0x08)))
#define AESSTATUS              (*((REG32_PTR_T)(AES_BASE + 0x0c)))
#define AESUNKREG1             (*((REG32_PTR_T)(AES_BASE + 0x10)))
#define AESKEYLEN              (*((REG32_PTR_T)(AES_BASE + 0x14)))
#define AESOUTSIZE             (*((REG32_PTR_T)(AES_BASE + 0x18)))
#define AESINSIZE              (*((REG32_PTR_T)(AES_BASE + 0x24)))
#define AESAUXSIZE             (*((REG32_PTR_T)(AES_BASE + 0x2c)))
#define AESSIZE3               (*((REG32_PTR_T)(AES_BASE + 0x34)))
#define AESTYPE                (*((REG32_PTR_T)(AES_BASE + 0x6c)))
#endif

#if CONFIG_CPU==S5L8701
#define AESOUTADDR             (*((REG32_PTR_T)(AES_BASE + 0x20)))
#define AESINADDR              (*((REG32_PTR_T)(AES_BASE + 0x28)))
#define AESAUXADDR             (*((REG32_PTR_T)(AES_BASE + 0x30)))
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define AESOUTADDR          (*((VOID_PTR_PTR_T)(AES_BASE + 0x20)))
#define AESINADDR     (*((CONST_VOID_PTR_PTR_T)(AES_BASE + 0x28)))
#define AESAUXADDR          (*((VOID_PTR_PTR_T)(AES_BASE + 0x30)))
#define AESKEY                   ((REG32_PTR_T)(AES_BASE + 0x4c))
#define AESIV                    ((REG32_PTR_T)(AES_BASE + 0x74))
#define AESTYPE2               (*((REG32_PTR_T)(AES_BASE + 0x88)))
#define AESUNKREG2             (*((REG32_PTR_T)(AES_BASE + 0x8c)))
#endif

/* SHA-1 unit - S5L8701+ */
#if CONFIG_CPU==S5L8701
#define SHA1_BASE 0x3C600000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU == S5L8720
#define SHA1_BASE 0x38000000
#endif

#define SHA1CONFIG    (*((REG32_PTR_T)(SHA1_BASE)))
#define SHA1RESET     (*((REG32_PTR_T)(SHA1_BASE + 0x04)))

#if CONFIG_CPU == S5L8720
#define SHA1UNK10     (*((REG32_PTR_T)(SHA1_BASE + 0x10)))
#endif

#define SHA1RESULT      ((REG32_PTR_T)(SHA1_BASE + 0x20))
#define SHA1DATAIN      ((REG32_PTR_T)(SHA1_BASE + 0x40))

#if CONFIG_CPU == S5L8720
#define SHA1UNK80     (*((REG32_PTR_T)(SHA1_BASE + 0x80)))
#endif

/* Clickwheel controller - S5L8701+ */
#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define WHEEL_BASE 0x3C200000

#define WHEEL00      (*((REG32_PTR_T)(WHEEL_BASE)))
#define WHEEL04      (*((REG32_PTR_T)(WHEEL_BASE + 0x04)))
#define WHEEL08      (*((REG32_PTR_T)(WHEEL_BASE + 0x08)))
#define WHEEL0C      (*((REG32_PTR_T)(WHEEL_BASE + 0x0C)))
#define WHEEL10      (*((REG32_PTR_T)(WHEEL_BASE + 0x10)))
#define WHEELINT     (*((REG32_PTR_T)(WHEEL_BASE + 0x14)))
#define WHEELRX      (*((REG32_PTR_T)(WHEEL_BASE + 0x18)))
#define WHEELTX      (*((REG32_PTR_T)(WHEEL_BASE + 0x1C)))
#endif

/* Synopsys OTG - S5L8701 only */
#if CONFIG_CPU==S5L8701
#define OTGBASE 0x38800000
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define OTGBASE 0x38400000
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
#define PHYBASE 0x3C400000

/* OTG PHY control registers */
#define OPHYPWR     (*((REG32_PTR_T)(PHYBASE + 0x00)))
#define OPHYCLK     (*((REG32_PTR_T)(PHYBASE + 0x04)))
#define ORSTCON     (*((REG32_PTR_T)(PHYBASE + 0x08)))
#define OPHYUNK3    (*((REG32_PTR_T)(PHYBASE + 0x18)))
#define OPHYUNK1    (*((REG32_PTR_T)(PHYBASE + 0x1c)))
#define OPHYUNK2    (*((REG32_PTR_T)(PHYBASE + 0x44)))
#endif

#if CONFIG_CPU==S5L8701
/* 7 available EPs (0b00000000011101010000000001101011), 6 used */
#define USB_NUM_ENDPOINTS 6
#elif CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/* 9 available EPs (0b00000001111101010000000111101011), 6 used */
#define USB_NUM_ENDPOINTS 6
#endif

#if CONFIG_CPU==S5L8701
/* Define this if the DWC implemented on this SoC does not support
   dedicated FIFOs. */
#define USB_DW_SHARED_FIFO
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/* Define this if the DWC implemented on this SoC does not support
   DMA or you want to disable it. */
// #define USB_DW_ARCH_SLAVE
#endif

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/////INTERRUPT CONTROLLERS/////
#define VIC_BASE   0x38E00000
#define VIC_OFFSET 0x1000

#define VICBASE(v) (VIC_BASE + VIC_OFFSET * (v))

#define VICIRQSTATUS(v)                (*((REG32_PTR_T)(VICBASE(v))))
#define VICFIQSTATUS(v)                (*((REG32_PTR_T)(VICBASE(v) + 0x04)))
#define VICRAWINTR(v)                  (*((REG32_PTR_T)(VICBASE(v) + 0x08)))
#define VICINTSELECT(v)                (*((REG32_PTR_T)(VICBASE(v) + 0x0C)))
#define VICINTENABLE(v)                (*((REG32_PTR_T)(VICBASE(v) + 0x10)))
#define VICINTENCLEAR(v)               (*((REG32_PTR_T)(VICBASE(v) + 0x14)))
#define VICSOFTINT(v)                  (*((REG32_PTR_T)(VICBASE(v) + 0x18)))
#define VICSOFTINTCLEAR(v)             (*((REG32_PTR_T)(VICBASE(v) + 0x1C)))
#define VICPROTECTION(v)               (*((REG32_PTR_T)(VICBASE(v) + 0x20)))
#define VICSWPRIORITYMASK(v)           (*((REG32_PTR_T)(VICBASE(v) + 0x24)))
#define VICPRIORITYDAISY(v)            (*((REG32_PTR_T)(VICBASE(v) + 0x28)))
#define VICVECTADDR(v, i)              (*((REG32_PTR_T)(VICBASE(v) + 0x100 + 4 * (i))))
#define VICVECTPRIORITY(v, i)          (*((REG32_PTR_T)(VICBASE(v) + 0x200 + 4 * (i))))
#define VICADDRESS(v)         (*((CONST_VOID_PTR_PTR_T)(VICBASE(v) + 0xF00)))

#define VIC0IRQSTATUS         VICIRQSTATUS(0)
#define VIC0FIQSTATUS         VICFIQSTATUS(0)
#define VIC0RAWINTR           VICRAWINTR(0)
#define VIC0INTSELECT         VICINTSELECT(0)
#define VIC0INTENABLE         VICINTENABLE(0)
#define VIC0INTENCLEAR        VICINTENCLEAR(0)
#define VIC0SOFTINT           VICSOFTINT(0)
#define VIC0SOFTINTCLEAR      VICSOFTINTCLEAR(0)
#define VIC0PROTECTION        VICPROTECTION(0)
#define VIC0SWPRIORITYMASK    VICSWPRIORITYMASK(0)
#define VIC0PRIORITYDAISY     VICPRIORITYDAISY(0)

#define VIC0VECTADDR(i)       VICVECTADDR(0, i)
#define VIC0VECTADDR0         VIC0VECTADDR(0)
#define VIC0VECTADDR1         VIC0VECTADDR(1)
#define VIC0VECTADDR2         VIC0VECTADDR(2)
#define VIC0VECTADDR3         VIC0VECTADDR(3)
#define VIC0VECTADDR4         VIC0VECTADDR(4)
#define VIC0VECTADDR5         VIC0VECTADDR(5)
#define VIC0VECTADDR6         VIC0VECTADDR(6)
#define VIC0VECTADDR7         VIC0VECTADDR(7)
#define VIC0VECTADDR8         VIC0VECTADDR(8)
#define VIC0VECTADDR9         VIC0VECTADDR(9)
#define VIC0VECTADDR10        VIC0VECTADDR(10)
#define VIC0VECTADDR11        VIC0VECTADDR(11)
#define VIC0VECTADDR12        VIC0VECTADDR(12)
#define VIC0VECTADDR13        VIC0VECTADDR(13)
#define VIC0VECTADDR14        VIC0VECTADDR(14)
#define VIC0VECTADDR15        VIC0VECTADDR(15)
#define VIC0VECTADDR16        VIC0VECTADDR(16)
#define VIC0VECTADDR17        VIC0VECTADDR(17)
#define VIC0VECTADDR18        VIC0VECTADDR(18)
#define VIC0VECTADDR19        VIC0VECTADDR(19)
#define VIC0VECTADDR20        VIC0VECTADDR(20)
#define VIC0VECTADDR21        VIC0VECTADDR(21)
#define VIC0VECTADDR22        VIC0VECTADDR(22)
#define VIC0VECTADDR23        VIC0VECTADDR(23)
#define VIC0VECTADDR24        VIC0VECTADDR(24)
#define VIC0VECTADDR25        VIC0VECTADDR(25)
#define VIC0VECTADDR26        VIC0VECTADDR(26)
#define VIC0VECTADDR27        VIC0VECTADDR(27)
#define VIC0VECTADDR28        VIC0VECTADDR(28)
#define VIC0VECTADDR29        VIC0VECTADDR(29)
#define VIC0VECTADDR30        VIC0VECTADDR(30)
#define VIC0VECTADDR31        VIC0VECTADDR(31)

#define VIC0VECTPRIORITY(i)   VICVECTPRIORITY(0, i)
#define VIC0VECTPRIORITY0     VIC0VECTPRIORITY(0)
#define VIC0VECTPRIORITY1     VIC0VECTPRIORITY(1)
#define VIC0VECTPRIORITY2     VIC0VECTPRIORITY(2)
#define VIC0VECTPRIORITY3     VIC0VECTPRIORITY(3)
#define VIC0VECTPRIORITY4     VIC0VECTPRIORITY(4)
#define VIC0VECTPRIORITY5     VIC0VECTPRIORITY(5)
#define VIC0VECTPRIORITY6     VIC0VECTPRIORITY(6)
#define VIC0VECTPRIORITY7     VIC0VECTPRIORITY(7)
#define VIC0VECTPRIORITY8     VIC0VECTPRIORITY(8)
#define VIC0VECTPRIORITY9     VIC0VECTPRIORITY(9)
#define VIC0VECTPRIORITY10    VIC0VECTPRIORITY(10)
#define VIC0VECTPRIORITY11    VIC0VECTPRIORITY(11)
#define VIC0VECTPRIORITY12    VIC0VECTPRIORITY(12)
#define VIC0VECTPRIORITY13    VIC0VECTPRIORITY(13)
#define VIC0VECTPRIORITY14    VIC0VECTPRIORITY(14)
#define VIC0VECTPRIORITY15    VIC0VECTPRIORITY(15)
#define VIC0VECTPRIORITY16    VIC0VECTPRIORITY(16)
#define VIC0VECTPRIORITY17    VIC0VECTPRIORITY(17)
#define VIC0VECTPRIORITY18    VIC0VECTPRIORITY(18)
#define VIC0VECTPRIORITY19    VIC0VECTPRIORITY(19)
#define VIC0VECTPRIORITY20    VIC0VECTPRIORITY(20)
#define VIC0VECTPRIORITY21    VIC0VECTPRIORITY(21)
#define VIC0VECTPRIORITY22    VIC0VECTPRIORITY(22)
#define VIC0VECTPRIORITY23    VIC0VECTPRIORITY(23)
#define VIC0VECTPRIORITY24    VIC0VECTPRIORITY(24)
#define VIC0VECTPRIORITY25    VIC0VECTPRIORITY(25)
#define VIC0VECTPRIORITY26    VIC0VECTPRIORITY(26)
#define VIC0VECTPRIORITY27    VIC0VECTPRIORITY(27)
#define VIC0VECTPRIORITY28    VIC0VECTPRIORITY(28)
#define VIC0VECTPRIORITY29    VIC0VECTPRIORITY(29)
#define VIC0VECTPRIORITY30    VIC0VECTPRIORITY(30)
#define VIC0VECTPRIORITY31    VIC0VECTPRIORITY(31)

#define VIC0ADDRESS           VICADDRESS(0)

#define VIC1IRQSTATUS         VICIRQSTATUS(1)
#define VIC1FIQSTATUS         VICFIQSTATUS(1)
#define VIC1RAWINTR           VICRAWINTR(1)
#define VIC1INTSELECT         VICINTSELECT(1)
#define VIC1INTENABLE         VICINTENABLE(1)
#define VIC1INTENCLEAR        VICINTENCLEAR(1)
#define VIC1SOFTINT           VICSOFTINT(1)
#define VIC1SOFTINTCLEAR      VICSOFTINTCLEAR(1)
#define VIC1PROTECTION        VICPROTECTION(1)
#define VIC1SWPRIORITYMASK    VICSWPRIORITYMASK(1)
#define VIC1PRIORITYDAISY     VICPRIORITYDAISY(1)

#define VIC1VECTADDR(i)       VICVECTADDR(1, i)
#define VIC1VECTADDR0         VIC1VECTADDR(0)
#define VIC1VECTADDR1         VIC1VECTADDR(1)
#define VIC1VECTADDR2         VIC1VECTADDR(2)
#define VIC1VECTADDR3         VIC1VECTADDR(3)
#define VIC1VECTADDR4         VIC1VECTADDR(4)
#define VIC1VECTADDR5         VIC1VECTADDR(5)
#define VIC1VECTADDR6         VIC1VECTADDR(6)
#define VIC1VECTADDR7         VIC1VECTADDR(7)
#define VIC1VECTADDR8         VIC1VECTADDR(8)
#define VIC1VECTADDR9         VIC1VECTADDR(9)
#define VIC1VECTADDR10        VIC1VECTADDR(10)
#define VIC1VECTADDR11        VIC1VECTADDR(11)
#define VIC1VECTADDR12        VIC1VECTADDR(12)
#define VIC1VECTADDR13        VIC1VECTADDR(13)
#define VIC1VECTADDR14        VIC1VECTADDR(14)
#define VIC1VECTADDR15        VIC1VECTADDR(15)
#define VIC1VECTADDR16        VIC1VECTADDR(16)
#define VIC1VECTADDR17        VIC1VECTADDR(17)
#define VIC1VECTADDR18        VIC1VECTADDR(18)
#define VIC1VECTADDR19        VIC1VECTADDR(19)
#define VIC1VECTADDR20        VIC1VECTADDR(20)
#define VIC1VECTADDR21        VIC1VECTADDR(21)
#define VIC1VECTADDR22        VIC1VECTADDR(22)
#define VIC1VECTADDR23        VIC1VECTADDR(23)
#define VIC1VECTADDR24        VIC1VECTADDR(24)
#define VIC1VECTADDR25        VIC1VECTADDR(25)
#define VIC1VECTADDR26        VIC1VECTADDR(26)
#define VIC1VECTADDR27        VIC1VECTADDR(27)
#define VIC1VECTADDR28        VIC1VECTADDR(28)
#define VIC1VECTADDR29        VIC1VECTADDR(29)
#define VIC1VECTADDR30        VIC1VECTADDR(30)
#define VIC1VECTADDR31        VIC1VECTADDR(31)

#define VIC1VECTPRIORITY(i)   VICVECTPRIORITY(1, i)
#define VIC1VECTPRIORITY0     VIC1VECTPRIORITY(0)
#define VIC1VECTPRIORITY1     VIC1VECTPRIORITY(1)
#define VIC1VECTPRIORITY2     VIC1VECTPRIORITY(2)
#define VIC1VECTPRIORITY3     VIC1VECTPRIORITY(3)
#define VIC1VECTPRIORITY4     VIC1VECTPRIORITY(4)
#define VIC1VECTPRIORITY5     VIC1VECTPRIORITY(5)
#define VIC1VECTPRIORITY6     VIC1VECTPRIORITY(6)
#define VIC1VECTPRIORITY7     VIC1VECTPRIORITY(7)
#define VIC1VECTPRIORITY8     VIC1VECTPRIORITY(8)
#define VIC1VECTPRIORITY9     VIC1VECTPRIORITY(9)
#define VIC1VECTPRIORITY10    VIC1VECTPRIORITY(10)
#define VIC1VECTPRIORITY11    VIC1VECTPRIORITY(11)
#define VIC1VECTPRIORITY12    VIC1VECTPRIORITY(12)
#define VIC1VECTPRIORITY13    VIC1VECTPRIORITY(13)
#define VIC1VECTPRIORITY14    VIC1VECTPRIORITY(14)
#define VIC1VECTPRIORITY15    VIC1VECTPRIORITY(15)
#define VIC1VECTPRIORITY16    VIC1VECTPRIORITY(16)
#define VIC1VECTPRIORITY17    VIC1VECTPRIORITY(17)
#define VIC1VECTPRIORITY18    VIC1VECTPRIORITY(18)
#define VIC1VECTPRIORITY19    VIC1VECTPRIORITY(19)
#define VIC1VECTPRIORITY20    VIC1VECTPRIORITY(20)
#define VIC1VECTPRIORITY21    VIC1VECTPRIORITY(21)
#define VIC1VECTPRIORITY22    VIC1VECTPRIORITY(22)
#define VIC1VECTPRIORITY23    VIC1VECTPRIORITY(23)
#define VIC1VECTPRIORITY24    VIC1VECTPRIORITY(24)
#define VIC1VECTPRIORITY25    VIC1VECTPRIORITY(25)
#define VIC1VECTPRIORITY26    VIC1VECTPRIORITY(26)
#define VIC1VECTPRIORITY27    VIC1VECTPRIORITY(27)
#define VIC1VECTPRIORITY28    VIC1VECTPRIORITY(28)
#define VIC1VECTPRIORITY29    VIC1VECTPRIORITY(29)
#define VIC1VECTPRIORITY30    VIC1VECTPRIORITY(30)
#define VIC1VECTPRIORITY31    VIC1VECTPRIORITY(31)

#define VIC1ADDRESS           VICADDRESS(1)

#define VIC0EDGE0             (*((REG32_PTR_T)(VICBASE(2))))            // TBC: INTENABLE
#define VIC1EDGE0             (*((REG32_PTR_T)(VICBASE(2) + 0x04)))
#define VIC0EDGE1             (*((REG32_PTR_T)(VICBASE(2) + 0x08)))     // TBC: INTENCLEAR
#define VIC1EDGE1             (*((REG32_PTR_T)(VICBASE(2) + 0x0C)))

/////INTERRUPTS/////
// #define IRQ_UNK5        5
#define IRQ_TIMER32     7
#define IRQ_TIMER       8
#define IRQ_SPI(i)      (9+(i)) /* TBC */
#define IRQ_SPI0        IRQ_SPI(0)
#define IRQ_SPI1        IRQ_SPI(1)
#define IRQ_SPI2        IRQ_SPI(2)
#define IRQ_LCD         14
#define IRQ_DMAC(d)     (16+(d))
#define IRQ_DMAC0       IRQ_DMAC(0)
#define IRQ_DMAC1       IRQ_DMAC(1)
#define IRQ_USB_FUNC    19
#define IRQ_NAND        20  // TBC: it is so in s5l8900
#define IRQ_I2C(i)      (21+(i))
#define IRQ_I2C0        IRQ_I2C(0)
#define IRQ_I2C1        IRQ_I2C(1)
#define IRQ_WHEEL       23
#define IRQ_UART(i)     (24+(i))
#define IRQ_UART0       IRQ_UART(0)
#define IRQ_UART1       IRQ_UART(1)
#define IRQ_UART2       IRQ_UART(2)
#define IRQ_UART3       IRQ_UART(3)
#define IRQ_UART4       IRQ_UART(4)    /* obsolete/not implemented on s5l8702 ??? */
#define IRQ_ATA         29
#define IRQ_SECBOOT     36
#define IRQ_AES         39
#define IRQ_SHA         40
#define IRQ_NANDECC     43  // TBC: it is so in s5l8900
#define IRQ_MMC         44

#define IRQ_EXT0        0
#define IRQ_EXT1        1
#define IRQ_EXT2        2
#define IRQ_EXT3        3
#define IRQ_EXT4        31
#define IRQ_EXT5        32
#define IRQ_EXT6        33
#endif

/* Something related to the ATA controller, needed for HDD power up on ipod6g */
#define ATA_UNKNOWN_BASE 0x38a00000

#if CONFIG_CPU == S5L8702
#define ATA_UNKNOWN (*((REG32_PTR_T)(ATA_UNKNOWN_BASE)))
#endif

#if CONFIG_CPU==S5L8702 || CONFIG_CPU==S5L8720
/*
 * s5l8702 External (GPIO) Interrupt Controller
 *
 * 7 groups of 32 interrupts, GPIO pins are seen as 'wired'
 * to groups 6..3 in reverse order.
 * On group 3, last four bits are dissbled (GPIO 124..127).
 * All bits in groups 1 and 2 are disabled (not used).
 * On group 0, all bits are masked except bits 0 and 2:
 *  bit 0: if unmasked, EINT6 is generated when ALVTCNT
 *         reachs ALVTEND.
 *  bit 2: if unmasked, EINT6 is generated when USB cable
 *         is plugged and/or(TBC) unplugged.
 *
 * EIC_GROUP0..6 are connected to EINT6..0 of the VIC.
 */
#define EIC_N_GROUPS    7

/* get EIC group and bit for a given GPIO port */
#define EIC_GROUP(n)    (6 - ((n) >> 5))
#define EIC_INDEX(n)    ((0x18 - ((n) & 0x18)) | ((n) & 0x7))

/* SoC EINTs uses these 'gpio' numbers */
#define GPIO_EINT_USB       0xd8
#define GPIO_EINT_ALIVE     0xda

/* probably a part of the system controller */
#if CONFIG_CPU == S5L8702
#define EIC_BASE 0x39a00000
#elif CONFIG_CPU == S5L8720
#define EIC_BASE 0x39700000
#endif

#define EIC_INTLEVEL(g) (*((REG32_PTR_T)(EIC_BASE + 0x80 + 4*(g))))
#define EIC_INTSTAT(g)  (*((REG32_PTR_T)(EIC_BASE + 0xA0 + 4*(g))))
#define EIC_INTEN(g)    (*((REG32_PTR_T)(EIC_BASE + 0xC0 + 4*(g))))
#define EIC_INTTYPE(g)  (*((REG32_PTR_T)(EIC_BASE + 0xE0 + 4*(g))))

#define EIC_INTLEVEL_LOW    0
#define EIC_INTLEVEL_HIGH   1

#define EIC_INTTYPE_EDGE    0
#define EIC_INTTYPE_LEVEL   1
#endif

#if CONFIG_CPU == S5L8702
/*
 * This is very preliminary work in progress, ATM this region is called
 * system 'alive' because it seems there are similiarities when mixing
 * concepts from:
 *  - s3c2440 datasheet (figure 7-12, Sleep mode) and
 *  - ARM-DDI-0287B (2.1.8 System Mode Control, Sleep an Doze modes)
 *
 * Known components:
 * - independent clocking
 * - 32-bit timer
 * - level/edge configurable interrupt controller
 *
 *
 *        OSCSEL
 *          |\    CKSEL
 *  OSC0 -->| |    |\
 *          | |--->| |     _________             ___________
 *  OSC1 -->| |    | |    |         | SClk      |           |
 *          |/     | |--->| 1/CKDIV |---------->| 1/ALVTDIV |--> Timer
 *                 | |    |_________|       |   |___________|    counter
 *  PClk --------->| |                      |    ___________
 *                 |/                       |   |           |
 *                                          +-->| 1/UNKDIV  |--> Unknown
 *                                              |___________|
 */
#define SYSALV_BASE 0x39a00000

#define ALVCON          (*((REG32_PTR_T)(SYSALV_BASE)))
#define ALVUNK4         (*((REG32_PTR_T)(SYSALV_BASE + 0x4)))
#define ALVUNK100       (*((REG32_PTR_T)(SYSALV_BASE + 0x100)))
#define ALVUNK104       (*((REG32_PTR_T)(SYSALV_BASE + 0x104)))


/*
 * System Alive control register
 */
#define ALVCON_CKSEL_BIT    (1 << 25)  /* 0 -> S5L8702_OSCx, 1 -> PClk */
#define ALVCON_CKDIVEN_BIT  (1 << 24)  /* 0 -> CK divider Off, 1 -> On */
#define ALVCON_CKDIV_POS    20         /* real_val = reg_val+1 */
#define ALVCON_CKDIV_MSK    0xf

/* UNKDIV: real_val = reg_val+1 (TBC), valid reg_val=0,1,2 */
/* experimental: for registers in this region, read/write speed is
 * scaled by this divider, so probably it is related with internal
 * 'working' frequency.
 */
#define ALVCON_UNKDIV_POS   16
#define ALVCON_UNKDIV_MSK   0x3

/* bits[14:1] are UNKNOWN */

#define ALVCON_OSCSEL_BIT   (1 << 0)   /* 0 -> OSC0, 1 -> OSC1 */


/*
 * System Alive timer
 *
 * ALVCOM_RUN_BIT starts/stops count on ALVTCNT, counter frequency
 * is SClk / ALVTDIV. When count reachs ALVTEND then ALVTSTAT[0]
 * and ALVUNK4[0] are set, optionally an interrupt is generated (see
 * GPIO_EINT_ALIVE). Writing 1 to ALVTCOM_RST_BIT clears ALVSTAT[0]
 * and ALVUNK4[0], and initializes ALVTCNT to zero.
 */
#define ALVTCOM         (*((REG32_PTR_T)(SYSALV_BASE + 0x6c)))
#define ALVTCOM_RUN_BIT     (1 << 0)  /* 0 -> Stop, 1 -> Start */
#define ALVTCOM_RST_BIT     (1 << 1)  /* 1 -> Reset */

#define ALVTEND         (*((REG32_PTR_T)(SYSALV_BASE + 0x70)))
#define ALVTDIV         (*((REG32_PTR_T)(SYSALV_BASE + 0x74)))

#define ALVTCNT         (*((REG32_PTR_T)(SYSALV_BASE + 0x78)))
#define ALVTSTAT        (*((REG32_PTR_T)(SYSALV_BASE + 0x7c)))

#endif /* CONFIG_CPU == S5L8702 */

#endif /* __S5L87XX_H__ */
