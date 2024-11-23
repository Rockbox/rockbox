/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: s5l8700.h 28791 2010-12-11 09:39:33Z Buschel $
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

#ifndef __S5L8702_H__
#define __S5L8702_H__

#ifndef ASM
#include <stdint.h>
#endif

#define REG32_PTR_T volatile uint32_t *

#define CACHEALIGN_BITS (5) /* 2^5 = 32 bytes */

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


/////SYSTEM CONTROLLER/////
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


/////WATCHDOG/////
#define WDTCON      (*((REG32_PTR_T)(0x3C800000)))
#define WDTCNT      (*((REG32_PTR_T)(0x3C800004)))


/////MEMCONTROLLER/////
#define MIU_BASE        (0x38100000)
#define MIU_REG(off)    (*((REG32_PTR_T)(MIU_BASE + (off))))
/* following registers are similar to s5l8700x */
#define MIUCON          (*((REG32_PTR_T)(0x38100000)))
#define MIUCOM          (*((REG32_PTR_T)(0x38100004)))
#define MIUAREF         (*((REG32_PTR_T)(0x38100008)))
#define MIUMRS          (*((REG32_PTR_T)(0x3810000C)))
#define MIUSDPARA       (*((REG32_PTR_T)(0x38100010)))


/////TIMER/////
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

#define TACON        (*((REG32_PTR_T)(0x3C700000)))
#define TACMD        (*((REG32_PTR_T)(0x3C700004)))
#define TADATA0      (*((REG32_PTR_T)(0x3C700008)))
#define TADATA1      (*((REG32_PTR_T)(0x3C70000C)))
#define TAPRE        (*((REG32_PTR_T)(0x3C700010)))
#define TACNT        (*((REG32_PTR_T)(0x3C700014)))
#define TBCON        (*((REG32_PTR_T)(0x3C700020)))
#define TBCMD        (*((REG32_PTR_T)(0x3C700024)))
#define TBDATA0      (*((REG32_PTR_T)(0x3C700028)))
#define TBDATA1      (*((REG32_PTR_T)(0x3C70002C)))
#define TBPRE        (*((REG32_PTR_T)(0x3C700030)))
#define TBCNT        (*((REG32_PTR_T)(0x3C700034)))
#define TCCON        (*((REG32_PTR_T)(0x3C700040)))
#define TCCMD        (*((REG32_PTR_T)(0x3C700044)))
#define TCDATA0      (*((REG32_PTR_T)(0x3C700048)))
#define TCDATA1      (*((REG32_PTR_T)(0x3C70004C)))
#define TCPRE        (*((REG32_PTR_T)(0x3C700050)))
#define TCCNT        (*((REG32_PTR_T)(0x3C700054)))
#define TDCON        (*((REG32_PTR_T)(0x3C700060)))
#define TDCMD        (*((REG32_PTR_T)(0x3C700064)))
#define TDDATA0      (*((REG32_PTR_T)(0x3C700068)))
#define TDDATA1      (*((REG32_PTR_T)(0x3C70006C)))
#define TDPRE        (*((REG32_PTR_T)(0x3C700070)))
#define TDCNT        (*((REG32_PTR_T)(0x3C700074)))
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


/////USB/////
#define OTGBASE 0x38400000
#define PHYBASE 0x3C400000

/* OTG PHY control registers */
#define OPHYPWR     (*((REG32_PTR_T)(PHYBASE + 0x000)))
#define OPHYCLK     (*((REG32_PTR_T)(PHYBASE + 0x004)))
#define ORSTCON     (*((REG32_PTR_T)(PHYBASE + 0x008)))
#define OPHYUNK3    (*((REG32_PTR_T)(PHYBASE + 0x018)))
#define OPHYUNK1    (*((REG32_PTR_T)(PHYBASE + 0x01c)))
#define OPHYUNK2    (*((REG32_PTR_T)(PHYBASE + 0x044)))

/* 9 available EPs (0b00000001111101010000000111101011), 6 used */
#define USB_NUM_ENDPOINTS 6

/* Define this if the DWC implemented on this SoC does not support
   DMA or you want to disable it. */
// #define USB_DW_ARCH_SLAVE


/////I2C/////
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


/////GPIO/////
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


/////SPI/////
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


/////AES/////
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


/////SHA1/////
#define SHA1CONFIG    (*((REG32_PTR_T)(0x38000000)))
#define SHA1RESET     (*((REG32_PTR_T)(0x38000004)))
#define SHA1RESULT      ((REG32_PTR_T)(0x38000020))
#define SHA1DATAIN      ((REG32_PTR_T)(0x38000040))


/////LCD/////
#define LCD_BASE   (0x38300000)
#define LCD_CONFIG (*((REG32_PTR_T)(0x38300000)))
#define LCD_WCMD   (*((REG32_PTR_T)(0x38300004)))
#define LCD_STATUS (*((REG32_PTR_T)(0x3830001c)))
#define LCD_PHTIME (*((REG32_PTR_T)(0x38300020)))
#define LCD_WDATA  (*((REG32_PTR_T)(0x38300040)))


/////ATA/////
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


/////SDCI/////
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


/////CLICKWHEEL/////
#define WHEEL00      (*((REG32_PTR_T)(0x3C200000)))
#define WHEEL04      (*((REG32_PTR_T)(0x3C200004)))
#define WHEEL08      (*((REG32_PTR_T)(0x3C200008)))
#define WHEEL0C      (*((REG32_PTR_T)(0x3C20000C)))
#define WHEEL10      (*((REG32_PTR_T)(0x3C200010)))
#define WHEELINT     (*((REG32_PTR_T)(0x3C200014)))
#define WHEELRX      (*((REG32_PTR_T)(0x3C200018)))
#define WHEELTX      (*((REG32_PTR_T)(0x3C20001C)))


/////I2S/////
#define I2SCLKGATE(i)   ((i) == 2 ? CLOCKGATE_I2S2 : \
                         (i) == 1 ? CLOCKGATE_I2S1 : \
                                    CLOCKGATE_I2S0)

#define I2SCLKCON           (*((REG32_PTR_T)(0x3CA00000)))
#define I2STXCON            (*((REG32_PTR_T)(0x3CA00004)))
#define I2STXCOM            (*((REG32_PTR_T)(0x3CA00008)))
#define I2STXDB0            (*((REG32_PTR_T)(0x3CA00010)))
#define I2SRXCON            (*((REG32_PTR_T)(0x3CA00030)))
#define I2SRXCOM            (*((REG32_PTR_T)(0x3CA00034)))
#define I2SRXDB             (*((REG32_PTR_T)(0x3CA00038)))
#define I2SSTATUS           (*((REG32_PTR_T)(0x3CA0003C)))
#define I2SCLKDIV           (*((REG32_PTR_T)(0x3CA00040)))


/////UART/////
/* s5l8702 UC87XX HW: 1 UARTC, 4 ports */
#define UARTC_BASE_ADDR     0x3CC00000
#define UARTC_N_PORTS       4
#define UARTC_PORT_OFFSET   0x4000


/////CLOCK GATES/////
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
