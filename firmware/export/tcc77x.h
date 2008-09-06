/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#ifndef __TCC77X_H__
#define __TCC77X_H__

/* General-purpose IO */

#define GPIOA      (*(volatile unsigned long *)0x80000300)
#define GPIOB      (*(volatile unsigned long *)0x80000310)
#define GPIOC      (*(volatile unsigned long *)0x80000320)
#define GPIOD      (*(volatile unsigned long *)0x80000330)
#define GPIOE      (*(volatile unsigned long *)0x80000340)

#define GPIOA_DIR  (*(volatile unsigned long *)0x80000304)
#define GPIOB_DIR  (*(volatile unsigned long *)0x80000314)
#define GPIOC_DIR  (*(volatile unsigned long *)0x80000324)
#define GPIOD_DIR  (*(volatile unsigned long *)0x80000334)
#define GPIOE_DIR  (*(volatile unsigned long *)0x80000344)

#define GPIOA_FUNC (*(volatile unsigned long *)0x80000308)
#define GPIOB_FUNC (*(volatile unsigned long *)0x80000318)
#define GPIOC_FUNC (*(volatile unsigned long *)0x80000328)
#define GPIOD_FUNC (*(volatile unsigned long *)0x80000338)
#define GPIOE_FUNC (*(volatile unsigned long *)0x80000348)

#define BMI        (*(volatile unsigned long *)0x80000364)

/* Clock Generator */

#define CLKCTRL    (*(volatile unsigned long *)0x80000400)
#define PLL0CFG    (*(volatile unsigned long *)0x80000404)
#define CLKDIVC    (*(volatile unsigned long *)0x8000040c)
#define MODECTR    (*(volatile unsigned long *)0x80000410)
#define BCLKCTR    (*(volatile unsigned long *)0x80000414)
#define SWRESET    (*(volatile unsigned long *)0x80000418)
#define PCLKCFG0   (*(volatile unsigned long *)0x8000041c)
#define PCLKCFG1   (*(volatile unsigned long *)0x80000420)
#define PCLKCFG2   (*(volatile unsigned long *)0x80000424)
#define PCLKCFG3   (*(volatile unsigned long *)0x80000428)
#define PCLKCFG4   (*(volatile unsigned long *)0x8000042c)
#define PCLKCFG5   (*(volatile unsigned long *)0x80000430)
#define PCLKCFG6   (*(volatile unsigned long *)0x80000434)

#define PCLK_DAI   PCLKCFG6

/* Device bits for SWRESET & BCLKCTR */

#define DEV_DAI  (1<<0)
#define DEV_USBD (1<<4)
#define DEV_ECC  (1<<9)
#define DEV_NAND (1<<16)

/* ADC */

#define ADCCON     (*(volatile unsigned long *)0x80000a00)
#define ADCDATA    (*(volatile unsigned long *)0x80000a04)
#define ADCCONA    (*(volatile unsigned long *)0x80000a80)
#define ADCSTATUS  (*(volatile unsigned long *)0x80000a84)
#define ADCCFG     (*(volatile unsigned long *)0x80000a88)


/* Memory Controller */
#define SDCFG      (*(volatile unsigned long *)0xf0000000)
#define SDFSM      (*(volatile unsigned long *)0xf0000004)
#define MCFG       (*(volatile unsigned long *)0xf0000008)
#define TST        (*(volatile unsigned long *)0xf000000c)
#define CSCFG0     (*(volatile unsigned long *)0xf0000010)
#define CSCFG1     (*(volatile unsigned long *)0xf0000014)
#define CSCFG2     (*(volatile unsigned long *)0xf0000018)
#define CSCFG3     (*(volatile unsigned long *)0xf000001c)
#define CLKCFG     (*(volatile unsigned long *)0xf0000020)
#define SDCMD      (*(volatile unsigned long *)0xf0000024)


/* IRQ Controller */
#define EXT0_IRQ_MASK    (1<<0)
#define EXT1_IRQ_MASK    (1<<1)
#define EXT2_IRQ_MASK    (1<<2)
#define EXT3_IRQ_MASK    (1<<3)
#define I2SR_IRQ_MASK    (1<<4)
#define I2ST_IRQ_MASK    (1<<5)
#define TIMER0_IRQ_MASK  (1<<6)
#define USBD_IRQ_MASK    (1<<8)  /* USB 2.0 device */
#define USBH_IRQ_MASK    (1<<10) /* USB 1.1 host */
#define ADC_IRQ_MASK     (1<<16)
#define USB_DMA_IRQ_MASK (1<<26) /* USB DMA */
#define ECC_IRQ_MASK     (1<<27)

#define DAI_RX_IRQ_MASK  I2SR_IRQ_MASK
#define DAI_TX_IRQ_MASK  I2ST_IRQ_MASK

