/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Tomasz Malesinski
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

#ifndef __PNX0101_H__
#define __PNX0101_H__

#define GPIO0_READ (*(volatile unsigned long *)0x80003000)
#define GPIO0_SET  (*(volatile unsigned long *)0x80003014)
#define GPIO0_CLR  (*(volatile unsigned long *)0x80003018)
#define GPIO1_READ (*(volatile unsigned long *)0x80003040)
#define GPIO1_SET  (*(volatile unsigned long *)0x80003054)
#define GPIO1_CLR  (*(volatile unsigned long *)0x80003058)
#define GPIO2_READ (*(volatile unsigned long *)0x80003080)
#define GPIO2_SET  (*(volatile unsigned long *)0x80003094)
#define GPIO2_CLR  (*(volatile unsigned long *)0x80003098)
#define GPIO3_READ (*(volatile unsigned long *)0x800030c0)
#define GPIO3_SET  (*(volatile unsigned long *)0x800030d4)
#define GPIO3_CLR  (*(volatile unsigned long *)0x800030d8)
#define GPIO4_READ (*(volatile unsigned long *)0x80003100)
#define GPIO4_SET  (*(volatile unsigned long *)0x80003114)
#define GPIO4_CLR  (*(volatile unsigned long *)0x80003118)
#define GPIO5_READ (*(volatile unsigned long *)0x80003140)
#define GPIO5_SET  (*(volatile unsigned long *)0x80003154)
#define GPIO5_CLR  (*(volatile unsigned long *)0x80003158)
#define GPIO6_READ (*(volatile unsigned long *)0x80003180)
#define GPIO6_SET  (*(volatile unsigned long *)0x80003194)
#define GPIO6_CLR  (*(volatile unsigned long *)0x80003198)
#define GPIO7_READ (*(volatile unsigned long *)0x800031c0)
#define GPIO7_SET  (*(volatile unsigned long *)0x800031d4)
#define GPIO7_CLR  (*(volatile unsigned long *)0x800031d8)

#define LCDREG04 (*(volatile unsigned long *)0x80104004)
#define LCDSTAT  (*(volatile unsigned long *)0x80104008)
#define LCDREG10 (*(volatile unsigned long *)0x80104010)
#define LCDCMD   (*(volatile unsigned long *)0x80104020)
#define LCDDATA  (*(volatile unsigned long *)0x80104030)

#define TIMERR00 (*(volatile unsigned long *)0x80020000)
#define TIMERR08 (*(volatile unsigned long *)0x80020008)
#define TIMERR0C (*(volatile unsigned long *)0x8002000c)

#define ADCCH0   (*(volatile unsigned long *)0x80002400)
#define ADCCH1   (*(volatile unsigned long *)0x80002404)
#define ADCCH2   (*(volatile unsigned long *)0x80002408)
#define ADCCH3   (*(volatile unsigned long *)0x8000240c)
#define ADCCH4   (*(volatile unsigned long *)0x80002410)
#define ADCST    (*(volatile unsigned long *)0x80002420)
#define ADCR24   (*(volatile unsigned long *)0x80002424)
#define ADCR28   (*(volatile unsigned long *)0x80002428)

#define DMAINTSTAT   (*(volatile unsigned long *)0x80104c04)
#define DMAINTEN     (*(volatile unsigned long *)0x80104c08)

#define DMASRC(n)    (*(volatile unsigned long *)(0x80104800 + (n) * 0x20))
#define DMADEST(n)   (*(volatile unsigned long *)(0x80104804 + (n) * 0x20))
#define DMALEN(n)    (*(volatile unsigned long *)(0x80104808 + (n) * 0x20))
#define DMAR0C(n)    (*(volatile unsigned long *)(0x8010480c + (n) * 0x20))
#define DMAR10(n)    (*(volatile unsigned long *)(0x80104810 + (n) * 0x20))
#define DMAR1C(n)    (*(volatile unsigned long *)(0x8010481c + (n) * 0x20))

#define MMUBLOCK(n)  (*(volatile unsigned long *)(0x80105018 + (n) * 4))

#define CODECVOL     (*(volatile unsigned long *)0x80200398)

#ifndef ASM

/* Clock generation unit */

struct pnx0101_cgu {
    unsigned long base_scr[12];
    unsigned long base_fs1[12];
    unsigned long base_fs2[12];
    unsigned long base_ssr[12];
    unsigned long clk_pcr[73];
    unsigned long clk_psr[73];
    unsigned long clk_esr[67];
    unsigned long base_bcr[3];
    unsigned long base_fdc[18];
};

#define CGU (*(volatile struct pnx0101_cgu *)0x80004000)

#define PNX0101_SEL_STAGE_SYS  0
#define PNX0101_SEL_STAGE_APB0 1
#define PNX0101_SEL_STAGE_APB1 2
#define PNX0101_SEL_STAGE_APB3 3
#define PNX0101_SEL_STAGE_DAIO 9

#define PNX0101_HIPREC_FDC 16

