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

#ifndef __S5L8700_H__
#define __S5L8700_H__

#ifndef ASM
#include <stdint.h>
#endif

#define REG16_PTR_T volatile uint16_t *
#define REG32_PTR_T volatile uint32_t *

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define CACHEALIGN_BITS (4) /* 2^4 = 16 bytes */
#elif CONFIG_CPU==S5L8702
#define CACHEALIGN_BITS (5) /* 2^5 = 32 bytes */
#endif

#if CONFIG_CPU==S5L8702
#define DRAM_ORIG 0x08000000
#define IRAM_ORIG 0x22000000

#define DRAM_SIZE (MEMORYSIZE * 0x100000)
#define IRAM_SIZE 0x40000

#define TTB_SIZE        0x4000
#define TTB_BASE_ADDR   (DRAM_ORIG + DRAM_SIZE - TTB_SIZE)

#define IRAM0_ORIG      0x22000000
#define IRAM0_SIZE      0x20000
#define IRAM1_ORIG      0x22020000
#define IRAM1_SIZE      0x20000
#endif

/* 04. CALMADM2E */

/* Following registers are mapped on IO Area in data memory area of Calm. */
#define CONFIG0                 (*(REG16_PTR_T)(0x3F000000))     /* configuration/control register 0 */
#define CONFIG1                 (*(REG16_PTR_T)(0x3F000002))     /* configuration/control register 1*/
#define COMMUN                  (*(REG16_PTR_T)(0x3F000004))     /* Communication Control Register */
#define DDATA0                  (*(REG16_PTR_T)(0x3F000006))     /* Communication data from host to ADM */
#define DDATA1                  (*(REG16_PTR_T)(0x3F000008))     /* Communication data from host to ADM */
#define DDATA2                  (*(REG16_PTR_T)(0x3F00000A))     /* Communication data from host to ADM */
#define DDATA3                  (*(REG16_PTR_T)(0x3F00000C))     /* Communication data from host to ADM */
#define DDATA4                  (*(REG16_PTR_T)(0x3F00000E))     /* Communication data from host to ADM */
#define DDATA5                  (*(REG16_PTR_T)(0x3F000010))     /* Communication data from host to ADM */
#define DDATA6                  (*(REG16_PTR_T)(0x3F000012))     /* Communication data from host to ADM */
#define DDATA7                  (*(REG16_PTR_T)(0x3F000014))     /* Communication data from host to ADM */
#define UDATA0                  (*(REG16_PTR_T)(0x3F000016))     /* Communication data from ADM to host */
#define UDATA1                  (*(REG16_PTR_T)(0x3F000018))     /* Communication data from ADM to host */
#define UDATA2                  (*(REG16_PTR_T)(0x3F00001A))     /* Communication data from ADM to host */
#define UDATA3                  (*(REG16_PTR_T)(0x3F00001C))     /* Communication data from ADM to host */
#define UDATA4                  (*(REG16_PTR_T)(0x3F00001E))     /* Communication data from ADM to host */
#define UDATA5                  (*(REG16_PTR_T)(0x3F000020))     /* Communication data from ADM to host */
#define UDATA6                  (*(REG16_PTR_T)(0x3F000022))     /* Communication data from ADM to host */
#define UDATA7                  (*(REG16_PTR_T)(0x3F000024))     /* Communication data from ADM to host */
#define IBASE_H                 (*(REG16_PTR_T)(0x3F000026))     /* Higher half of start address for ADM instruction area */
#define IBASE_L                 (*(REG16_PTR_T)(0x3F000028))     /* Lower half of start address for ADM instruction area */
#define DBASE_H                 (*(REG16_PTR_T)(0x3F00002A))     /* Higher half of start address for CalmRISC data area */
#define DBASE_L                 (*(REG16_PTR_T)(0x3F00002C))     /* Lower half of start address for CalmRISC data area */
#define XBASE_H                 (*(REG16_PTR_T)(0x3F00002E))     /* Higher half of start address for Mac X area */
#define XBASE_L                 (*(REG16_PTR_T)(0x3F000030))     /* Lower half of start address for Mac X area */
#define YBASE_H                 (*(REG16_PTR_T)(0x3F000032))     /* Higher half of start address for Mac Y area */
#define YBASE_L                 (*(REG16_PTR_T)(0x3F000034))     /* Lower half of start address for Mac Y area */
#define S0BASE_H                (*(REG16_PTR_T)(0x3F000036))     /* Higher half of start address for sequential buffer 0 area */
#define S0BASE_L                (*(REG16_PTR_T)(0x3F000038))     /* Lower half of start address for sequential buffer 0 area */
#define S1BASE_H                (*(REG16_PTR_T)(0x3F00003A))     /* Higher half of start address for sequential buffer 1 area */
#define S1BASE_L                (*(REG16_PTR_T)(0x3F00003C))     /* Lower half of start address for sequential buffer 1 area */
#define CACHECON                (*(REG16_PTR_T)(0x3F00003E))     /* Cache Control Register */
#define CACHESTAT               (*(REG16_PTR_T)(0x3F000040))     /* Cache status register */
#define SBFCON                  (*(REG16_PTR_T)(0x3F000042))     /* Sequential Buffer Control Register */
#define SBFSTAT                 (*(REG16_PTR_T)(0x3F000044))     /* Sequential Buffer Status Register */
#define SBL0OFF_H               (*(REG16_PTR_T)(0x3F000046))     /* Higher bits of Offset register of sequential block 0 area */
#define SBL0OFF_L               (*(REG16_PTR_T)(0x3F000048))     /* Lower bits of Offset register of sequential block 0 area */
#define SBL1OFF_H               (*(REG16_PTR_T)(0x3F00004A))     /* Higher bits of Offset register of sequential block 1 area */
#define SBL1OFF_L               (*(REG16_PTR_T)(0x3F00004C))     /* Lower bits of Offset register of sequential block 1 area */
#define SBL0BEGIN_H             (*(REG16_PTR_T)(0x3F00004E))     /* Higher bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL0BEGIN_L             (*(REG16_PTR_T)(0x3F000050))     /* Lower bits of Begin Offset of sequential block 0 area in ring mode */
#define SBL1BEGIN_H             (*(REG16_PTR_T)(0x3F000052))     /* Higher bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL1BEGIN_L             (*(REG16_PTR_T)(0x3F000054))     /* Lower bits of Begin Offset of sequential block 1 area in ring mode */
#define SBL0END_H               (*(REG16_PTR_T)(0x3F000056))     /* Lower bits of End Offset of sequential block 0 area in ring mode */
#define SBL0END_L               (*(REG16_PTR_T)(0x3F000058))     /* Higher bits of End Offset of sequential block 0 area in ring mode */
#define SBL1END_H               (*(REG16_PTR_T)(0x3F00005A))     /* Lower bits of End Offset of sequential block 1 area in ring mode */
#define SBL1END_L               (*(REG16_PTR_T)(0x3F00005C))     /* Higher bits of End Offset of sequential block 1 area in ring mode */

/* Following registers are components of SFRS of the target system */
#define ADM_CONFIG              (*(REG32_PTR_T)(0x39000000))     /* Configuration/Control Register */
#define ADM_COMMUN              (*(REG32_PTR_T)(0x39000004))     /* Communication Control Register */
#define ADM_DDATA0              (*(REG32_PTR_T)(0x39000010))     /* Communication data from host to ADM */
#define ADM_DDATA1              (*(REG32_PTR_T)(0x39000014))     /* Communication data from host to ADM */
#define ADM_DDATA2              (*(REG32_PTR_T)(0x39000018))     /* Communication data from host to ADM */
#define ADM_DDATA3              (*(REG32_PTR_T)(0x3900001C))     /* Communication data from host to ADM */
#define ADM_DDATA4              (*(REG32_PTR_T)(0x39000020))     /* Communication data from host to ADM */
#define ADM_DDATA5              (*(REG32_PTR_T)(0x39000024))     /* Communication data from host to ADM */
#define ADM_DDATA6              (*(REG32_PTR_T)(0x39000028))     /* Communication data from host to ADM */
#define ADM_DDATA7              (*(REG32_PTR_T)(0x3900002C))     /* Communication data from host to ADM */
#define ADM_UDATA0              (*(REG32_PTR_T)(0x39000030))     /* Communication data from ADM to host */
#define ADM_UDATA1              (*(REG32_PTR_T)(0x39000034))     /* Communication data from ADM to host */
#define ADM_UDATA2              (*(REG32_PTR_T)(0x39000038))     /* Communication data from ADM to host */
#define ADM_UDATA3              (*(REG32_PTR_T)(0x3900003C))     /* Communication data from ADM to host */
#define ADM_UDATA4              (*(REG32_PTR_T)(0x39000040))     /* Communication data from ADM to host */
#define ADM_UDATA5              (*(REG32_PTR_T)(0x39000044))     /* Communication data from ADM to host */
#define ADM_UDATA6              (*(REG32_PTR_T)(0x39000048))     /* Communication data from ADM to host */
#define ADM_UDATA7              (*(REG32_PTR_T)(0x3900004C))     /* Communication data from ADM to host */
#define ADM_IBASE               (*(REG32_PTR_T)(0x39000050))     /* Start Address for ADM Instruction Area */
#define ADM_DBASE               (*(REG32_PTR_T)(0x39000054))     /* Start Address for CalmRISC Data Area */
#define ADM_XBASE               (*(REG32_PTR_T)(0x39000058))     /* Start Address for Mac X Area */
#define ADM_YBASE               (*(REG32_PTR_T)(0x3900005C))     /* Start Address for Mac Y Area */
#define ADM_S0BASE              (*(REG32_PTR_T)(0x39000060))     /* Start Address for Sequential Block 0 Area */
#define ADM_S1BASE              (*(REG32_PTR_T)(0x39000064))     /* Start Address for Sequential Block 1 Area */

/* 05. CLOCK & POWER MANAGEMENT */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define CLKCON                  (*(REG32_PTR_T)(0x3C500000))     /* Clock control register */
#define PLL0PMS                 (*(REG32_PTR_T)(0x3C500004))     /* PLL PMS value register */
#define PLL1PMS                 (*(REG32_PTR_T)(0x3C500008))     /* PLL PMS value register */
#define PLL2PMS                 (*(REG32_PTR_T)(0x3C50000C))     /* PLL PMS value register  - S5L8701 only? */
#define CLKCON3                 (*(REG32_PTR_T)(0x3C500010))     /* Clock control register 3 */
#define PLL0LCNT                (*(REG32_PTR_T)(0x3C500014))     /* PLL0 lock count register */
#define PLL1LCNT                (*(REG32_PTR_T)(0x3C500018))     /* PLL1 lock count register */
#define PLL2LCNT                (*(REG32_PTR_T)(0x3C50001C))     /* PLL2 lock count register - S5L8701 only? */
#define PLLLOCK                 (*(REG32_PTR_T)(0x3C500020))     /* PLL lock status register */
#define PLLCON                  (*(REG32_PTR_T)(0x3C500024))     /* PLL control register */
#define PWRCON                  (*(REG32_PTR_T)(0x3C500028))     /* Clock power control register */
#define PWRMODE                 (*(REG32_PTR_T)(0x3C50002C))     /* Power mode control register */
#define SWRCON                  (*(REG32_PTR_T)(0x3C500030))     /* Software reset control register */
#define RSTSR                   (*(REG32_PTR_T)(0x3C500034))     /* Reset status register */
#define DSPCLKMD                (*(REG32_PTR_T)(0x3C500038))     /* DSP clock mode register */
#define CLKCON2                 (*(REG32_PTR_T)(0x3C50003C))     /* Clock control register 2 */
#define PWRCONEXT               (*(REG32_PTR_T)(0x3C500040))     /* Clock power control register 2 */
#elif CONFIG_CPU==S5L8702
#define CLKCON0      (*((REG32_PTR_T)(0x3C500000)))
#define CLKCON1      (*((REG32_PTR_T)(0x3C500004)))
#define CLKCON2      (*((REG32_PTR_T)(0x3C500008)))
#define CLKCON3      (*((REG32_PTR_T)(0x3C50000C)))
#define CLKCON4      (*((REG32_PTR_T)(0x3C500010)))
#define CLKCON5      (*((REG32_PTR_T)(0x3C500014)))
#define PLL0PMS      (*((REG32_PTR_T)(0x3C500020)))
#define PLL1PMS      (*((REG32_PTR_T)(0x3C500024)))
#define PLL2PMS      (*((REG32_PTR_T)(0x3C500028)))
#define PLL0LCNT     (*((REG32_PTR_T)(0x3C500030)))
#define PLL1LCNT     (*((REG32_PTR_T)(0x3C500034)))
#define PLL2LCNT     (*((REG32_PTR_T)(0x3C500038)))
#define PLLLOCK      (*((REG32_PTR_T)(0x3C500040)))
#define PLLMODE      (*((REG32_PTR_T)(0x3C500044)))
#define PWRCON(i)    (*((REG32_PTR_T)(0x3C500000 \
                                           + ((i) == 4 ? 0x6C : \
                                             ((i) == 3 ? 0x68 : \
                                             ((i) == 2 ? 0x58 : \
                                             ((i) == 1 ? 0x4C : \
                                                         0x48)))))))