#define USB_DMA_IRQ_MASK (1<<26) /* USB DMA */

#define IEN      (*(volatile unsigned long *)0x80000100)
#define CREQ     (*(volatile unsigned long *)0x80000104)
#define IREQ     (*(volatile unsigned long *)0x80000108)
#define IRQSEL   (*(volatile unsigned long *)0x8000010c)
#define ICFG     (*(volatile unsigned long *)0x80000110)
#define MREQ     (*(volatile unsigned long *)0x80000114)
#define TSTREQ   (*(volatile unsigned long *)0x80000118)
#define IRQ      (*(volatile unsigned long *)0x80000120)
#define FIQ      (*(volatile unsigned long *)0x80000124)
#define MIRQ     (*(volatile unsigned long *)0x80000128)
#define MFIQ     (*(volatile unsigned long *)0x8000012c)
#define TMODE    (*(volatile unsigned long *)0x80000130)
#define SYNC     (*(volatile unsigned long *)0x80000134)
#define WKUP     (*(volatile unsigned long *)0x80000138)

/* Timer Controller */

#define TCFG0    (*(volatile unsigned long *)0x80000200)
#define TCNT0    (*(volatile unsigned long *)0x80000204)
#define TREF0    (*(volatile unsigned long *)0x80000208)
#define TMREF0   (*(volatile unsigned long *)0x8000020c)
#define TCFG1    (*(volatile unsigned long *)0x80000210)
#define TCNT1    (*(volatile unsigned long *)0x80000214)
#define TREF1    (*(volatile unsigned long *)0x80000218)
#define TMREF1   (*(volatile unsigned long *)0x8000021c)
#define TCFG2    (*(volatile unsigned long *)0x80000220)
#define TCNT2    (*(volatile unsigned long *)0x80000224)
#define TREF2    (*(volatile unsigned long *)0x80000228)
#define TMREF2   (*(volatile unsigned long *)0x8000022c)
#define TCFG3    (*(volatile unsigned long *)0x80000230)
#define TCNT3    (*(volatile unsigned long *)0x80000234)
#define TREF3    (*(volatile unsigned long *)0x80000238)
#define TMREF3   (*(volatile unsigned long *)0x8000023c)
#define TCFG4    (*(volatile unsigned long *)0x80000240)
#define TCNT4    (*(volatile unsigned long *)0x80000244)
#define TREF4    (*(volatile unsigned long *)0x80000248)
#define TCFG5    (*(volatile unsigned long *)0x80000250)
#define TCNT5    (*(volatile unsigned long *)0x80000254)
#define TREF5    (*(volatile unsigned long *)0x80000258)
#define TIREQ    (*(volatile unsigned long *)0x80000260)
#define TWDCFG   (*(volatile unsigned long *)0x80000270)
#define TWDCLR   (*(volatile unsigned long *)0x80000274)
#define TC32EN   (*(volatile unsigned long *)0x80000280)
#define TC32LDV  (*(volatile unsigned long *)0x80000284)
#define TC32CMP0 (*(volatile unsigned long *)0x80000288)
#define TC32CMP1 (*(volatile unsigned long *)0x8000028c)
#define TC32PCNT (*(volatile unsigned long *)0x80000290)
#define TC32MCNT (*(volatile unsigned long *)0x80000294)
#define TC32IRQ  (*(volatile unsigned long *)0x80000298)

/* TIREQ flags */
#define TF0 (1<<8) /* Timer 0 reference value reached */
#define TF1 (1<<9) /* Timer 1 reference value reached */
#define TI0 (1<<0) /* Timer 0 IRQ flag */
#define TI1 (1<<1) /* Timer 1 IRQ flag */

/* NAND Flash Controller */

#define NFC_CMD    (*(volatile unsigned long *)0x90000000)
#define NFC_SADDR  (*(volatile unsigned long *)0x9000000C)
#define NFC_SDATA  (*(volatile unsigned long *)0x90000040)
#define NFC_WDATA  (*(volatile unsigned long *)0x90000010)
#define NFC_CTRL   (*(volatile unsigned long *)0x90000050)
    #define NFC_16BIT (1<<26)
    #define NFC_CS0   (1<<23)
    #define NFC_CS1   (1<<22)
    #define NFC_READY (1<<20)
#define NFC_IREQ   (*(volatile unsigned long *)0x90000060)
#define NFC_RST    (*(volatile unsigned long *)0x90000064)


/* ECC controller */