#define PNX0101_FIRST_DIV_SYS  0
#define PNX0101_N_DIV_SYS      7
#define PNX0101_FIRST_DIV_APB0 7
#define PNX0101_N_DIV_APB0     2
#define PNX0101_FIRST_DIV_APB1 9
#define PNX0101_N_DIV_APB1     1
#define PNX0101_FIRST_DIV_APB3 10
#define PNX0101_N_DIV_APB3     1
#define PNX0101_FIRST_DIV_DAIO 12
#define PNX0101_N_DIV_DAIO     6

#define PNX0101_BCR_SYS  0
#define PNX0101_BCR_APB0 1
#define PNX0101_BCR_DAIO 2

#define PNX0101_FIRST_ESR_SYS  0
#define PNX0101_N_ESR_SYS      28
#define PNX0101_FIRST_ESR_APB0 28
#define PNX0101_N_ESR_APB0     9
#define PNX0101_FIRST_ESR_APB1 37
#define PNX0101_N_ESR_APB1     4
#define PNX0101_FIRST_ESR_APB3 41
#define PNX0101_N_ESR_APB3     16
#define PNX0101_FIRST_ESR_DAIO 58
#define PNX0101_N_ESR_DAIO     9

#define PNX0101_ESR_APB1 0x25
#define PNX0101_ESR_T0   0x26
#define PNX0101_ESR_T1   0x27
#define PNX0101_ESR_I2C  0x28

#define PNX0101_CLOCK_APB1 0x25
#define PNX0101_CLOCK_T0   0x26
#define PNX0101_CLOCK_T1   0x27
#define PNX0101_CLOCK_I2C  0x28

#define PNX0101_MAIN_CLOCK_FAST     1
#define PNX0101_MAIN_CLOCK_MAIN_PLL 9

struct pnx0101_pll {
    unsigned long hpfin;
    unsigned long hpmdec;
    unsigned long hpndec;
    unsigned long hppdec;
    unsigned long hpmode;
    unsigned long hpstat;
    unsigned long hpack;
    unsigned long hpreq;
    unsigned long hppad1;
    unsigned long hppad2;
    unsigned long hppad3;
    unsigned long hpselr;
    unsigned long hpseli;
    unsigned long hpselp;
    unsigned long lpfin;
    unsigned long lppdn;
    unsigned long lpmbyp;
    unsigned long lplock;
    unsigned long lpdbyp;
    unsigned long lpmsel;
    unsigned long lppsel;
};

#define PLL (*(volatile struct pnx0101_pll *)0x80004cac)

struct pnx0101_emc {
    unsigned long control;
    unsigned long status;
};

#define EMC (*(volatile struct pnx0101_emc *)0x80008000)

struct pnx0101_emcstatic {
    unsigned long config;
    unsigned long waitwen;
    unsigned long waitoen;
    unsigned long waitrd;
    unsigned long waitpage;
    unsigned long waitwr;
    unsigned long waitturn;
};

#define EMCSTATIC0 (*(volatile struct pnx0101_emcstatic *)0x80008200)
#define EMCSTATIC1 (*(volatile struct pnx0101_emcstatic *)0x80008220)
#define EMCSTATIC2 (*(volatile struct pnx0101_emcstatic *)0x80008240)

/* Timers */

struct pnx0101_timer {
    unsigned long load;
    unsigned long value;
    unsigned long ctrl;
    unsigned long clr;
};

#define TIMER0 (*(volatile struct pnx0101_timer *)0x80020000)
#define TIMER1 (*(volatile struct pnx0101_timer *)0x80020400)

/* Interrupt controller */

#define IRQ_TIMER0 5
#define IRQ_TIMER1 6
#define IRQ_DMA    28

#define INTPRIOMASK ((volatile unsigned long *)0x80300000)
#define INTVECTOR   ((volatile unsigned long *)0x80300100)
#define INTPENDING  (*(volatile unsigned long *)0x80300200)
#define INTFEATURES (*(volatile unsigned long *)0x80300300)
#define INTREQ      ((volatile unsigned long *)0x80300400)

#define INTREQ_WEPRIO   0x10000000
#define INTREQ_WETARGET 0x08000000
#define INTREQ_WEENABLE 0x04000000
#define INTREQ_WEACTVLO 0x02000000
#define INTREQ_ENABLE   0x00010000

/* General purpose DMA */

struct pnx0101_dma_channel {
    unsigned long source;
    unsigned long dest;
    unsigned long length;
    unsigned long config;
    unsigned long enable;
    unsigned long pad1;
    unsigned long pad2;
    unsigned long count;
};

#define DMACHANNEL ((volatile struct pnx0101_dma_channel *)0x80104800)

struct pnx0101_dma {
    unsigned long enable;
    unsigned long stat;
    unsigned long irqmask;
    unsigned long softint;
};

#define DMA (*(volatile struct pnx0101_dma *)0x80104c00)

struct pnx0101_audio {
    unsigned long pad1;
    unsigned long siocr;
    unsigned long pad2;
    unsigned long pad3;
    unsigned long pad4;
    unsigned long pad5;
    unsigned long ddacctrl;
    unsigned long ddacstat;
    unsigned long ddacset;
};

#define AUDIO (*(volatile struct pnx0101_audio *)0x80200380)
 
#endif /* ASM */

#endif