/* SW Reset Control Register */
#define SWRCON      (*((REG32_PTR_T)(0x3C500050)))
/* Reset Status Register */
#define RSTSR       (*((REG32_PTR_T)(0x3C500054)))
#define RSTSR_WDR_BIT   (1 << 2)
#define RSTSR_SWR_BIT   (1 << 1)
#define RSTSR_HWR_BIT   (1 << 0)
#endif

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
#define CLOCKGATE_SDCI      9
#define CLOCKGATE_AES       10
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
#endif

/* 06. INTERRUPT CONTROLLER UNIT */
#define SRCPND                  (*(REG32_PTR_T)(0x39C00000))     /* Indicates the interrupt request status. */
#define INTMOD                  (*(REG32_PTR_T)(0x39C00004))     /* Interrupt mode register. */
#define INTMSK                  (*(REG32_PTR_T)(0x39C00008))     /* Determines which interrupt source is masked. The */

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

#define PRIORITY                (*(REG32_PTR_T)(0x39C0000C))     /* IRQ priority control register */
#define INTPND                  (*(REG32_PTR_T)(0x39C00010))     /* Indicates the interrupt request status. */
#define INTOFFSET               (*(REG32_PTR_T)(0x39C00014))     /* Indicates the IRQ interrupt request source */

#if CONFIG_CPU==S5L8700
#define EINTPOL                 (*(REG32_PTR_T)(0x39C00018))     /* Indicates external interrupt polarity */
#define EINTPEND                (*(REG32_PTR_T)(0x39C0001C))     /* Indicates whether external interrupts are pending. */
#define EINTMSK                 (*(REG32_PTR_T)(0x39C00020))     /* Indicates whether external interrupts are masked */
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
#define GPIOIC_INTLEVEL(g)      (*(REG32_PTR_T)(0x39C00018 + 4*(g)))
#define GPIOIC_INTSTAT(g)       (*(REG32_PTR_T)(0x39C00028 + 4*(g)))
#define GPIOIC_INTEN(g)         (*(REG32_PTR_T)(0x39C00038 + 4*(g)))
#define GPIOIC_INTTYPE(g)       (*(REG32_PTR_T)(0x39C00048 + 4*(g)))
#endif

/* 07. MEMORY INTERFACE UNIT (MIU) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
/* SDRAM */
#define MIUCON                  (*(REG32_PTR_T)(0x38200000))     /* External Memory configuration register */
#define MIUCOM                  (*(REG32_PTR_T)(0x38200004))     /* Command and status register */
#define MIUAREF                 (*(REG32_PTR_T)(0x38200008))     /* Auto-refresh control register */
#define MIUMRS                  (*(REG32_PTR_T)(0x3820000C))     /* SDRAM Mode Register Set Value Register */
#define MIUSDPARA               (*(REG32_PTR_T)(0x38200010))     /* SDRAM parameter register */

/* DDR */
#define MEMCONF                 (*(REG32_PTR_T)(0x38200020))     /* External Memory configuration register */
#define USRCMD                  (*(REG32_PTR_T)(0x38200024))     /* Command and Status register */
#define AREF                    (*(REG32_PTR_T)(0x38200028))     /* Auto-refresh control register */
#define MRS                     (*(REG32_PTR_T)(0x3820002C))     /* DRAM mode register set value register */
#define DPARAM                  (*(REG32_PTR_T)(0x38200030))     /* DRAM parameter register (Unit of ‘tXXX’ : tCK */
#define SMEMCONF                (*(REG32_PTR_T)(0x38200034))     /* Static memory mode register set value register */
#define MIUS01PARA              (*(REG32_PTR_T)(0x38200038))     /* SRAM0, SRAM1 static memory parameter register (In S5L8700, SRAM0 is Nor Flash) */
#define MIUS23PARA              (*(REG32_PTR_T)(0x3820003C))     /* SRAM2 and SRAM3 static memory parameter register */

#define MIUORG                  (*(REG32_PTR_T)(0x38200040))     /* SDR/DDR selection */
#define MIUDLYDQS               (*(REG32_PTR_T)(0x38200044))     /* DQS/DQS-rst delay parameter */
#define MIUDLYCLK               (*(REG32_PTR_T)(0x38200048))     /* SDR/DDR Clock delay parameter */
#define MIU_DSS_SEL_B           (*(REG32_PTR_T)(0x3820004C))     /* SSTL2 Drive Strength parameter for Bi-direction signal */
#define MIU_DSS_SEL_O           (*(REG32_PTR_T)(0x38200050))     /* SSTL2 Drive Strength parameter for Output signal */
#define MIU_DSS_SEL_C           (*(REG32_PTR_T)(0x38200054))     /* SSTL2 Drive Strength parameter for Clock signal */
#define PAD_DSS_SEL_NOR         (*(REG32_PTR_T)(0x38200058))     /* Wide range I/O Drive Strength parameter for NOR interface */
#define PAD_DSS_SEL_ATA         (*(REG32_PTR_T)(0x3820005C))     /* Wide range I/O Drive Strength parameter for ATA interface */
#define SSTL2_PAD_ON            (*(REG32_PTR_T)(0x38200060))     /* SSTL2 pad ON/OFF select */
#elif CONFIG_CPU==S5L8702
#define MIU_BASE        (0x38100000)
#define MIU_REG(off)    (*((REG32_PTR_T)(MIU_BASE + (off))))
/* following registers are similar to s5l8700x */
#define MIUCON          (*((REG32_PTR_T)(0x38100000)))
#define MIUCOM          (*((REG32_PTR_T)(0x38100004)))
#define MIUAREF         (*((REG32_PTR_T)(0x38100008)))
#define MIUMRS          (*((REG32_PTR_T)(0x3810000C)))
#define MIUSDPARA       (*((REG32_PTR_T)(0x38100010)))
#endif

/* 08. IODMA CONTROLLER */
#define DMABASE0                (*(REG32_PTR_T)(0x38400000))     /* Base address register for channel 0 */
#define DMACON0                 (*(REG32_PTR_T)(0x38400004))     /* Configuration register for channel 0 */
#define DMATCNT0                (*(REG32_PTR_T)(0x38400008))     /* Transfer count register for channel 0 */
#define DMACADDR0               (*(REG32_PTR_T)(0x3840000C))     /* Current memory address register for channel 0 */
#define DMACTCNT0               (*(REG32_PTR_T)(0x38400010))     /* Current transfer count register for channel 0 */
#define DMACOM0                 (*(REG32_PTR_T)(0x38400014))     /* Channel 0 command register */
#define DMANOFF0                (*(REG32_PTR_T)(0x38400018))     /* Channel 0 offset2 register */
#define DMABASE1                (*(REG32_PTR_T)(0x38400020))     /* Base address register for channel 1 */
#define DMACON1                 (*(REG32_PTR_T)(0x38400024))     /* Configuration register for channel 1 */
#define DMATCNT1                (*(REG32_PTR_T)(0x38400028))     /* Transfer count register for channel 1 */
#define DMACADDR1               (*(REG32_PTR_T)(0x3840002C))     /* Current memory address register for channel 1 */
#define DMACTCNT1               (*(REG32_PTR_T)(0x38400030))     /* Current transfer count register for channel 1 */
#define DMACOM1                 (*(REG32_PTR_T)(0x38400034))     /* Channel 1 command register */
#define DMABASE2                (*(REG32_PTR_T)(0x38400040))     /* Base address register for channel 2 */
#define DMACON2                 (*(REG32_PTR_T)(0x38400044))     /* Configuration register for channel 2 */
#define DMATCNT2                (*(REG32_PTR_T)(0x38400048))     /* Transfer count register for channel 2 */
#define DMACADDR2               (*(REG32_PTR_T)(0x3840004C))     /* Current memory address register for channel 2 */
#define DMACTCNT2               (*(REG32_PTR_T)(0x38400050))     /* Current transfer count register for channel 2 */
#define DMACOM2                 (*(REG32_PTR_T)(0x38400054))     /* Channel 2 command register */
#define DMABASE3                (*(REG32_PTR_T)(0x38400060))     /* Base address register for channel 3 */
#define DMACON3                 (*(REG32_PTR_T)(0x38400064))     /* Configuration register for channel 3 */
#define DMATCNT3                (*(REG32_PTR_T)(0x38400068))     /* Transfer count register for channel 3 */
#define DMACADDR3               (*(REG32_PTR_T)(0x3840006C))     /* Current memory address register for channel 3 */
#define DMACTCNT3               (*(REG32_PTR_T)(0x38400070))     /* Current transfer count register for channel 3 */
#define DMACOM3                 (*(REG32_PTR_T)(0x38400074))     /* Channel 3 command register */

