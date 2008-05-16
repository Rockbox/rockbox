/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by James Espinoza
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef AVIC_IMX31_H
#define AVIC_IMX31_H

struct avic_map
{
    volatile uint32_t intcntl;              /* 00h */
    volatile uint32_t nimask;               /* 04h */
    volatile uint32_t intennum;             /* 08h */
    volatile uint32_t intdisnum;            /* 0Ch */
    union                                   /* 10h */
    {
        struct
        {
            volatile uint32_t intenableh;   /* 10h */
            volatile uint32_t intenablel;   /* 14h */
        };
        volatile uint32_t intenable[2];     /* H,L */
    };
    union
    {
        struct
        {
            volatile uint32_t inttypeh;     /* 18h */
            volatile uint32_t inttypel;     /* 1Ch */
        };
        volatile uint32_t inttype[2];       /* H,L */
    };
    union
    {
        struct
        {
            volatile uint32_t nipriority7;  /* 20h */
            volatile uint32_t nipriority6;  /* 24h */
            volatile uint32_t nipriority5;  /* 28h */
            volatile uint32_t nipriority4;  /* 2Ch */
            volatile uint32_t nipriority3;  /* 30h */
            volatile uint32_t nipriority2;  /* 34h */
            volatile uint32_t nipriority1;  /* 38h */
            volatile uint32_t nipriority0;  /* 3Ch */
        };
        volatile uint32_t nipriority[8];    /* 7-0 */
    };
    volatile uint32_t nivecsr;              /* 40h */
    volatile uint32_t fivecsr;              /* 44h */
    union
    {
        struct
        {
            volatile uint32_t intsrch;      /* 48h */
            volatile uint32_t intsrcl;      /* 4Ch */
        };
        volatile uint32_t intsrc[2];        /* H,L */
    };
    union
    {
        struct
        {
            volatile uint32_t intfrch;      /* 50h */
            volatile uint32_t intfrcl;      /* 54h */
        };
        volatile uint32_t intfrc[2];        /* H,L */
    };
    union
    {
        struct
        {
            volatile uint32_t nipndh;       /* 58h */
            volatile uint32_t nipndl;       /* 5Ch */
        };
        volatile uint32_t nipnd[2];         /* H,L */
    };
    union
    {
        struct
        {
            volatile uint32_t fipndh;       /* 60h */
            volatile uint32_t fipndl;       /* 64h */
        };
        volatile uint32_t fipnd[2];         /* H,L */
    };
    volatile uint32_t skip1[0x26];          /* 68h */
    union                                   /* 100h */ 
    {
        struct
        {
            volatile uint32_t reserved0;
            volatile uint32_t reserved1;
            volatile uint32_t reserved2;
            volatile uint32_t i2c3;
            volatile uint32_t i2c2;
            volatile uint32_t mpeg4encoder;
            volatile uint32_t rtic;
            volatile uint32_t fir;
            volatile uint32_t mmc_sdhc2;
            volatile uint32_t mmc_sdhc1;
            volatile uint32_t i2c1;
            volatile uint32_t ssi2;
            volatile uint32_t ssi1;
            volatile uint32_t cspi2;
            volatile uint32_t cspi1;
            volatile uint32_t ata;
            volatile uint32_t mbx;
            volatile uint32_t cspi3;
            volatile uint32_t uart3;
            volatile uint32_t iim;
            volatile uint32_t sim1;
            volatile uint32_t sim2;
            volatile uint32_t rnga;
            volatile uint32_t evtmon;
            volatile uint32_t kpp;
            volatile uint32_t rtc;
            volatile uint32_t pwn;
            volatile uint32_t epit2;
            volatile uint32_t epit1;
            volatile uint32_t gpt;
            volatile uint32_t pwr_fail;
            volatile uint32_t ccm_dvfs;
            volatile uint32_t uart2;
            volatile uint32_t nandfc;
            volatile uint32_t sdma;
            volatile uint32_t usb_host1;
            volatile uint32_t usb_host2;
            volatile uint32_t usb_otg;
            volatile uint32_t reserved3;
            volatile uint32_t mshc1;
            volatile uint32_t mshc2;
            volatile uint32_t ipu_err;
            volatile uint32_t ipu;
            volatile uint32_t reserved4;
            volatile uint32_t reserved5;
            volatile uint32_t uart1;
            volatile uint32_t uart4;
            volatile uint32_t uart5;
            volatile uint32_t etc_irq;
            volatile uint32_t scc_scm;
            volatile uint32_t scc_smn;
            volatile uint32_t gpio2;
            volatile uint32_t gpio1;
            volatile uint32_t ccm_clk;
            volatile uint32_t pcmcia;
            volatile uint32_t wdog;
            volatile uint32_t gpio3;
            volatile uint32_t reserved6;
            volatile uint32_t ext_pwmg;
            volatile uint32_t ext_temp;
            volatile uint32_t ext_sense1;
            volatile uint32_t ext_sense2;
            volatile uint32_t ext_wdog;
            volatile uint32_t ext_tv;
        };
        volatile uint32_t vector[0x40];     /* 100h */
    };
};

enum INT_TYPE
{
    IRQ = 0,
    FIQ
};

enum IMX31_INT_LIST
{
    __IMX31_INT_FIRST = -1,
   RESERVED0, RESERVED1,     RESERVED2, I2C3,
   I2C2,      MPEG4_ENCODER, RTIC,      FIR,
   MMC_SDHC2, MMC_SDHC1,     I2C1,      SSI2,
   SSI1,      CSPI2,         CSPI1,     ATA,
   MBX,       CSPI3,         UART3,     IIM,
   SIM1,      SIM2,          RNGA,      EVTMON,
   KPP,       RTC,           PWN,       EPIT2,
   EPIT1,     GPT,           PWR_FAIL,  CCM_DVFS,
   UART2,     NANDFC,        SDMA,      USB_HOST1,
   USB_HOST2, USB_OTG,       RESERVED3, MSHC1,
   MSHC2,     IPU_ERR,       IPU,       RESERVED4,
   RESERVED5, UART1,         UART4,     UART5,
   ETC_IRQ,   SCC_SCM,       SCC_SMN,   GPIO2,
   GPIO1,     CCM_CLK,       PCMCIA,    WDOG,
   GPIO3,     RESERVED6,     EXT_PWMG,  EXT_TEMP,
   EXT_SENS1, EXT_SENS2,     EXT_WDOG,  EXT_TV,
   ALL
};

void avic_init(void);
void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
                     unsigned long ni_priority, void (*handler)(void));
void avic_set_int_priority(enum IMX31_INT_LIST ints,
                           unsigned long ni_priority);
void avic_disable_int(enum IMX31_INT_LIST ints);
void avic_set_int_type(enum IMX31_INT_LIST ints, enum INT_TYPE intstype);

#endif /* AVIC_IMX31_H */