#define ECC_CTRL   (*(volatile unsigned long *)0x80000900)
    #define ECC_DMA_REQ (1<<28)
    #define ECC_ENC     (1<<27) /* MLC ECC3/4 */
    #define ECC_FLG     (1<<26)
    #define ECC_IEN     (1<<25)
    #define ECC_MANUAL  (1<<22)
    #define ECC_WCNT    (1<<12) /* [21:12] */
    #define ECC_HOLD    (1<<7)
    #define ECC_M4EN    (1<<6)
    #define ECC_ZERO    (1<<5)
    #define ECC_M3EN    (1<<4)
    #define ECC_CNT_MASK (7<<1)
    #define ECC_CNT     (1<<1)
    #define ECC_SLC     (1<<0)
    
#define ECC_BASE   (*(volatile unsigned long *)0x80000904)
#define ECC_MASK   (*(volatile unsigned long *)0x80000908)
#define ECC_CLR    (*(volatile unsigned long *)0x8000090c)
#define SLC_ECC0   (*(volatile unsigned long *)0x80000910)
#define SLC_ECC1   (*(volatile unsigned long *)0x80000914)
#define SLC_ECC2   (*(volatile unsigned long *)0x80000918)
#define SLC_ECC3   (*(volatile unsigned long *)0x8000091c)
#define SLC_ECC4   (*(volatile unsigned long *)0x80000920)
#define SLC_ECC5   (*(volatile unsigned long *)0x80000924)
#define SLC_ECC6   (*(volatile unsigned long *)0x80000928)
#define SLC_ECC7   (*(volatile unsigned long *)0x8000092c)
#define MLC_ECC0W  (*(volatile unsigned long *)0x80000930)
#define MLC_ECC1W  (*(volatile unsigned long *)0x80000934)
#define MLC_ECC2W  (*(volatile unsigned long *)0x80000938)
#define MLC_ECC0R  (*(volatile unsigned long *)0x80000940)
#define MLC_ECC1R  (*(volatile unsigned long *)0x80000944)
#define MLC_ECC2R  (*(volatile unsigned long *)0x80000948)
#define ECC_CORR_START (*(volatile unsigned long *)0x8000094c)
#define ECC_ERRADDR1 (*(volatile unsigned long *)0x80000950)
#define ECC_ERRADDR2 (*(volatile unsigned long *)0x80000954)
#define ECC_ERRADDR3 (*(volatile unsigned long *)0x80000958)
#define ECC_ERRADDR4 (*(volatile unsigned long *)0x8000095c)
#define ECC_ERRDATA1 (*(volatile unsigned long *)0x80000960)
#define ECC_ERRDATA2 (*(volatile unsigned long *)0x80000964)
#define ECC_ERRDATA3 (*(volatile unsigned long *)0x80000968)
#define ECC_ERRDATA4 (*(volatile unsigned long *)0x8000096c)
#define ECC_ERR_NUM  (*(volatile unsigned long *)0x80000970)

#define ECC_ERRDATA(x) (*(volatile unsigned long *)(0x80000960 + (x) * 4))
#define ECC_ERRADDR(x) (*(volatile unsigned long *)(0x80000950 + (x) * 4))

/* Digital Audio Interface */
#define DADI_L0    (*(volatile unsigned long *)0x80000000)
#define DADI_R0    (*(volatile unsigned long *)0x80000004)
#define DADI_L1    (*(volatile unsigned long *)0x80000008)
#define DADI_R1    (*(volatile unsigned long *)0x8000000C)
#define DADI_L2    (*(volatile unsigned long *)0x80000010)
#define DADI_R2    (*(volatile unsigned long *)0x80000014)
#define DADI_L3    (*(volatile unsigned long *)0x80000018)
#define DADI_R3    (*(volatile unsigned long *)0x8000001c)

#define DADO_L0    (*(volatile unsigned long *)0x80000020)
#define DADO_R0    (*(volatile unsigned long *)0x80000024)
#define DADO_L1    (*(volatile unsigned long *)0x80000028)
#define DADO_R1    (*(volatile unsigned long *)0x8000002C)
#define DADO_L2    (*(volatile unsigned long *)0x80000030)
#define DADO_R2    (*(volatile unsigned long *)0x80000034)
#define DADO_L3    (*(volatile unsigned long *)0x80000038)
#define DADO_R3    (*(volatile unsigned long *)0x8000003c)

#define DAMR       (*(volatile unsigned long *)0x80000040)
#define DAVC       (*(volatile unsigned long *)0x80000044)

#define DADI_L(x)  (*(volatile unsigned long *)(0x80000000 + (x) * 8))
#define DADI_R(x)  (*(volatile unsigned long *)(0x80000004 + (x) * 8))
#define DADO_L(x)  (*(volatile unsigned long *)(0x80000020 + (x) * 8))
#define DADO_R(x)  (*(volatile unsigned long *)(0x80000024 + (x) * 8))

/* USB 2.0 device system MMR base address */
#define USB_BASE 0x90000b00

#endif