#if CONFIG_CPU==S5L8700
#define DMAALLST                (*(REG32_PTR_T)(0x38400100))     /* All channel status register */
#elif CONFIG_CPU==S5L8701
#define DMABASE4                (*(REG32_PTR_T)(0x38400080))     /* Base address register for channel 4 */
#define DMACON4                 (*(REG32_PTR_T)(0x38400084))     /* Configuration register for channel 4 */
#define DMATCNT4                (*(REG32_PTR_T)(0x38400088))     /* Transfer count register for channel 4 */
#define DMACADDR4               (*(REG32_PTR_T)(0x3840008C))     /* Current memory address register for channel 4 */
#define DMACTCNT4               (*(REG32_PTR_T)(0x38400090))     /* Current transfer count register for channel 4 */
#define DMACOM4                 (*(REG32_PTR_T)(0x38400094))     /* Channel 4 command register */
#define DMABASE5                (*(REG32_PTR_T)(0x384000A0))     /* Base address register for channel 5 */
#define DMACON5                 (*(REG32_PTR_T)(0x384000A4))     /* Configuration register for channel 5 */
#define DMATCNT5                (*(REG32_PTR_T)(0x384000A8))     /* Transfer count register for channel 5 */
#define DMACADDR5               (*(REG32_PTR_T)(0x384000AC))     /* Current memory address register for channel 5 */
#define DMACTCNT5               (*(REG32_PTR_T)(0x384000B0))     /* Current transfer count register for channel 5 */
#define DMACOM5                 (*(REG32_PTR_T)(0x384000B4))     /* Channel 5 command register */
#define DMABASE6                (*(REG32_PTR_T)(0x384000C0))     /* Base address register for channel 6 */
#define DMACON6                 (*(REG32_PTR_T)(0x384000C4))     /* Configuration register for channel 6 */
#define DMATCNT6                (*(REG32_PTR_T)(0x384000C8))     /* Transfer count register for channel 6 */
#define DMACADDR6               (*(REG32_PTR_T)(0x384000CC))     /* Current memory address register for channel 6 */
#define DMACTCNT6               (*(REG32_PTR_T)(0x384000D0))     /* Current transfer count register for channel 6 */
#define DMACOM6                 (*(REG32_PTR_T)(0x384000D4))     /* Channel 6 command register */
#define DMABASE7                (*(REG32_PTR_T)(0x384000E0))     /* Base address register for channel 7 */
#define DMACON7                 (*(REG32_PTR_T)(0x384000E4))     /* Configuration register for channel 7 */
#define DMATCNT7                (*(REG32_PTR_T)(0x384000E8))     /* Transfer count register for channel 7 */
#define DMACADDR7               (*(REG32_PTR_T)(0x384000EC))     /* Current memory address register for channel 7 */
#define DMACTCNT7               (*(REG32_PTR_T)(0x384000F0))     /* Current transfer count register for channel 7 */
#define DMACOM7                 (*(REG32_PTR_T)(0x384000F4))     /* Channel 7 command register */
#define DMABASE8                (*(REG32_PTR_T)(0x38400100))     /* Base address register for channel 8 */
#define DMACON8                 (*(REG32_PTR_T)(0x38400104))     /* Configuration register for channel 8 */
#define DMATCNT8                (*(REG32_PTR_T)(0x38400108))     /* Transfer count register for channel 8 */
#define DMACADDR8               (*(REG32_PTR_T)(0x3840010C))     /* Current memory address register for channel 8 */
#define DMACTCNT8               (*(REG32_PTR_T)(0x38400110))     /* Current transfer count register for channel 8 */
#define DMACOM8                 (*(REG32_PTR_T)(0x38400114))     /* Channel 8 command register */
#define DMAALLST                (*(REG32_PTR_T)(0x38400180))     /* All channel status register */
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
#define RTCCON                  (*(REG32_PTR_T)(0x3D200000))     /* RTC Control Register */
#define RTCRST                  (*(REG32_PTR_T)(0x3D200004))     /* RTC Round Reset Register */
#define RTCALM                  (*(REG32_PTR_T)(0x3D200008))     /* RTC Alarm Control Register */
#define ALMSEC                  (*(REG32_PTR_T)(0x3D20000C))     /* Alarm Second Data Register */
#define ALMMIN                  (*(REG32_PTR_T)(0x3D200010))     /* Alarm Minute Data Register */
#define ALMHOUR                 (*(REG32_PTR_T)(0x3D200014))     /* Alarm Hour Data Register */
#define ALMDATE                 (*(REG32_PTR_T)(0x3D200018))     /* Alarm Date Data Register */
#define ALMDAY                  (*(REG32_PTR_T)(0x3D20001C))     /* Alarm Day of Week Data Register */
#define ALMMON                  (*(REG32_PTR_T)(0x3D200020))     /* Alarm Month Data Register */
#define ALMYEAR                 (*(REG32_PTR_T)(0x3D200024))     /* Alarm Year Data Register */
#define BCDSEC                  (*(REG32_PTR_T)(0x3D200028))     /* BCD Second Register */
#define BCDMIN                  (*(REG32_PTR_T)(0x3D20002C))     /* BCD Minute Register */
#define BCDHOUR                 (*(REG32_PTR_T)(0x3D200030))     /* BCD Hour Register */
#define BCDDATE                 (*(REG32_PTR_T)(0x3D200034))     /* BCD Date Register */
#define BCDDAY                  (*(REG32_PTR_T)(0x3D200038))     /* BCD Day of Week Register */
#define BCDMON                  (*(REG32_PTR_T)(0x3D20003C))     /* BCD Month Register */
#define BCDYEAR                 (*(REG32_PTR_T)(0x3D200040))     /* BCD Year Register */
#define RTCIM                   (*(REG32_PTR_T)(0x3D200044))     /* RTC Interrupt Mode Register */
#define RTCPEND                 (*(REG32_PTR_T)(0x3D200048))     /* RTC Interrupt Pending Register */

/* 09. WATCHDOG TIMER*/
#define WDTCON                  (*(REG32_PTR_T)(0x3C800000))     /* Control Register */
#define WDTCNT                  (*(REG32_PTR_T)(0x3C800004))     /* 11-bits internal counter */

/* 11. 16 BIT TIMER */
#define TACON                   (*(REG32_PTR_T)(0x3C700000))     /* Control Register for timer A */
#define TACMD                   (*(REG32_PTR_T)(0x3C700004))     /* Command Register for timer A */
#define TADATA0                 (*(REG32_PTR_T)(0x3C700008))     /* Data0 Register */
#define TADATA1                 (*(REG32_PTR_T)(0x3C70000C))     /* Data1 Register */
#define TAPRE                   (*(REG32_PTR_T)(0x3C700010))     /* Pre-scale register */
#define TACNT                   (*(REG32_PTR_T)(0x3C700014))     /* Counter register */
#define TBCON                   (*(REG32_PTR_T)(0x3C700020))     /* Control Register for timer B */
#define TBCMD                   (*(REG32_PTR_T)(0x3C700024))     /* Command Register for timer B */
#define TBDATA0                 (*(REG32_PTR_T)(0x3C700028))     /* Data0 Register */
#define TBDATA1                 (*(REG32_PTR_T)(0x3C70002C))     /* Data1 Register */
#define TBPRE                   (*(REG32_PTR_T)(0x3C700030))     /* Pre-scale register */
#define TBCNT                   (*(REG32_PTR_T)(0x3C700034))     /* Counter register */
#define TCCON                   (*(REG32_PTR_T)(0x3C700040))     /* Control Register for timer C */
#define TCCMD                   (*(REG32_PTR_T)(0x3C700044))     /* Command Register for timer C */
#define TCDATA0                 (*(REG32_PTR_T)(0x3C700048))     /* Data0 Register */
#define TCDATA1                 (*(REG32_PTR_T)(0x3C70004C))     /* Data1 Register */
#define TCPRE                   (*(REG32_PTR_T)(0x3C700050))     /* Pre-scale register */
#define TCCNT                   (*(REG32_PTR_T)(0x3C700054))     /* Counter register */
#define TDCON                   (*(REG32_PTR_T)(0x3C700060))     /* Control Register for timer D */
#define TDCMD                   (*(REG32_PTR_T)(0x3C700064))     /* Command Register for timer D */
#define TDDATA0                 (*(REG32_PTR_T)(0x3C700068))     /* Data0 Register */
#define TDDATA1                 (*(REG32_PTR_T)(0x3C70006C))     /* Data1 Register */
#define TDPRE                   (*(REG32_PTR_T)(0x3C700070))     /* Pre-scale register */
#define TDCNT                   (*(REG32_PTR_T)(0x3C700074))     /* Counter register */

#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define TIMER_FREQ  (1843200 * 4 * 26 / 1 / 4) /* 47923200 Hz */

#define FIVE_USEC_TIMER         (((uint64_t)(*(REG32_PTR_T)(0x3C700080)) << 32) \
                                | (*(REG32_PTR_T)(0x3C700084)))  /* 64bit 5usec timer */
#define USEC_TIMER              (FIVE_USEC_TIMER * 5) /* usecs */
#elif CONFIG_CPU==S5L8702
/* 16/32-bit timers:
 *
 * - Timers A..D: 16-bit counter, very similar to 16-bit timers described
 *   in S5L8700 DS, it seems that the timers C and D are disabled or not
 *   implemented.
 *
 * - Timers E..H: 32-bit counter, they are like 16-bit timers, but the
 *   interrupt status for all 32-bit timers is located in TSTAT register.
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
 *   - Ext. Clock 0: not connected or disabled
 *   - Ext. Clock 1: 32768 Hz, external OSC1?, PMU?
 *   - ECLK: 12 MHz, external OSC0?
 */
#define TIMER_FREQ  12000000    /* ECLK */

#define TECON        (*((REG32_PTR_T)(0x3C7000A0)))
#define TECMD        (*((REG32_PTR_T)(0x3C7000A4)))
#define TEDATA0      (*((REG32_PTR_T)(0x3C7000A8)))
#define TEDATA1      (*((REG32_PTR_T)(0x3C7000AC)))
#define TEPRE        (*((REG32_PTR_T)(0x3C7000B0)))
#define TECNT        (*((REG32_PTR_T)(0x3C7000B4)))
#define TFCON        (*((REG32_PTR_T)(0x3C7000C0)))
#define TFCMD        (*((REG32_PTR_T)(0x3C7000C4)))
#define TFDATA0      (*((REG32_PTR_T)(0x3C7000C8)))
#define TFDATA1      (*((REG32_PTR_T)(0x3C7000CC)))
#define TFPRE        (*((REG32_PTR_T)(0x3C7000D0)))
#define TFCNT        (*((REG32_PTR_T)(0x3C7000D4)))
#define TGCON        (*((REG32_PTR_T)(0x3C7000E0)))
#define TGCMD        (*((REG32_PTR_T)(0x3C7000E4)))
#define TGDATA0      (*((REG32_PTR_T)(0x3C7000E8)))
#define TGDATA1      (*((REG32_PTR_T)(0x3C7000EC)))
#define TGPRE        (*((REG32_PTR_T)(0x3C7000F0)))
#define TGCNT        (*((REG32_PTR_T)(0x3C7000F4)))
#define THCON        (*((REG32_PTR_T)(0x3C700100)))
#define THCMD        (*((REG32_PTR_T)(0x3C700104)))
#define THDATA0      (*((REG32_PTR_T)(0x3C700108)))
#define THDATA1      (*((REG32_PTR_T)(0x3C70010C)))
#define THPRE        (*((REG32_PTR_T)(0x3C700110)))
#define THCNT        (*((REG32_PTR_T)(0x3C700114)))
#define TSTAT        (*((REG32_PTR_T)(0x3C700118)))
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
#define FMCSTAT_RBB             (1 << 0)
#define FMCSTAT_RBBDONE         (1 << 1)
#define FMCSTAT_CMDDONE         (1 << 2)
#define FMCSTAT_ADDRDONE        (1 << 3)
#define FMCSTAT_BANK0READY      (1 << 4)
#define FMCSTAT_BANK1READY      (1 << 5)
#define FMCSTAT_BANK2READY      (1 << 6)
#define FMCSTAT_BANK3READY      (1 << 7)

/* 13. SECURE DIGITAL CARD INTERFACE (SDCI) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define SDCI_CTRL               (*(REG32_PTR_T)(0x3C300000))     /* Control Register */
#define SDCI_DCTRL              (*(REG32_PTR_T)(0x3C300004))     /* Data Control Register */
#define SDCI_CMD                (*(REG32_PTR_T)(0x3C300008))     /* Command Register */
#define SDCI_ARGU               (*(REG32_PTR_T)(0x3C30000C))     /* Argument Register */
#define SDCI_STATE              (*(REG32_PTR_T)(0x3C300010))     /* State Register */
#define SDCI_STAC               (*(REG32_PTR_T)(0x3C300014))     /* Status Clear Register */
#define SDCI_DSTA               (*(REG32_PTR_T)(0x3C300018))     /* Data Status Register */
#define SDCI_FSTA               (*(REG32_PTR_T)(0x3C30001C))     /* FIFO Status Register */
#define SDCI_RESP0              (*(REG32_PTR_T)(0x3C300020))     /* Response0 Register */
#define SDCI_RESP1              (*(REG32_PTR_T)(0x3C300024))     /* Response1 Register */
#define SDCI_RESP2              (*(REG32_PTR_T)(0x3C300028))     /* Response2 Register */
#define SDCI_RESP3              (*(REG32_PTR_T)(0x3C30002C))     /* Response3 Register */
#define SDCI_CLKDIV             (*(REG32_PTR_T)(0x3C300030))     /* Clock Divider Register */
#define SDIO_CSR                (*(REG32_PTR_T)(0x3C300034))     /* SDIO Control & Status Register */
#define SDIO_IRQ                (*(REG32_PTR_T)(0x3C300038))     /* Interrupt Source Register */
#elif CONFIG_CPU==S5L8702
#define SDCI_CTRL     (*((REG32_PTR_T)(0x38b00000)))
#define SDCI_DCTRL    (*((REG32_PTR_T)(0x38b00004)))
#define SDCI_CMD      (*((REG32_PTR_T)(0x38b00008)))
#define SDCI_ARGU     (*((REG32_PTR_T)(0x38b0000c)))
#define SDCI_STATE    (*((REG32_PTR_T)(0x38b00010)))
#define SDCI_STAC     (*((REG32_PTR_T)(0x38b00014)))
#define SDCI_DSTA     (*((REG32_PTR_T)(0x38b00018)))
#define SDCI_FSTA     (*((REG32_PTR_T)(0x38b0001c)))
#define SDCI_RESP0    (*((REG32_PTR_T)(0x38b00020)))
#define SDCI_RESP1    (*((REG32_PTR_T)(0x38b00024)))
#define SDCI_RESP2    (*((REG32_PTR_T)(0x38b00028)))
#define SDCI_RESP3    (*((REG32_PTR_T)(0x38b0002c)))
#define SDCI_CDIV     (*((REG32_PTR_T)(0x38b00030)))
#define SDCI_SDIO_CSR (*((REG32_PTR_T)(0x38b00034)))
#define SDCI_IRQ      (*((REG32_PTR_T)(0x38b00038)))
#define SDCI_IRQ_MASK (*((REG32_PTR_T)(0x38b0003c)))
#define SDCI_DATA     (*((REG32_PTR_T)(0x38b00040)))
#define SDCI_DMAADDR  (*((void* volatile*)(0x38b00044)))
#define SDCI_DMASIZE  (*((REG32_PTR_T)(0x38b00048)))
#define SDCI_DMACOUNT (*((REG32_PTR_T)(0x38b0004c)))
#define SDCI_RESET    (*((REG32_PTR_T)(0x38b0006c)))

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
#endif

