/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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
#ifndef __X1000_H__
#define __X1000_H__

#include "stdint.h"

#define REG32(addr) (*(volatile uint32_t *)(addr))

/* CPM */
#define REG_CPM_CCR         REG32(0xb0000000)
#define CPM_CCR_SEL_SRC_EXCLK   (1u << 30)
#define CPM_CCR_SEL_SRC_APLL    (2u << 30)
#define CPM_CCR_SEL_CPLL_SCLK_A (1 << 28)
#define CPM_CCR_SEL_H0PLL_SCLK_A (1 << 26)
#define CPM_CCR_SEL_H0PLL_MPLL  (2 << 26)
#define CPM_CCR_SEL_H2PLL_SCLK_A (1 << 24)
#define CPM_CCR_SEL_H2PLL_MPLL  (2 << 24)
#define CPM_CCR_MUX_MASK    0xff000000
#define CPM_CCR_CE_CPU      (1 << 22)
#define CPM_CCR_CE_AHB0     (1 << 21)
#define CPM_CCR_CE_AHB2     (1 << 20)
#define CPM_CCR_CE_MASK     (7 << 20)
#define CPM_CCR_PDIV(x)     (((x) - 1) << 16)
#define CPM_CCR_H2DIV(x)    (((x) - 1) << 12)
#define CPM_CCR_H0DIV(x)    (((x) - 1) << 8)
#define CPM_CCR_L2DIV(x)    (((x) - 1) << 4)
#define CPM_CCR_CDIV(x)     (((x) - 1) << 0)
#define CPM_CCR_DIV_MASK    0xfffff
#define REG_CPM_CSR         REG32(0xb00000d4)
#define CPM_CSR_MUX_MASK    (0xfu << 28)
#define CPM_CSR_DIV_BUSY_MASK 7
#define REG_CPM_APCR        REG32(0xb0000010)
#define CPM_APCR_BS         (1u << 31)
#define CPM_APCR_PLLM(x)    ((x) << 24)
#define CPM_APCR_ON         (1 << 10)
#define CPM_APCR_ENABLE     (1 << 8)
#define REG_CPM_CLKGR       REG32(0xb0000020)
#define CPM_CLKGR_OST       (1 << 20)
#define CPM_CLKGR_OTG       (1 << 3)
#define REG_CPM_OPCR        REG32(0xb0000024)
#define CPM_OPCR_GATE_USBPHY_CLK (1 << 23)
#define CPM_OPCR_SPENDN0    (1 << 7)
#define REG_CPM_USBPCR      REG32(0xb000003c)
#define CPM_USBPCR_COMMONONN (1 << 25)
#define CPM_USBPCR_VBUSVLDEXT (1 << 24)
#define CPM_USBPCR_VBUSVLDEXTSEL (1 << 23)
#define CPM_USBPCR_POR      (1 << 22)
#define CPM_USBPCR_OTG_DISABLE (1 << 20)
#define CPM_USBPCR_SQRXTUNE(x) ((x) << 11)
#define CPM_USBPCR_TXPREEMPHTUNE (1 << 6)
#define CPM_USBPCR_TXHSXVTUNE(x) ((x) << 4)
#define CPM_USBPCR_TXVREFTUNE(x) ((x) << 0)
#define REG_CPM_USBRDT      REG32(0xb0000040)
#define CPM_USBRDT_VBFIL_LD_EN (1 << 25)
#define CPM_USBRDT_RDT(x)   ((x) << 0)
#define REG_CPM_USBVBFIL    REG32(0xb0000044)
#define REG_CPM_USBPCR1     REG32(0xb0000048)
#define CPM_USBPCR1_BVLD_REG (1u << 31)
#define CPM_USBPCR1_REFCLK_SEL_CLKCORE (2 << 26)
#define CPM_USBPCR1_REFCLK_DIV_24MHZ (1 << 24)
#define CPM_USBPCR1_WORD_IF_16BIT (1 << 19)
#define REG_CPM_USBCDR      REG32(0xb0000050)
#define CPM_USBCDR_CLKSRC_MASK (3u << 30)
#define CPM_USBCDR_CE       (1 << 29)
#define CPM_USBCDR_BUSY     (1 << 28)
#define CPM_USBCDR_STOP     (1 << 27)
#define CPM_USBCDR_PHY_GATE (1 << 26)
#define CPM_USBCDR_CLKDIV_MASK 0xff
#define REG_CPM_SRBC        REG32(0xb00000c4)
#define CPM_SRBC_OTG_SR     (1 << 12)

/* OST: channel 2 is a free running 64-bit counter */
#define REG_OST_CTRL        REG32(0xb2000000)
#define OST_CTRL_PRESCALE2_MASK (3 << 2)
#define OST_CTRL_PRESCALE2_BY_4 (1 << 2)
#define REG_OST_ENABLE_SET  REG32(0xb2000034)
#define REG_OST_ENABLE_CLR  REG32(0xb2000038)
#define REG_OST_CLEAR       REG32(0xb2000008)
#define OST_OST2            (1 << 1)
#define REG_OST_2CNTH       REG32(0xb200001c)
#define REG_OST_2CNTL       REG32(0xb2000020)