/* 14. MEMORY STICK HOST CONTROLLER */
#define MSPRE                   (*(REG32_PTR_T)(0x3C600000))     /* Prescaler Register */
#define MSINTEN                 (*(REG32_PTR_T)(0x3C600004))     /* Interrupt Enable Register */
#define MSCMD                   (*(REG32_PTR_T)(0x3C601000))     /* Command Register */
#define MSFIFO                  (*(REG32_PTR_T)(0x3C601008))     /* Receive/Transmit Register */
#define MSPP                    (*(REG32_PTR_T)(0x3C601010))     /* Parallel Port Control/Data Register */
#define MSCTRL2                 (*(REG32_PTR_T)(0x3C601014))     /* Control Register 2 */
#define MSACD                   (*(REG32_PTR_T)(0x3C601018))     /* ACD Command Register */

/* 15. SPDIF TRANSMITTER (SPDIFOUT) */
#define SPDCLKCON               (*(REG32_PTR_T)(0x3CB00000))     /* Clock Control Register */
#define SPDCON                  (*(REG32_PTR_T)(0x3CB00004))     /* Control Register 0020 */
#define SPDBSTAS                (*(REG32_PTR_T)(0x3CB00008))     /* Burst Status Register */
#define SPDCSTAS                (*(REG32_PTR_T)(0x3CB0000C))     /* Channel Status Register 0x2000 8000 */
#define SPDDAT                  (*(REG32_PTR_T)(0x3CB00010))     /* SPDIFOUT Data Buffer */
#define SPDCNT                  (*(REG32_PTR_T)(0x3CB00014))     /* Repetition Count Register */

/* 16. REED-SOLOMON ECC CODEC */
#define ECC_DATA_PTR            (*(REG32_PTR_T)(0x39E00004))     /* Data Area Start Pointer */
#define ECC_SPARE_PTR           (*(REG32_PTR_T)(0x39E00008))     /* Spare Area Start Pointer */
#define ECC_CTRL                (*(REG32_PTR_T)(0x39E0000C))     /* ECC Control Register */
#define ECC_RESULT              (*(REG32_PTR_T)(0x39E00010))     /* ECC Result */
#define ECC_UNK1                (*(REG32_PTR_T)(0x39E00014))     /* No idea what this is, but the OFW uses it on S5L8701 */
#define ECC_EVAL0               (*(REG32_PTR_T)(0x39E00020))     /* Error Eval0 Poly */
#define ECC_EVAL1               (*(REG32_PTR_T)(0x39E00024))     /* Error Eval1 Poly */
#define ECC_LOC0                (*(REG32_PTR_T)(0x39E00028))     /* Error Loc0 Poly */
#define ECC_LOC1                (*(REG32_PTR_T)(0x39E0002C))     /* Error Loc1 Poly */
#define ECC_PARITY0             (*(REG32_PTR_T)(0x39E00030))     /* Encode Parity0 Poly */
#define ECC_PARITY1             (*(REG32_PTR_T)(0x39E00034))     /* Encode Pariyt1 Poly */
#define ECC_PARITY2             (*(REG32_PTR_T)(0x39E00038))     /* Encode Parity2 Poly */
#define ECC_INT_CLR             (*(REG32_PTR_T)(0x39E00040))     /* Interrupt Clear Register */
#define ECC_SYND0               (*(REG32_PTR_T)(0x39E00044))     /* Syndrom0 Poly */
#define ECC_SYND1               (*(REG32_PTR_T)(0x39E00048))     /* Syndrom1 Poly */
#define ECC_SYND2               (*(REG32_PTR_T)(0x39E0004C))     /* Syndrom2 Poly */
#define ECCCTRL_STARTDECODING  (1 << 0)
#define ECCCTRL_STARTENCODING  (1 << 1)
#define ECCCTRL_STARTDECNOSYND (1 << 2)

/* 17. IIS Tx/Rx INTERFACE */
#define I2SCLKCON               (*(REG32_PTR_T)(0x3CA00000))     /* Clock Control Register */
#define I2STXCON                (*(REG32_PTR_T)(0x3CA00004))     /* Tx configuration Register */
#define I2STXCOM                (*(REG32_PTR_T)(0x3CA00008))     /* Tx command Register */
#define I2STXDB0                (*(REG32_PTR_T)(0x3CA00010))     /* Tx data buffer */
#define I2SRXCON                (*(REG32_PTR_T)(0x3CA00030))     /* Rx configuration Register */
#define I2SRXCOM                (*(REG32_PTR_T)(0x3CA00034))     /* Rx command Register */
#define I2SRXDB                 (*(REG32_PTR_T)(0x3CA00038))     /* Rx data buffer */
#define I2SSTATUS               (*(REG32_PTR_T)(0x3CA0003C))     /* status register */

#if CONFIG_CPU==S5L8702
#define I2SCLKDIV               (*(REG32_PTR_T)(0x3CA00040))

#define I2SCLKGATE(i)   ((i) == 2 ? CLOCKGATE_I2S2 : \
                         (i) == 1 ? CLOCKGATE_I2S1 : \
                                    CLOCKGATE_I2S0)
#endif

/* 18. IIC BUS INTERFACE */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define IICCON                  (*(REG32_PTR_T)(0x3C900000))     /* Control Register */
#define IICSTAT                 (*(REG32_PTR_T)(0x3C900004))     /* Control/Status Register */
#define IICADD                  (*(REG32_PTR_T)(0x3C900008))     /* Bus Address Register */
#define IICDS                   (*(REG32_PTR_T)(0x3C90000C))
#elif CONFIG_CPU==S5L8702
#define I2CCLKGATE(i)   ((i) == 1 ? CLOCKGATE_I2C1 : \
                                    CLOCKGATE_I2C0)

/*  s5l8702 I2C controller is similar to s5l8700, known differences are:

    * IICCON[5] is not used in s5l8702.
    * IICCON[13:8] are used to enable interrupts.
      IICSTA2[13:8] are used to read the status and write-clear interrupts.
      Known interrupts:
       [13] STOP on bus (TBC)
       [12] START on bus (TBC)
       [8] byte transmited or received in Master mode (not tested in Slave)
    * IICCON[4] does not clear interrupts, it is enabled when a byte is
      transmited or received, in Master mode the tx/rx of the next byte
      starts when it is written as "1".
*/

#define IICCON(bus)     (*((REG32_PTR_T)(0x3C600000 + 0x300000 * (bus))))
#define IICSTAT(bus)    (*((REG32_PTR_T)(0x3C600004 + 0x300000 * (bus))))
#define IICADD(bus)     (*((REG32_PTR_T)(0x3C600008 + 0x300000 * (bus))))
#define IICDS(bus)      (*((REG32_PTR_T)(0x3C60000C + 0x300000 * (bus))))
#define IICUNK10(bus)   (*((REG32_PTR_T)(0x3C600010 + 0x300000 * (bus))))
#define IICUNK14(bus)   (*((REG32_PTR_T)(0x3C600014 + 0x300000 * (bus))))
#define IICUNK18(bus)   (*((REG32_PTR_T)(0x3C600018 + 0x300000 * (bus))))
#define IICSTA2(bus)    (*((REG32_PTR_T)(0x3C600020 + 0x300000 * (bus))))
#endif

/* 19. SPI (SERIAL PERHIPERAL INTERFACE) */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define SPCLKCON                (*(REG32_PTR_T)(0x3CD00000))     /* Clock Control Register */
#define SPCON                   (*(REG32_PTR_T)(0x3CD00004))     /* Control Register */
#define SPSTA                   (*(REG32_PTR_T)(0x3CD00008))     /* Status Register */
#define SPPIN                   (*(REG32_PTR_T)(0x3CD0000C))     /* Pin Control Register */
#define SPTDAT                  (*(REG32_PTR_T)(0x3CD00010))     /* Tx Data Register */
#define SPRDAT                  (*(REG32_PTR_T)(0x3CD00014))     /* Rx Data Register */
#define SPPRE                   (*(REG32_PTR_T)(0x3CD00018))     /* Baud Rate Prescaler Register */
#elif CONFIG_CPU==S5L8702
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

/* 20. ADC CONTROLLER */
#define ADCCON                  (*(REG32_PTR_T)(0x3CE00000))     /* ADC Control Register */
#define ADCTSC                  (*(REG32_PTR_T)(0x3CE00004))     /* ADC Touch Screen Control Register */
#define ADCDLY                  (*(REG32_PTR_T)(0x3CE00008))     /* ADC Start or Interval Delay Register */
#define ADCDAT0                 (*(REG32_PTR_T)(0x3CE0000C))     /* ADC Conversion Data Register */
#define ADCDAT1                 (*(REG32_PTR_T)(0x3CE00010))     /* ADC Conversion Data Register */
#define ADCUPDN                 (*(REG32_PTR_T)(0x3CE00014))     /* Stylus Up or Down Interrpt Register */

/* 21. USB 2.0 FUNCTION CONTROLER SPECIAL REGISTER */
#define USB_IR                  (*(REG32_PTR_T)(0x38800000))     /* Index Register */
#define USB_EIR                 (*(REG32_PTR_T)(0x38800004))     /* Endpoint Interrupt Register */
#define USB_EIER                (*(REG32_PTR_T)(0x38800008))     /* Endpoint Interrupt Enable Register */
#define USB_FAR                 (*(REG32_PTR_T)(0x3880000C))     /* Function Address Register */
#define USB_FNR                 (*(REG32_PTR_T)(0x38800010))     /* Frame Number Register */
#define USB_EDR                 (*(REG32_PTR_T)(0x38800014))     /* Endpoint Direction Register */
#define USB_TR                  (*(REG32_PTR_T)(0x38800018))     /* Test Register */
#define USB_SSR                 (*(REG32_PTR_T)(0x3880001C))     /* System Status Register */
#define USB_SCR                 (*(REG32_PTR_T)(0x38800020))     /* System Control Register */
#define USB_EP0SR               (*(REG32_PTR_T)(0x38800024))     /* EP0 Status Register */
#define USB_EP0CR               (*(REG32_PTR_T)(0x38800028))     /* EP0 Control Register */
#define USB_ESR                 (*(REG32_PTR_T)(0x3880002C))     /* Endpoints Status Register */
#define USB_ECR                 (*(REG32_PTR_T)(0x38800030))     /* Endpoints Control Register */
#define USB_BRCR                (*(REG32_PTR_T)(0x38800034))     /* Byte Read Count Register */
#define USB_BWCR                (*(REG32_PTR_T)(0x38800038))     /* Byte Write Count Register */
#define USB_MPR                 (*(REG32_PTR_T)(0x3880003C))     /* Max Packet Register */
#define USB_MCR                 (*(REG32_PTR_T)(0x38800040))     /* Master Control Register */
#define USB_MTCR                (*(REG32_PTR_T)(0x38800044))     /* Master Transfer Counter Register */
#define USB_MFCR                (*(REG32_PTR_T)(0x38800048))     /* Master FIFO Counter Register */
#define USB_MTTCR1              (*(REG32_PTR_T)(0x3880004C))     /* Master Total Transfer Counter1 Register */
#define USB_MTTCR2              (*(REG32_PTR_T)(0x38800050))     /* Master Total Transfer Counter2 Register */
#define USB_EP0BR               (*(REG32_PTR_T)(0x38800060))     /* EP0 Buffer Register */
#define USB_EP1BR               (*(REG32_PTR_T)(0x38800064))     /* EP1 Buffer Register */
#define USB_EP2BR               (*(REG32_PTR_T)(0x38800068))     /* EP2 Buffer Register */
#define USB_EP3BR               (*(REG32_PTR_T)(0x3880006C))     /* EP3 Buffer Register */
#define USB_EP4BR               (*(REG32_PTR_T)(0x38800070))     /* EP4 Buffer Register */
#define USB_EP5BR               (*(REG32_PTR_T)(0x38800074))     /* EP5 Buffer Register */
#define USB_EP6BR               (*(REG32_PTR_T)(0x38800078))     /* EP6 Buffer Register */
#define USB_MICR                (*(REG32_PTR_T)(0x38800084))     /* Master Interface Counter Register */
#define USB_MBAR1               (*(REG32_PTR_T)(0x38800088))     /* Memory Base Address Register1 */
#define USB_MBAR2               (*(REG32_PTR_T)(0x3880008C))     /* Memory Base Address Register2 */
#define USB_MCAR1               (*(REG32_PTR_T)(0x38800094))     /* Memory Current Address Register1 */
#define USB_MCAR2               (*(REG32_PTR_T)(0x38800098))     /* Memory Current Address Register2 */

/* 22. USB 1.1 HOST CONTROLLER SPECIAL REGISTER */
#define HcRevision              (*(REG32_PTR_T)(0x38600000))
#define HcControl               (*(REG32_PTR_T)(0x38600004))
#define HcCommandStatus         (*(REG32_PTR_T)(0x38600008))
#define HcInterruptStatus       (*(REG32_PTR_T)(0x3860000C))
#define HcInterruptEnable       (*(REG32_PTR_T)(0x38600010))
#define HcInterruptDisable      (*(REG32_PTR_T)(0x38600014))
#define HcHCCA                  (*(REG32_PTR_T)(0x38600018))
#define HcPeriodCurrentED       (*(REG32_PTR_T)(0x3860001C))
#define HcControlHeadED         (*(REG32_PTR_T)(0x38600020))
#define HcControlCurrentED      (*(REG32_PTR_T)(0x38600024))
#define HcBulkHeadED            (*(REG32_PTR_T)(0x38600028))
#define HcBulkCurrentED         (*(REG32_PTR_T)(0x3860002C))
#define HcDoneHead              (*(REG32_PTR_T)(0x38600030))
#define HcFmInterval            (*(REG32_PTR_T)(0x38600034))
#define HcFmRemaining           (*(REG32_PTR_T)(0x38600038))
#define HcFmNumber              (*(REG32_PTR_T)(0x3860003C))
#define HcPeriodicStart         (*(REG32_PTR_T)(0x38600040))
#define HcLSThreshold           (*(REG32_PTR_T)(0x38600044))
#define HcRhDescriptorA         (*(REG32_PTR_T)(0x38600048))
#define HcRhDescriptorB         (*(REG32_PTR_T)(0x3860004C))
#define HcRhStatus              (*(REG32_PTR_T)(0x38600050))
#define HcRhPortStatus          (*(REG32_PTR_T)(0x38600054))

/* 23. USB 2.0 PHY CONTROL */
#define PHYCTRL                 (*(REG32_PTR_T)(0x3C400000))     /* USB2.0 PHY Control Register */
#define PHYPWR                  (*(REG32_PTR_T)(0x3C400004))     /* USB2.0 PHY Power Control Register */
#define URSTCON                 (*(REG32_PTR_T)(0x3C400008))     /* USB Reset Control Register */
#define UCLKCON                 (*(REG32_PTR_T)(0x3C400010))     /* USB Clock Control Register */

/* 24. GPIO PORT CONTROL */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define PCON0                   (*(REG32_PTR_T)(0x3CF00000))     /* Configures the pins of port 0 */
#define PDAT0                   (*(REG32_PTR_T)(0x3CF00004))     /* The data register for port 0 */
#define PCON1                   (*(REG32_PTR_T)(0x3CF00010))     /* Configures the pins of port 1 */
#define PDAT1                   (*(REG32_PTR_T)(0x3CF00014))     /* The data register for port 1 */
#define PCON2                   (*(REG32_PTR_T)(0x3CF00020))     /* Configures the pins of port 2 */
#define PDAT2                   (*(REG32_PTR_T)(0x3CF00024))     /* The data register for port 2 */
#define PCON3                   (*(REG32_PTR_T)(0x3CF00030))     /* Configures the pins of port 3 */
#define PDAT3                   (*(REG32_PTR_T)(0x3CF00034))     /* The data register for port 3 */
#define PCON4                   (*(REG32_PTR_T)(0x3CF00040))     /* Configures the pins of port 4 */
#define PDAT4                   (*(REG32_PTR_T)(0x3CF00044))     /* The data register for port 4 */
#define PCON5                   (*(REG32_PTR_T)(0x3CF00050))     /* Configures the pins of port 5 */
#define PDAT5                   (*(REG32_PTR_T)(0x3CF00054))     /* The data register for port 5 */
#define PUNK5                   (*(REG32_PTR_T)(0x3CF0005C))     /* Unknown thing for port 5 */
#define PCON6                   (*(REG32_PTR_T)(0x3CF00060))     /* Configures the pins of port 6 */
#define PDAT6                   (*(REG32_PTR_T)(0x3CF00064))     /* The data register for port 6 */
#define PCON7                   (*(REG32_PTR_T)(0x3CF00070))     /* Configures the pins of port 7 */
#define PDAT7                   (*(REG32_PTR_T)(0x3CF00074))     /* The data register for port 7 */
#define PCON10                  (*(REG32_PTR_T)(0x3CF000A0))     /* Configures the pins of port 10 */
#define PDAT10                  (*(REG32_PTR_T)(0x3CF000A4))     /* The data register for port 10 */
#define PCON11                  (*(REG32_PTR_T)(0x3CF000B0))     /* Configures the pins of port 11 */
#define PDAT11                  (*(REG32_PTR_T)(0x3CF000B4))     /* The data register for port 11 */
#define PCON13                  (*(REG32_PTR_T)(0x3CF000D0))     /* Configures the pins of port 13 */
#define PDAT13                  (*(REG32_PTR_T)(0x3CF000D4))     /* The data register for port 13 */
#define PCON14                  (*(REG32_PTR_T)(0x3CF000E0))     /* Configures the pins of port 14 */
#define PDAT14                  (*(REG32_PTR_T)(0x3CF000E4))     /* The data register for port 14 */
#define PCON15                  (*(REG32_PTR_T)(0x3CF000F0))     /* Configures the pins of port 15 */
#define PUNK15                  (*(REG32_PTR_T)(0x3CF000FC))     /* Unknown thing for port 15 */
#define PCON_ASRAM              (*(REG32_PTR_T)(0x3CF000F0))     /* Configures the pins of port nor flash */
#define PCON_SDRAM              (*(REG32_PTR_T)(0x3CF000F4))     /* Configures the pins of port sdram */
#elif CONFIG_CPU==S5L8702
#define PCON(i)       (*((REG32_PTR_T)(0x3cf00000 + ((i) << 5))))
#define PDAT(i)       (*((REG32_PTR_T)(0x3cf00004 + ((i) << 5))))
#define PUNA(i)       (*((REG32_PTR_T)(0x3cf00008 + ((i) << 5))))
#define PUNB(i)       (*((REG32_PTR_T)(0x3cf0000c + ((i) << 5))))
#define PUNC(i)       (*((REG32_PTR_T)(0x3cf00010 + ((i) << 5))))
#define PCON0         (*((REG32_PTR_T)(0x3cf00000)))
#define PDAT0         (*((REG32_PTR_T)(0x3cf00004)))
#define PCON1         (*((REG32_PTR_T)(0x3cf00020)))
#define PDAT1         (*((REG32_PTR_T)(0x3cf00024)))
#define PCON2         (*((REG32_PTR_T)(0x3cf00040)))
#define PDAT2         (*((REG32_PTR_T)(0x3cf00044)))
#define PCON3         (*((REG32_PTR_T)(0x3cf00060)))
#define PDAT3         (*((REG32_PTR_T)(0x3cf00064)))
#define PCON4         (*((REG32_PTR_T)(0x3cf00080)))
#define PDAT4         (*((REG32_PTR_T)(0x3cf00084)))
#define PCON5         (*((REG32_PTR_T)(0x3cf000a0)))
#define PDAT5         (*((REG32_PTR_T)(0x3cf000a4)))
#define PCON6         (*((REG32_PTR_T)(0x3cf000c0)))
#define PDAT6         (*((REG32_PTR_T)(0x3cf000c4)))
#define PCON7         (*((REG32_PTR_T)(0x3cf000e0)))
#define PDAT7         (*((REG32_PTR_T)(0x3cf000e4)))
#define PCON8         (*((REG32_PTR_T)(0x3cf00100)))
#define PDAT8         (*((REG32_PTR_T)(0x3cf00104)))
#define PCON9         (*((REG32_PTR_T)(0x3cf00120)))
#define PDAT9         (*((REG32_PTR_T)(0x3cf00124)))
#define PCONA         (*((REG32_PTR_T)(0x3cf00140)))
#define PDATA         (*((REG32_PTR_T)(0x3cf00144)))
#define PCONB         (*((REG32_PTR_T)(0x3cf00160)))
#define PDATB         (*((REG32_PTR_T)(0x3cf00164)))
#define PCONC         (*((REG32_PTR_T)(0x3cf00180)))
#define PDATC         (*((REG32_PTR_T)(0x3cf00184)))
#define PCOND         (*((REG32_PTR_T)(0x3cf001a0)))
#define PDATD         (*((REG32_PTR_T)(0x3cf001a4)))
#define PCONE         (*((REG32_PTR_T)(0x3cf001c0)))
#define PDATE         (*((REG32_PTR_T)(0x3cf001c4)))
#define PCONF         (*((REG32_PTR_T)(0x3cf001e0)))
#define PDATF         (*((REG32_PTR_T)(0x3cf001e4)))
#define GPIOCMD       (*((REG32_PTR_T)(0x3cf00200)))
#endif

/* 25. UART */
#if CONFIG_CPU==S5L8700
/* s5l8700 UC87XX HW: 1 UARTC, 2 ports */
#define S5L8700_N_UARTC         1
#define S5L8700_N_PORTS         2

#define UARTC_BASE_ADDR         0x3CC00000
#define UARTC_N_PORTS           2
#define UARTC_PORT_OFFSET       0x8000

/* UART 0 */
#define ULCON0                  (*(REG32_PTR_T)(0x3CC00000))     /* Line Control Register */
#define UCON0                   (*(REG32_PTR_T)(0x3CC00004))     /* Control Register */
#define UFCON0                  (*(REG32_PTR_T)(0x3CC00008))     /* FIFO Control Register */
#define UMCON0                  (*(REG32_PTR_T)(0x3CC0000C))     /* Modem Control Register */
#define UTRSTAT0                (*(REG32_PTR_T)(0x3CC00010))     /* Tx/Rx Status Register */
#define UERSTAT0                (*(REG32_PTR_T)(0x3CC00014))     /* Rx Error Status Register */
#define UFSTAT0                 (*(REG32_PTR_T)(0x3CC00018))     /* FIFO Status Register */
#define UMSTAT0                 (*(REG32_PTR_T)(0x3CC0001C))     /* Modem Status Register */
#define UTXH0                   (*(REG32_PTR_T)(0x3CC00020))     /* Transmit Buffer Register */
#define URXH0                   (*(REG32_PTR_T)(0x3CC00024))     /* Receive Buffer Register */
#define UBRDIV0                 (*(REG32_PTR_T)(0x3CC00028))     /* Baud Rate Divisor Register */