/* GPIO: port n, the SET/CLR registers are at +4/+8 */
#define REG_GPIO_INT_CLR(n) REG32(0xb0010018 + (n) * 0x100)
#define REG_GPIO_MSK_SET(n) REG32(0xb0010024 + (n) * 0x100)
#define REG_GPIO_PAT1_CLR(n) REG32(0xb0010038 + (n) * 0x100)
#define REG_GPIO_PAT0_SET(n) REG32(0xb0010044 + (n) * 0x100)
#define GPIO_PORT_C         2

/* the OST counts EXCLK (24 MHz) / 4 */
#define OST_TICKS_PER_US    6

/* USB OTG core (Synopsys DWC2) */
#define OTG_BASE            0xb3500000
#define REG_DWC_GAHBCFG     REG32(OTG_BASE + 0x008)
#define DWC_GAHBCFG_DMAEN   (1 << 5)
#define DWC_GAHBCFG_HBSTLEN_INCR16 (7 << 1)
#define REG_DWC_GUSBCFG     REG32(OTG_BASE + 0x00c)
#define DWC_GUSBCFG_TRDT(x) ((x) << 10)
#define DWC_GUSBCFG_PHYIF16 (1 << 3)
#define REG_DWC_GRSTCTL     REG32(OTG_BASE + 0x010)
#define DWC_GRSTCTL_AHBIDL  (1 << 31)
#define DWC_GRSTCTL_TXFNUM(x) ((x) << 6)
#define DWC_GRSTCTL_TXFFLSH (1 << 5)
#define DWC_GRSTCTL_RXFFLSH (1 << 4)
#define DWC_GRSTCTL_CSRST   (1 << 0)
#define REG_DWC_GINTSTS     REG32(OTG_BASE + 0x014)
#define DWC_GINTSTS_ENUMDNE (1 << 13)
#define DWC_GINTSTS_USBRST  (1 << 12)
#define REG_DWC_GINTMSK     REG32(OTG_BASE + 0x018)
#define REG_DWC_GRXFSIZ     REG32(OTG_BASE + 0x024)
#define REG_DWC_TX0FSIZ     REG32(OTG_BASE + 0x028)
#define REG_DWC_DCFG        REG32(OTG_BASE + 0x800)
#define DWC_DCFG_DAD_MASK   (0x7f << 4)
#define DWC_DCFG_DAD(x)     ((x) << 4)
#define DWC_DCFG_NZLSOHSK   (1 << 2)
#define DWC_DCFG_DSPD_HS    0
#define REG_DWC_DCTL        REG32(OTG_BASE + 0x804)
#define DWC_DCTL_SDIS       (1 << 1)
#define REG_DWC_DIEPCTL0    REG32(OTG_BASE + 0x900)
#define REG_DWC_DIEPINT0    REG32(OTG_BASE + 0x908)
#define REG_DWC_DIEPTSIZ0   REG32(OTG_BASE + 0x910)
#define REG_DWC_DIEPDMA0    REG32(OTG_BASE + 0x914)
#define REG_DWC_DOEPCTL0    REG32(OTG_BASE + 0xb00)
#define REG_DWC_DOEPINT0    REG32(OTG_BASE + 0xb08)
#define REG_DWC_DOEPTSIZ0   REG32(OTG_BASE + 0xb10)
#define REG_DWC_DOEPDMA0    REG32(OTG_BASE + 0xb14)
#define DWC_EPCTL_EPENA     (1 << 31)
#define DWC_EPCTL_SNAK      (1 << 27)
#define DWC_EPCTL_CNAK      (1 << 26)
#define DWC_EPCTL_STALL     (1 << 21)
#define DWC_EPINT_SETUPRECVD (1 << 15)
#define DWC_EPINT_STUP      (1 << 3)
#define DWC_EPINT_XFRC      (1 << 0)
#define DWC_EPTSIZ_STUPCNT(x) ((x) << 29)
#define DWC_EPTSIZ_PKTCNT(x) ((x) << 19)
#define DWC_EPTSIZ_XFERSIZE(x) ((x) << 0)
#define DWC_EPTSIZ0_XFERSIZE_MASK 0x7f
#define REG_DWC_PCGCCTL     REG32(OTG_BASE + 0xe00)

/* kseg0 address to physical (for DMA) and to uncached kseg1 */
#define PHYSADDR(x)         ((uint32_t)(x) & 0x1fffffff)
#define UNCACHEDADDR(x)     ((void *)(PHYSADDR(x) | 0xa0000000))

#endif /* __X1000_H__ */