/* UART 1*/
#define ULCON1                  (*(REG32_PTR_T)(0x3CC08000))     /* Line Control Register */
#define UCON1                   (*(REG32_PTR_T)(0x3CC08004))     /* Control Register */
#define UFCON1                  (*(REG32_PTR_T)(0x3CC08008))     /* FIFO Control Register */
#define UMCON1                  (*(REG32_PTR_T)(0x3CC0800C))     /* Modem Control Register */
#define UTRSTAT1                (*(REG32_PTR_T)(0x3CC08010))     /* Tx/Rx Status Register */
#define UERSTAT1                (*(REG32_PTR_T)(0x3CC08014))     /* Rx Error Status Register */
#define UFSTAT1                 (*(REG32_PTR_T)(0x3CC08018))     /* FIFO Status Register */
#define UMSTAT1                 (*(REG32_PTR_T)(0x3CC0801C))     /* Modem Status Register */
#define UTXH1                   (*(REG32_PTR_T)(0x3CC08020))     /* Transmit Buffer Register */
#define URXH1                   (*(REG32_PTR_T)(0x3CC08024))     /* Receive Buffer Register */
#define UBRDIV1                 (*(REG32_PTR_T)(0x3CC08028))     /* Baud Rate Divisor Register */
#elif CONFIG_CPU==S5L8701
/* s5l8701 UC87XX HW: 3 UARTC, 1 port per UARTC */
#define S5L8701_N_UARTC         3
#define S5L8701_N_PORTS         3

#define UARTC0_BASE_ADDR        0x3CC00000
#define UARTC0_N_PORTS          1
#define UARTC0_PORT_OFFSET      0x0
#define UARTC1_BASE_ADDR        0x3CC08000
#define UARTC1_N_PORTS          1
#define UARTC1_PORT_OFFSET      0x0
#define UARTC2_BASE_ADDR        0x3CC0C000
#define UARTC2_N_PORTS          1
#define UARTC2_PORT_OFFSET      0x0
#elif CONFIG_CPU==S5L8702
/* s5l8702 UC87XX HW: 1 UARTC, 4 ports */
#define UARTC_BASE_ADDR     0x3CC00000
#define UARTC_N_PORTS       4
#define UARTC_PORT_OFFSET   0x4000
#endif

/* 26. LCD INTERFACE CONTROLLER */
#if CONFIG_CPU==S5L8700
#define LCD_BASE 0x3C100000
#elif CONFIG_CPU==S5L8701
#define LCD_BASE 0x38600000
#elif CONFIG_CPU==S5L8702
#define LCD_BASE 0x38300000
#endif

#define LCD_CON                 (*(REG32_PTR_T)(LCD_BASE+0x00))  /* Control register. */
#define LCD_WCMD                (*(REG32_PTR_T)(LCD_BASE+0x04))  /* Write command register. */
#define LCD_RCMD                (*(REG32_PTR_T)(LCD_BASE+0x0C))  /* Read command register. */
#define LCD_RDATA               (*(REG32_PTR_T)(LCD_BASE+0x10))  /* Read data register. */
#define LCD_DBUFF               (*(REG32_PTR_T)(LCD_BASE+0x14))  /* Read Data buffer */
#define LCD_INTCON              (*(REG32_PTR_T)(LCD_BASE+0x18))  /* Interrupt control register */
#define LCD_STATUS              (*(REG32_PTR_T)(LCD_BASE+0x1C))  /* LCD Interface status 0106 */
#define LCD_PHTIME              (*(REG32_PTR_T)(LCD_BASE+0x20))  /* Phase time register 0060 */
#define LCD_RST_TIME            (*(REG32_PTR_T)(LCD_BASE+0x24))  /* Reset active period 07FF */
#define LCD_DRV_RST             (*(REG32_PTR_T)(LCD_BASE+0x28))  /* Reset drive signal */
#define LCD_WDATA               (*(REG32_PTR_T)(LCD_BASE+0x40))  /* Write data register (0x40...0x5C) FIXME */

/* 27. CLCD CONTROLLER */
#define LCDCON1                 (*(REG32_PTR_T)(0x39200000))     /* LCD control 1 register */
#define LCDCON2                 (*(REG32_PTR_T)(0x39200004))     /* LCD control 2 register */
#define LCDTCON1                (*(REG32_PTR_T)(0x39200008))     /* LCD time control 1 register */
#define LCDTCON2                (*(REG32_PTR_T)(0x3920000C))     /* LCD time control 2 register */
#define LCDTCON3                (*(REG32_PTR_T)(0x39200010))     /* LCD time control 3 register */
#define LCDOSD1                 (*(REG32_PTR_T)(0x39200014))     /* LCD OSD control 1 register */
#define LCDOSD2                 (*(REG32_PTR_T)(0x39200018))     /* LCD OSD control 2 register */
#define LCDOSD3                 (*(REG32_PTR_T)(0x3920001C))     /* LCD OSD control 3 register */
#define LCDB1SADDR1             (*(REG32_PTR_T)(0x39200020))     /* Frame buffer start address register for Back-Ground buffer 1 */
#define LCDB2SADDR1             (*(REG32_PTR_T)(0x39200024))     /* Frame buffer start address register for Back-Ground buffer 2 */
#define LCDF1SADDR1             (*(REG32_PTR_T)(0x39200028))     /* Frame buffer start address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR1             (*(REG32_PTR_T)(0x3920002C))     /* Frame buffer start address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR2             (*(REG32_PTR_T)(0x39200030))     /* Frame buffer end address register for Back-Ground buffer 1 */
#define LCDB2SADDR2             (*(REG32_PTR_T)(0x39200034))     /* Frame buffer end address register for Back-Ground buffer 2 */
#define LCDF1SADDR2             (*(REG32_PTR_T)(0x39200038))     /* Frame buffer end address register for Fore-Ground (OSD) buffer 1 */
#define LCDF2SADDR2             (*(REG32_PTR_T)(0x3920003C))     /* Frame buffer end address register for Fore-Ground (OSD) buffer 2 */
#define LCDB1SADDR3             (*(REG32_PTR_T)(0x39200040))     /* Virtual screen address set for Back-Ground buffer 1 */
#define LCDB2SADDR3             (*(REG32_PTR_T)(0x39200044))     /* Virtual screen address set for Back-Ground buffer 2 */
#define LCDF1SADDR3             (*(REG32_PTR_T)(0x39200048))     /* Virtual screen address set for Fore-Ground(OSD) buffer 1 */
#define LCDF2SADDR3             (*(REG32_PTR_T)(0x3920004C))     /* Virtual screen address set for Fore-Ground(OSD) buffer 2 */
#define LCDINTCON               (*(REG32_PTR_T)(0x39200050))     /* Indicate the LCD interrupt control register */
#define LCDKEYCON               (*(REG32_PTR_T)(0x39200054))     /* Color key control register */
#define LCDCOLVAL               (*(REG32_PTR_T)(0x39200058))     /* Color key value ( transparent value) register */
#define LCDBGCON                (*(REG32_PTR_T)(0x3920005C))     /* Back-Ground color control */
#define LCDFGCON                (*(REG32_PTR_T)(0x39200060))     /* Fore-Ground color control */
#define LCDDITHMODE             (*(REG32_PTR_T)(0x39200064))     /* Dithering mode register. */

/* 28. ATA CONTROLLER */
#if CONFIG_CPU==S5L8700 || CONFIG_CPU==S5L8701
#define ATA_CONTROL             (*(REG32_PTR_T)(0x38E00000))     /* Enable and clock down status */
#define ATA_STATUS              (*(REG32_PTR_T)(0x38E00004))     /* Status */
#define ATA_COMMAND             (*(REG32_PTR_T)(0x38E00008))     /* Command */
#define ATA_SWRST               (*(REG32_PTR_T)(0x38E0000C))     /* Software reset */
#define ATA_IRQ                 (*(REG32_PTR_T)(0x38E00010))     /* Interrupt sources */
#define ATA_IRQ_MASK            (*(REG32_PTR_T)(0x38E00014))     /* Interrupt mask */
#define ATA_CFG                 (*(REG32_PTR_T)(0x38E00018))     /* Configuration for ATA interface */
#define ATA_PIO_TIME            (*(REG32_PTR_T)(0x38E0002C))     /* PIO timing */
#define ATA_UDMA_TIME           (*(REG32_PTR_T)(0x38E00030))     /* UDMA timing */
#define ATA_XFR_NUM             (*(REG32_PTR_T)(0x38E00034))     /* Transfer number */
#define ATA_XFR_CNT             (*(REG32_PTR_T)(0x38E00038))     /* Current transfer count */
#define ATA_TBUF_START          (*(REG32_PTR_T)(0x38E0003C))     /* Start address of track buffer */
#define ATA_TBUF_SIZE           (*(REG32_PTR_T)(0x38E00040))     /* Size of track buffer */
#define ATA_SBUF_START          (*(REG32_PTR_T)(0x38E00044))     /* Start address of Source buffer1 */
#define ATA_SBUF_SIZE           (*(REG32_PTR_T)(0x38E00048))     /* Size of source buffer1 */
#define ATA_CADR_TBUF           (*(REG32_PTR_T)(0x38E0004C))     /* Current write address of track buffer */
#define ATA_CADR_SBUF           (*(REG32_PTR_T)(0x38E00050))     /* Current read address of source buffer */
#define ATA_PIO_DTR             (*(REG32_PTR_T)(0x38E00054))     /* PIO device data register */
#define ATA_PIO_FED             (*(REG32_PTR_T)(0x38E00058))     /* PIO device Feature/Error register */
#define ATA_PIO_SCR             (*(REG32_PTR_T)(0x38E0005C))     /* PIO sector count register */
#define ATA_PIO_LLR             (*(REG32_PTR_T)(0x38E00060))     /* PIO device LBA low register */
#define ATA_PIO_LMR             (*(REG32_PTR_T)(0x38E00064))     /* PIO device LBA middle register */
#define ATA_PIO_LHR             (*(REG32_PTR_T)(0x38E00068))     /* PIO device LBA high register */
#define ATA_PIO_DVR             (*(REG32_PTR_T)(0x38E0006C))     /* PIO device register */
#define ATA_PIO_CSD             (*(REG32_PTR_T)(0x38E00070))     /* PIO device command/status register */
#define ATA_PIO_DAD             (*(REG32_PTR_T)(0x38E00074))     /* PIO control/alternate status register */
#define ATA_PIO_READY           (*(REG32_PTR_T)(0x38E00078))     /* PIO data read/write ready */
#define ATA_PIO_RDATA           (*(REG32_PTR_T)(0x38E0007C))     /* PIO read data from device register */
#define BUS_FIFO_STATUS         (*(REG32_PTR_T)(0x38E00080))     /* Reserved */
#define ATA_FIFO_STATUS         (*(REG32_PTR_T)(0x38E00084))     /* Reserved */
#elif CONFIG_CPU==S5L8702
#define ATA_CONTROL         (*((REG32_PTR_T)(0x38700000)))
#define ATA_STATUS          (*((REG32_PTR_T)(0x38700004)))
#define ATA_COMMAND         (*((REG32_PTR_T)(0x38700008)))
#define ATA_SWRST           (*((REG32_PTR_T)(0x3870000c)))
#define ATA_IRQ             (*((REG32_PTR_T)(0x38700010)))
#define ATA_IRQ_MASK        (*((REG32_PTR_T)(0x38700014)))
#define ATA_CFG             (*((REG32_PTR_T)(0x38700018)))
#define ATA_MDMA_TIME       (*((REG32_PTR_T)(0x38700028)))
#define ATA_PIO_TIME        (*((REG32_PTR_T)(0x3870002c)))
#define ATA_UDMA_TIME       (*((REG32_PTR_T)(0x38700030)))
#define ATA_XFR_NUM         (*((REG32_PTR_T)(0x38700034)))
#define ATA_XFR_CNT         (*((REG32_PTR_T)(0x38700038)))
#define ATA_TBUF_START      (*((void* volatile*)(0x3870003c)))
#define ATA_TBUF_SIZE       (*((REG32_PTR_T)(0x38700040)))
#define ATA_SBUF_START      (*((void* volatile*)(0x38700044)))
#define ATA_SBUF_SIZE       (*((REG32_PTR_T)(0x38700048)))
#define ATA_CADR_TBUF       (*((void* volatile*)(0x3870004c)))
#define ATA_CADR_SBUF       (*((void* volatile*)(0x38700050)))
#define ATA_PIO_DTR         (*((REG32_PTR_T)(0x38700054)))
#define ATA_PIO_FED         (*((REG32_PTR_T)(0x38700058)))
#define ATA_PIO_SCR         (*((REG32_PTR_T)(0x3870005c)))
#define ATA_PIO_LLR         (*((REG32_PTR_T)(0x38700060)))
#define ATA_PIO_LMR         (*((REG32_PTR_T)(0x38700064)))
#define ATA_PIO_LHR         (*((REG32_PTR_T)(0x38700068)))
#define ATA_PIO_DVR         (*((REG32_PTR_T)(0x3870006c)))
#define ATA_PIO_CSD         (*((REG32_PTR_T)(0x38700070)))
#define ATA_PIO_DAD         (*((REG32_PTR_T)(0x38700074)))
#define ATA_PIO_READY       (*((REG32_PTR_T)(0x38700078)))
#define ATA_PIO_RDATA       (*((REG32_PTR_T)(0x3870007c)))
#define ATA_BUS_FIFO_STATUS (*((REG32_PTR_T)(0x38700080)))
#define ATA_FIFO_STATUS     (*((REG32_PTR_T)(0x38700084)))
#define ATA_DMA_ADDR        (*((void* volatile*)(0x38700088)))
#endif

/* 29. CHIP ID */
#define REG_ONE                 (*(REG32_PTR_T)(0x3D100000))     /* Receive the first 32 bits from a fuse box */
#define REG_TWO                 (*(REG32_PTR_T)(0x3D100004))     /* Receive the other 8 bits from a fuse box */


/* Hardware AES crypto unit - S5L8701+ */
#if CONFIG_CPU==S5L8701
#define AESCONTROL              (*(REG32_PTR_T)(0x39800000))
#define AESGO                   (*(REG32_PTR_T)(0x39800004))
#define AESUNKREG0              (*(REG32_PTR_T)(0x39800008))
#define AESSTATUS               (*(REG32_PTR_T)(0x3980000C))
#define AESUNKREG1              (*(REG32_PTR_T)(0x39800010))
#define AESKEYLEN               (*(REG32_PTR_T)(0x39800014))
#define AESOUTSIZE              (*(REG32_PTR_T)(0x39800018))
#define AESOUTADDR              (*(REG32_PTR_T)(0x39800020))
#define AESINSIZE               (*(REG32_PTR_T)(0x39800024))
#define AESINADDR               (*(REG32_PTR_T)(0x39800028))
#define AESAUXSIZE              (*(REG32_PTR_T)(0x3980002C))
#define AESAUXADDR              (*(REG32_PTR_T)(0x39800030))
#define AESSIZE3                (*(REG32_PTR_T)(0x39800034))
#define AESTYPE                 (*(REG32_PTR_T)(0x3980006C))
#elif CONFIG_CPU==S5L8702
#define AESCONTROL    (*((REG32_PTR_T)(0x38c00000)))
#define AESGO         (*((REG32_PTR_T)(0x38c00004)))
#define AESUNKREG0    (*((REG32_PTR_T)(0x38c00008)))
#define AESSTATUS     (*((REG32_PTR_T)(0x38c0000c)))
#define AESUNKREG1    (*((REG32_PTR_T)(0x38c00010)))
#define AESKEYLEN     (*((REG32_PTR_T)(0x38c00014)))
#define AESOUTSIZE    (*((REG32_PTR_T)(0x38c00018)))
#define AESOUTADDR    (*((void* volatile*)(0x38c00020)))
#define AESINSIZE     (*((REG32_PTR_T)(0x38c00024)))
#define AESINADDR     (*((const void* volatile*)(0x38c00028)))
#define AESAUXSIZE    (*((REG32_PTR_T)(0x38c0002c)))
#define AESAUXADDR    (*((void* volatile*)(0x38c00030)))
#define AESSIZE3      (*((REG32_PTR_T)(0x38c00034)))
#define AESKEY          ((REG32_PTR_T)(0x38c0004c))
#define AESTYPE       (*((REG32_PTR_T)(0x38c0006c)))
#define AESIV           ((REG32_PTR_T)(0x38c00074))
#define AESTYPE2      (*((REG32_PTR_T)(0x38c00088)))
#define AESUNKREG2    (*((REG32_PTR_T)(0x38c0008c)))
#endif

/* SHA-1 unit - S5L8701+ */
#if CONFIG_CPU==S5L8701
#define HASHCTRL                (*(REG32_PTR_T)(0x3C600000))
#define HASHRESULT               ((REG32_PTR_T)(0x3C600020))
#define HASHDATAIN               ((REG32_PTR_T)(0x3C600040))
#elif CONFIG_CPU==S5L8702
#define SHA1CONFIG    (*((REG32_PTR_T)(0x38000000)))
#define SHA1RESET     (*((REG32_PTR_T)(0x38000004)))
#define SHA1RESULT      ((REG32_PTR_T)(0x38000020))
#define SHA1DATAIN      ((REG32_PTR_T)(0x38000040))
#endif

/* Clickwheel controller - S5L8701+ */
#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
#define WHEEL00      (*((REG32_PTR_T)(0x3C200000)))
#define WHEEL04      (*((REG32_PTR_T)(0x3C200004)))
#define WHEEL08      (*((REG32_PTR_T)(0x3C200008)))
#define WHEEL0C      (*((REG32_PTR_T)(0x3C20000C)))
#define WHEEL10      (*((REG32_PTR_T)(0x3C200010)))
#define WHEELINT     (*((REG32_PTR_T)(0x3C200014)))
#define WHEELRX      (*((REG32_PTR_T)(0x3C200018)))
#define WHEELTX      (*((REG32_PTR_T)(0x3C20001C)))
#endif

/* Synopsys OTG - S5L8701 only */
#if CONFIG_CPU==S5L8701
#define OTGBASE 0x38800000
#elif CONFIG_CPU==S5L8702
#define OTGBASE 0x38400000
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
#define PHYBASE 0x3C400000

/* OTG PHY control registers */
#define OPHYPWR     (*((REG32_PTR_T)(PHYBASE + 0x000)))
#define OPHYCLK     (*((REG32_PTR_T)(PHYBASE + 0x004)))
#define ORSTCON     (*((REG32_PTR_T)(PHYBASE + 0x008)))
#define OPHYUNK3    (*((REG32_PTR_T)(PHYBASE + 0x018)))
#define OPHYUNK1    (*((REG32_PTR_T)(PHYBASE + 0x01c)))
#define OPHYUNK2    (*((REG32_PTR_T)(PHYBASE + 0x044)))
#endif

#if CONFIG_CPU==S5L8701
/* 7 available EPs (0b00000000011101010000000001101011), 6 used */
#define USB_NUM_ENDPOINTS 6
#elif CONFIG_CPU==S5L8702
/* 9 available EPs (0b00000001111101010000000111101011), 6 used */
#define USB_NUM_ENDPOINTS 6
#endif

#if CONFIG_CPU==S5L8701
/* Define this if the DWC implemented on this SoC does not support
   dedicated FIFOs. */
#define USB_DW_SHARED_FIFO
#endif

#if CONFIG_CPU==S5L8701 || CONFIG_CPU==S5L8702
/* Define this if the DWC implemented on this SoC does not support
   DMA or you want to disable it. */
// #define USB_DW_ARCH_SLAVE
#endif

#if CONFIG_CPU==S5L8702
/////INTERRUPT CONTROLLERS/////
#define VICIRQSTATUS(v)       (*((REG32_PTR_T)(0x38E00000 + 0x1000 * (v))))
#define VICFIQSTATUS(v)       (*((REG32_PTR_T)(0x38E00004 + 0x1000 * (v))))
#define VICRAWINTR(v)         (*((REG32_PTR_T)(0x38E00008 + 0x1000 * (v))))
#define VICINTSELECT(v)       (*((REG32_PTR_T)(0x38E0000C + 0x1000 * (v))))
#define VICINTENABLE(v)       (*((REG32_PTR_T)(0x38E00010 + 0x1000 * (v))))
#define VICINTENCLEAR(v)      (*((REG32_PTR_T)(0x38E00014 + 0x1000 * (v))))
#define VICSOFTINT(v)         (*((REG32_PTR_T)(0x38E00018 + 0x1000 * (v))))
#define VICSOFTINTCLEAR(v)    (*((REG32_PTR_T)(0x38E0001C + 0x1000 * (v))))
#define VICPROTECTION(v)      (*((REG32_PTR_T)(0x38E00020 + 0x1000 * (v))))
#define VICSWPRIORITYMASK(v)  (*((REG32_PTR_T)(0x38E00024 + 0x1000 * (v))))
#define VICPRIORITYDAISY(v)   (*((REG32_PTR_T)(0x38E00028 + 0x1000 * (v))))
#define VICVECTADDR(v, i)     (*((REG32_PTR_T)(0x38E00100 + 0x1000 * (v) + 4 * (i))))
#define VICVECTPRIORITY(v, i) (*((REG32_PTR_T)(0x38E00200 + 0x1000 * (v) + 4 * (i))))
#define VICADDRESS(v)         (*((const void* volatile*)(0x38E00F00 + 0x1000 * (v))))
#define VIC0IRQSTATUS         (*((REG32_PTR_T)(0x38E00000)))
#define VIC0FIQSTATUS         (*((REG32_PTR_T)(0x38E00004)))
#define VIC0RAWINTR           (*((REG32_PTR_T)(0x38E00008)))
#define VIC0INTSELECT         (*((REG32_PTR_T)(0x38E0000C)))
#define VIC0INTENABLE         (*((REG32_PTR_T)(0x38E00010)))
#define VIC0INTENCLEAR        (*((REG32_PTR_T)(0x38E00014)))
#define VIC0SOFTINT           (*((REG32_PTR_T)(0x38E00018)))
#define VIC0SOFTINTCLEAR      (*((REG32_PTR_T)(0x38E0001C)))
#define VIC0PROTECTION        (*((REG32_PTR_T)(0x38E00020)))
#define VIC0SWPRIORITYMASK    (*((REG32_PTR_T)(0x38E00024)))
#define VIC0PRIORITYDAISY     (*((REG32_PTR_T)(0x38E00028)))
#define VIC0VECTADDR(i)       (*((const void* volatile*)(0x38E00100 + 4 * (i))))
#define VIC0VECTADDR0         (*((const void* volatile*)(0x38E00100)))
#define VIC0VECTADDR1         (*((const void* volatile*)(0x38E00104)))
#define VIC0VECTADDR2         (*((const void* volatile*)(0x38E00108)))
#define VIC0VECTADDR3         (*((const void* volatile*)(0x38E0010C)))
#define VIC0VECTADDR4         (*((const void* volatile*)(0x38E00110)))
#define VIC0VECTADDR5         (*((const void* volatile*)(0x38E00114)))
#define VIC0VECTADDR6         (*((const void* volatile*)(0x38E00118)))
#define VIC0VECTADDR7         (*((const void* volatile*)(0x38E0011C)))
#define VIC0VECTADDR8         (*((const void* volatile*)(0x38E00120)))
#define VIC0VECTADDR9         (*((const void* volatile*)(0x38E00124)))
#define VIC0VECTADDR10        (*((const void* volatile*)(0x38E00128)))
#define VIC0VECTADDR11        (*((const void* volatile*)(0x38E0012C)))
#define VIC0VECTADDR12        (*((const void* volatile*)(0x38E00130)))
#define VIC0VECTADDR13        (*((const void* volatile*)(0x38E00134)))
#define VIC0VECTADDR14        (*((const void* volatile*)(0x38E00138)))
#define VIC0VECTADDR15        (*((const void* volatile*)(0x38E0013C)))
#define VIC0VECTADDR16        (*((const void* volatile*)(0x38E00140)))
#define VIC0VECTADDR17        (*((const void* volatile*)(0x38E00144)))
#define VIC0VECTADDR18        (*((const void* volatile*)(0x38E00148)))
#define VIC0VECTADDR19        (*((const void* volatile*)(0x38E0014C)))
#define VIC0VECTADDR20        (*((const void* volatile*)(0x38E00150)))
#define VIC0VECTADDR21        (*((const void* volatile*)(0x38E00154)))
#define VIC0VECTADDR22        (*((const void* volatile*)(0x38E00158)))
#define VIC0VECTADDR23        (*((const void* volatile*)(0x38E0015C)))
#define VIC0VECTADDR24        (*((const void* volatile*)(0x38E00160)))
#define VIC0VECTADDR25        (*((const void* volatile*)(0x38E00164)))
#define VIC0VECTADDR26        (*((const void* volatile*)(0x38E00168)))
#define VIC0VECTADDR27        (*((const void* volatile*)(0x38E0016C)))
#define VIC0VECTADDR28        (*((const void* volatile*)(0x38E00170)))
#define VIC0VECTADDR29        (*((const void* volatile*)(0x38E00174)))
#define VIC0VECTADDR30        (*((const void* volatile*)(0x38E00178)))
#define VIC0VECTADDR31        (*((const void* volatile*)(0x38E0017C)))
#define VIC0VECTPRIORITY(i)   (*((REG32_PTR_T)(0x38E00200 + 4 * (i))))
#define VIC0VECTPRIORITY0     (*((REG32_PTR_T)(0x38E00200)))
#define VIC0VECTPRIORITY1     (*((REG32_PTR_T)(0x38E00204)))
#define VIC0VECTPRIORITY2     (*((REG32_PTR_T)(0x38E00208)))
#define VIC0VECTPRIORITY3     (*((REG32_PTR_T)(0x38E0020C)))
#define VIC0VECTPRIORITY4     (*((REG32_PTR_T)(0x38E00210)))
#define VIC0VECTPRIORITY5     (*((REG32_PTR_T)(0x38E00214)))
#define VIC0VECTPRIORITY6     (*((REG32_PTR_T)(0x38E00218)))
#define VIC0VECTPRIORITY7     (*((REG32_PTR_T)(0x38E0021C)))
#define VIC0VECTPRIORITY8     (*((REG32_PTR_T)(0x38E00220)))
#define VIC0VECTPRIORITY9     (*((REG32_PTR_T)(0x38E00224)))
#define VIC0VECTPRIORITY10    (*((REG32_PTR_T)(0x38E00228)))
#define VIC0VECTPRIORITY11    (*((REG32_PTR_T)(0x38E0022C)))
#define VIC0VECTPRIORITY12    (*((REG32_PTR_T)(0x38E00230)))
#define VIC0VECTPRIORITY13    (*((REG32_PTR_T)(0x38E00234)))
#define VIC0VECTPRIORITY14    (*((REG32_PTR_T)(0x38E00238)))
#define VIC0VECTPRIORITY15    (*((REG32_PTR_T)(0x38E0023C)))
#define VIC0VECTPRIORITY16    (*((REG32_PTR_T)(0x38E00240)))
#define VIC0VECTPRIORITY17    (*((REG32_PTR_T)(0x38E00244)))
#define VIC0VECTPRIORITY18    (*((REG32_PTR_T)(0x38E00248)))
#define VIC0VECTPRIORITY19    (*((REG32_PTR_T)(0x38E0024C)))
#define VIC0VECTPRIORITY20    (*((REG32_PTR_T)(0x38E00250)))
#define VIC0VECTPRIORITY21    (*((REG32_PTR_T)(0x38E00254)))
#define VIC0VECTPRIORITY22    (*((REG32_PTR_T)(0x38E00258)))
#define VIC0VECTPRIORITY23    (*((REG32_PTR_T)(0x38E0025C)))
#define VIC0VECTPRIORITY24    (*((REG32_PTR_T)(0x38E00260)))
#define VIC0VECTPRIORITY25    (*((REG32_PTR_T)(0x38E00264)))
#define VIC0VECTPRIORITY26    (*((REG32_PTR_T)(0x38E00268)))
#define VIC0VECTPRIORITY27    (*((REG32_PTR_T)(0x38E0026C)))
#define VIC0VECTPRIORITY28    (*((REG32_PTR_T)(0x38E00270)))
#define VIC0VECTPRIORITY29    (*((REG32_PTR_T)(0x38E00274)))
#define VIC0VECTPRIORITY30    (*((REG32_PTR_T)(0x38E00278)))
#define VIC0VECTPRIORITY31    (*((REG32_PTR_T)(0x38E0027C)))
#define VIC0ADDRESS           (*((void* volatile*)(0x38E00F00)))
#define VIC1IRQSTATUS         (*((REG32_PTR_T)(0x38E01000)))
#define VIC1FIQSTATUS         (*((REG32_PTR_T)(0x38E01004)))
#define VIC1RAWINTR           (*((REG32_PTR_T)(0x38E01008)))
#define VIC1INTSELECT         (*((REG32_PTR_T)(0x38E0100C)))
#define VIC1INTENABLE         (*((REG32_PTR_T)(0x38E01010)))
#define VIC1INTENCLEAR        (*((REG32_PTR_T)(0x38E01014)))
#define VIC1SOFTINT           (*((REG32_PTR_T)(0x38E01018)))
#define VIC1SOFTINTCLEAR      (*((REG32_PTR_T)(0x38E0101C)))
#define VIC1PROTECTION        (*((REG32_PTR_T)(0x38E01020)))
#define VIC1SWPRIORITYMASK    (*((REG32_PTR_T)(0x38E01024)))
#define VIC1PRIORITYDAISY     (*((REG32_PTR_T)(0x38E01028)))
#define VIC1VECTADDR(i)       (*((const void* volatile*)(0x38E01100 + 4 * (i))))
#define VIC1VECTADDR0         (*((const void* volatile*)(0x38E01100)))
#define VIC1VECTADDR1         (*((const void* volatile*)(0x38E01104)))
#define VIC1VECTADDR2         (*((const void* volatile*)(0x38E01108)))
#define VIC1VECTADDR3         (*((const void* volatile*)(0x38E0110C)))
#define VIC1VECTADDR4         (*((const void* volatile*)(0x38E01110)))
#define VIC1VECTADDR5         (*((const void* volatile*)(0x38E01114)))
#define VIC1VECTADDR6         (*((const void* volatile*)(0x38E01118)))
#define VIC1VECTADDR7         (*((const void* volatile*)(0x38E0111C)))
#define VIC1VECTADDR8         (*((const void* volatile*)(0x38E01120)))
#define VIC1VECTADDR9         (*((const void* volatile*)(0x38E01124)))
#define VIC1VECTADDR10        (*((const void* volatile*)(0x38E01128)))
#define VIC1VECTADDR11        (*((const void* volatile*)(0x38E0112C)))
#define VIC1VECTADDR12        (*((const void* volatile*)(0x38E01130)))
#define VIC1VECTADDR13        (*((const void* volatile*)(0x38E01134)))
#define VIC1VECTADDR14        (*((const void* volatile*)(0x38E01138)))
#define VIC1VECTADDR15        (*((const void* volatile*)(0x38E0113C)))
#define VIC1VECTADDR16        (*((const void* volatile*)(0x38E01140)))
#define VIC1VECTADDR17        (*((const void* volatile*)(0x38E01144)))
#define VIC1VECTADDR18        (*((const void* volatile*)(0x38E01148)))
#define VIC1VECTADDR19        (*((const void* volatile*)(0x38E0114C)))
#define VIC1VECTADDR20        (*((const void* volatile*)(0x38E01150)))
#define VIC1VECTADDR21        (*((const void* volatile*)(0x38E01154)))
#define VIC1VECTADDR22        (*((const void* volatile*)(0x38E01158)))
#define VIC1VECTADDR23        (*((const void* volatile*)(0x38E0115C)))
#define VIC1VECTADDR24        (*((const void* volatile*)(0x38E01160)))
#define VIC1VECTADDR25        (*((const void* volatile*)(0x38E01164)))
#define VIC1VECTADDR26        (*((const void* volatile*)(0x38E01168)))
#define VIC1VECTADDR27        (*((const void* volatile*)(0x38E0116C)))
#define VIC1VECTADDR28        (*((const void* volatile*)(0x38E01170)))
#define VIC1VECTADDR29        (*((const void* volatile*)(0x38E01174)))
#define VIC1VECTADDR30        (*((const void* volatile*)(0x38E01178)))
#define VIC1VECTADDR31        (*((const void* volatile*)(0x38E0117C)))
#define VIC1VECTPRIORITY(i)   (*((REG32_PTR_T)(0x38E01200 + 4 * (i))))
#define VIC1VECTPRIORITY0     (*((REG32_PTR_T)(0x38E01200)))
#define VIC1VECTPRIORITY1     (*((REG32_PTR_T)(0x38E01204)))
#define VIC1VECTPRIORITY2     (*((REG32_PTR_T)(0x38E01208)))
#define VIC1VECTPRIORITY3     (*((REG32_PTR_T)(0x38E0120C)))
#define VIC1VECTPRIORITY4     (*((REG32_PTR_T)(0x38E01210)))
#define VIC1VECTPRIORITY5     (*((REG32_PTR_T)(0x38E01214)))
#define VIC1VECTPRIORITY6     (*((REG32_PTR_T)(0x38E01218)))
#define VIC1VECTPRIORITY7     (*((REG32_PTR_T)(0x38E0121C)))
#define VIC1VECTPRIORITY8     (*((REG32_PTR_T)(0x38E01220)))
#define VIC1VECTPRIORITY9     (*((REG32_PTR_T)(0x38E01224)))
#define VIC1VECTPRIORITY10    (*((REG32_PTR_T)(0x38E01228)))
#define VIC1VECTPRIORITY11    (*((REG32_PTR_T)(0x38E0122C)))
#define VIC1VECTPRIORITY12    (*((REG32_PTR_T)(0x38E01230)))
#define VIC1VECTPRIORITY13    (*((REG32_PTR_T)(0x38E01234)))
#define VIC1VECTPRIORITY14    (*((REG32_PTR_T)(0x38E01238)))
#define VIC1VECTPRIORITY15    (*((REG32_PTR_T)(0x38E0123C)))
#define VIC1VECTPRIORITY16    (*((REG32_PTR_T)(0x38E01240)))
#define VIC1VECTPRIORITY17    (*((REG32_PTR_T)(0x38E01244)))
#define VIC1VECTPRIORITY18    (*((REG32_PTR_T)(0x38E01248)))
#define VIC1VECTPRIORITY19    (*((REG32_PTR_T)(0x38E0124C)))
#define VIC1VECTPRIORITY20    (*((REG32_PTR_T)(0x38E01250)))
#define VIC1VECTPRIORITY21    (*((REG32_PTR_T)(0x38E01254)))
#define VIC1VECTPRIORITY22    (*((REG32_PTR_T)(0x38E01258)))
#define VIC1VECTPRIORITY23    (*((REG32_PTR_T)(0x38E0125C)))
#define VIC1VECTPRIORITY24    (*((REG32_PTR_T)(0x38E01260)))
#define VIC1VECTPRIORITY25    (*((REG32_PTR_T)(0x38E01264)))
#define VIC1VECTPRIORITY26    (*((REG32_PTR_T)(0x38E01268)))
#define VIC1VECTPRIORITY27    (*((REG32_PTR_T)(0x38E0126C)))
#define VIC1VECTPRIORITY28    (*((REG32_PTR_T)(0x38E01270)))
#define VIC1VECTPRIORITY29    (*((REG32_PTR_T)(0x38E01274)))
#define VIC1VECTPRIORITY30    (*((REG32_PTR_T)(0x38E01278)))
#define VIC1VECTPRIORITY31    (*((REG32_PTR_T)(0x38E0127C)))
#define VIC1ADDRESS           (*((void* volatile*)(0x38E01F00)))

/////INTERRUPTS/////
#define IRQ_TIMER32     7
#define IRQ_TIMER       8
#define IRQ_SPI(i)      (9+(i)) /* TBC */
#define IRQ_SPI0        9
#define IRQ_SPI1        10
#define IRQ_SPI2        11
#define IRQ_LCD         14
#define IRQ_DMAC(d)     (16+(d))
#define IRQ_DMAC0       16
#define IRQ_DMAC1       17
#define IRQ_USB_FUNC    19
#define IRQ_I2C(i)      (21+(i))
#define IRQ_I2C0        21
#define IRQ_I2C1        22
#define IRQ_WHEEL       23
#define IRQ_UART(i)     (24+(i))
#define IRQ_UART0       24
#define IRQ_UART1       25
#define IRQ_UART2       26
#define IRQ_UART3       27
#define IRQ_UART4       28    /* obsolete/not implemented on s5l8702 ??? */
#define IRQ_ATA         29
#define IRQ_SBOOT       36
#define IRQ_AES         39
#define IRQ_SHA         40
#define IRQ_MMC         44

#define IRQ_EXT0        0
#define IRQ_EXT1        1
#define IRQ_EXT2        2
#define IRQ_EXT3        3
#define IRQ_EXT4        31
#define IRQ_EXT5        32
#define IRQ_EXT6        33
#endif

#endif /* __S5L8700_H__ */
