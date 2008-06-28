/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 * Copyright (C) 2008 by Maurus Cuelenaere
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

/** All register offset definitions for the TI DM320
 * Taken from: http://svn.neurostechnology.com/filedetails.php?repname=neuros-bsp&path=%2Ftrunk%2Fkernels%2Flinux-2.6.15%2Finclude%2Fasm-arm%2Farch-ntosd-dm320%2Fio_registers.h&rev=0&sc=0
 */

#ifndef __DM320_H__
#define __DM320_H__

#define LCD_BUFFER_SIZE  (LCD_WIDTH*LCD_HEIGHT*2)
#define TTB_SIZE         (0x4000)
/* must be 16Kb (0x4000) aligned */
#if 0
#define MEM_END          0x00900000 + (MEM*0x00100000)
#define TTB_BASE         ((unsigned int *)(MEM_END - TTB_SIZE)) /* End of memory */
#else
#define TTB_BASE         ((unsigned int *)(0x04900000 - TTB_SIZE)) /* End of memory */
#endif
#define FRAME            ((short *) ((char*)TTB_BASE - LCD_BUFFER_SIZE))  /* Right before TTB */

#define PHY_IO_BASE      0x00030000
#define DM320_REG(addr)  (*(volatile unsigned short *)(PHY_IO_BASE + (addr)))
#define PHY_IO_BASE2     0x00060000
#define DM320_REG2(addr) (*(volatile unsigned int *)(PHY_IO_BASE2 + (addr)))

/* Timer 0-3 */
#define IO_TIMER0_TMMD            DM320_REG(0x0000)
#define IO_TIMER0_TMRSV0          DM320_REG(0x0002)
#define IO_TIMER0_TMPRSCL         DM320_REG(0x0004)
#define IO_TIMER0_TMDIV           DM320_REG(0x0006)
#define IO_TIMER0_TMTRG           DM320_REG(0x0008)
#define IO_TIMER0_TMCNT           DM320_REG(0x000A)

#define IO_TIMER1_TMMD            DM320_REG(0x0080)
#define IO_TIMER1_TMRSV0          DM320_REG(0x0082)
#define IO_TIMER1_TMPRSCL         DM320_REG(0x0084)
#define IO_TIMER1_TMDIV           DM320_REG(0x0086)
#define IO_TIMER1_TMTRG           DM320_REG(0x0088)
#define IO_TIMER1_TMCNT           DM320_REG(0x008A)

#define IO_TIMER2_TMMD            DM320_REG(0x0100)
#define IO_TIMER2_TMVDCLR         DM320_REG(0x0102)
#define IO_TIMER2_TMPRSCL         DM320_REG(0x0104)
#define IO_TIMER2_TMDIV           DM320_REG(0x0106)
#define IO_TIMER2_TMTRG           DM320_REG(0x0108)
#define IO_TIMER2_TMCNT           DM320_REG(0x010A)

#define IO_TIMER3_TMMD            DM320_REG(0x0180)
#define IO_TIMER3_TMVDCLR         DM320_REG(0x0182)
#define IO_TIMER3_TMPRSCL         DM320_REG(0x0184)
#define IO_TIMER3_TMDIV           DM320_REG(0x0186)
#define IO_TIMER3_TMTRG           DM320_REG(0x0188)
#define IO_TIMER3_TMCNT           DM320_REG(0x018A)

/* Serial 0/1 */
#define IO_SERIAL0_TX_DATA        DM320_REG(0x0200)
#define IO_SERIAL0_RX_DATA        DM320_REG(0x0202)
#define IO_SERIAL0_TX_ENABLE      DM320_REG(0x0204)
#define IO_SERIAL0_MODE           DM320_REG(0x0206)
#define IO_SERIAL0_DMA_TRIGGER    DM320_REG(0x0208)
#define IO_SERIAL0_DMA_MODE       DM320_REG(0x020A)
#define IO_SERIAL0_DMA_SDRAM_LOW  DM320_REG(0x020C)
#define IO_SERIAL0_DMA_SDRAM_HI   DM320_REG(0x020E)
#define IO_SERIAL0_DMA_STATUS     DM320_REG(0x0210)

#define IO_SERIAL1_TX_DATA        DM320_REG(0x0280)
#define IO_SERIAL1_RX_DATA        DM320_REG(0x0282)
#define IO_SERIAL1_TX_ENABLE      DM320_REG(0x0284)
#define IO_SERIAL1_MODE           DM320_REG(0x0286)

/* UART 0/1 */
#define IO_UART0_DTRR             DM320_REG(0x0300)
#define IO_UART0_BRSR             DM320_REG(0x0302)
#define IO_UART0_MSR              DM320_REG(0x0304)
#define IO_UART0_RFCR             DM320_REG(0x0306)
#define IO_UART0_TFCR             DM320_REG(0x0308)
#define IO_UART0_LCR              DM320_REG(0x030A)
#define IO_UART0_SR               DM320_REG(0x030C)

#define IO_UART1_DTRR             DM320_REG(0x0380)
#define IO_UART1_BRSR             DM320_REG(0x0382)
#define IO_UART1_MSR              DM320_REG(0x0384)
#define IO_UART1_RFCR             DM320_REG(0x0386)
#define IO_UART1_TFCR             DM320_REG(0x0388)
#define IO_UART1_LCR              DM320_REG(0x038A)
#define IO_UART1_SR               DM320_REG(0x038C)

/* Watchdog Timer */
#define IO_WATCHDOG_MODE          DM320_REG(0x0400)
#define IO_WATCHDOG_RESET         DM320_REG(0x0402)
#define IO_WATCHDOG_PRESCALAR     DM320_REG(0x0404)
#define IO_WATCHDOG_DIVISOR       DM320_REG(0x0406)
#define IO_WATCHDOG_EXT_RESET     DM320_REG(0x0408)

/* MMC/SD Controller */
#define IO_MMC_CONTROL            DM320_REG(0x0480)
#define IO_MMC_MEM_CLK_CONTROL    DM320_REG(0x0482)
#define IO_MMC_STATUS0            DM320_REG(0x0484)
#define IO_MMC_STATUS1            DM320_REG(0x0486)
#define IO_MMC_INT_ENABLE         DM320_REG(0x0488)
#define IO_MMC_RESPONSE_TIMEOUT   DM320_REG(0x048A)
#define IO_MMC_READ_TIMEOUT       DM320_REG(0x048C)
#define IO_MMC_BLOCK_LENGTH       DM320_REG(0x048E)
#define IO_MMC_NR_BLOCKS          DM320_REG(0x0490)
#define IO_MMC_NR_BLOCKS_COUNT    DM320_REG(0x0492)
#define IO_MMC_RX_DATA            DM320_REG(0x0494)
#define IO_MMC_TX_DATA            DM320_REG(0x0496)
#define IO_MMC_COMMAND            DM320_REG(0x0498)
#define IO_MMC_ARG_LOW            DM320_REG(0x049A)
#define IO_MMC_ARG_HI             DM320_REG(0x049C)
#define IO_MMC_RESPONSE0          DM320_REG(0x049E)
#define IO_MMC_RESPONSE1          DM320_REG(0x04A0)
#define IO_MMC_RESPONSE2          DM320_REG(0x04A2)
#define IO_MMC_RESPONSE3          DM320_REG(0x04A4)
#define IO_MMC_RESPONSE4          DM320_REG(0x04A6)
#define IO_MMC_RESPONSE5          DM320_REG(0x04A8)
#define IO_MMC_RESPONSE6          DM320_REG(0x04AA)
#define IO_MMC_RESPONSE7          DM320_REG(0x04AC)
#define IO_MMC_SPI_DATA           DM320_REG(0x04AE)
#define IO_MMC_SPI_ERR            DM320_REG(0x04B0)
#define IO_MMC_COMMAND_INDEX      DM320_REG(0x04B2)
#define IO_MMC_CLK_START_PHASE    DM320_REG(0x04B4)
#define IO_MMC_RESPONSE_TOUT_CNT  DM320_REG(0x04B6)
#define IO_MMC_READ_TOUT_CNT      DM320_REG(0x04B8)
#define IO_MMC_BLOCK_LENGTH_CNT   DM320_REG(0x04BA)

#define IO_MMC_SD_DMA_TRIGGER     DM320_REG(0x04BC)
#define IO_MMC_SD_DMA_MODE        DM320_REG(0x04BE)
#define IO_MMC_SD_DMA_ADDR_LOW    DM320_REG(0x04C0)
#define IO_MMC_SD_DMA_ADDR_HI     DM320_REG(0x04C2)
#define IO_MMC_SD_DMA_STATUS0     DM320_REG(0x04C4)
#define IO_MMC_SD_DMA_STATUS1     DM320_REG(0x04C6)
#define IO_MMC_SD_DMA_TIMEOUT     DM320_REG(0x04C8)

#define IO_SDIO_CONTROL           DM320_REG(0x04CA)
#define IO_SDIO_STATUS0           DM320_REG(0x04CC)
#define IO_SDIO_INT_ENABLE        DM320_REG(0x04CE)
#define IO_SDIO_INT_STATUS        DM320_REG(0x04D0)

/* Interrupt Controller */
#define IO_INTC_FIQ0              DM320_REG(0x0500)
#define IO_INTC_FIQ1              DM320_REG(0x0502)
#define IO_INTC_FIQ2              DM320_REG(0x0504)
#define IO_INTC_IRQ0              DM320_REG(0x0508)
#define IO_INTC_IRQ1              DM320_REG(0x050A)
#define IO_INTC_IRQ2              DM320_REG(0x050C)
#define IO_INTC_FIQENTRY0         DM320_REG(0x0510)
#define IO_INTC_FIQENTRY1         DM320_REG(0x0512)
#define IO_INTC_FIQ_LOCK_ADDR0    DM320_REG(0x0514)
#define IO_INTC_FIQ_LOCK_ADDR1    DM320_REG(0x0516)
#define IO_INTC_IRQENTRY0         DM320_REG(0x0518)
#define IO_INTC_IRQENTRY1         DM320_REG(0x051A)
#define IO_INTC_IRQ_LOCK_ADDR0    DM320_REG(0x051C)
#define IO_INTC_IRQ_LOCK_ADDR1    DM320_REG(0x051E)
#define IO_INTC_FISEL0            DM320_REG(0x0520)
#define IO_INTC_FISEL1            DM320_REG(0x0522)
#define IO_INTC_FISEL2            DM320_REG(0x0524)
#define IO_INTC_EINT0             DM320_REG(0x0528)
#define IO_INTC_EINT1             DM320_REG(0x052A)
#define IO_INTC_EINT2             DM320_REG(0x052C)
#define IO_INTC_RAW               DM320_REG(0x0530)
#define IO_INTC_ENTRY_TBA0        DM320_REG(0x0538)
#define IO_INTC_ENTRY_TBA1        DM320_REG(0x053A)
#define IO_INTC_PRIORITY0         DM320_REG(0x0540)
#define IO_INTC_PRIORITY1         DM320_REG(0x0542)
#define IO_INTC_PRIORITY2         DM320_REG(0x0544)
#define IO_INTC_PRIORITY3         DM320_REG(0x0546)
#define IO_INTC_PRIORITY4         DM320_REG(0x0548)
#define IO_INTC_PRIORITY5         DM320_REG(0x054A)
#define IO_INTC_PRIORITY6         DM320_REG(0x054C)
#define IO_INTC_PRIORITY7         DM320_REG(0x054E)
#define IO_INTC_PRIORITY8         DM320_REG(0x0550)
#define IO_INTC_PRIORITY9         DM320_REG(0x0552)
#define IO_INTC_PRIORITY10        DM320_REG(0x0554)
#define IO_INTC_PRIORITY11        DM320_REG(0x0556)
#define IO_INTC_PRIORITY12        DM320_REG(0x0558)
#define IO_INTC_PRIORITY13        DM320_REG(0x055A)
#define IO_INTC_PRIORITY14        DM320_REG(0x055C)
#define IO_INTC_PRIORITY15        DM320_REG(0x055E)
#define IO_INTC_PRIORITY16        DM320_REG(0x0560)
#define IO_INTC_PRIORITY17        DM320_REG(0x0562)
#define IO_INTC_PRIORITY18        DM320_REG(0x0564)
#define IO_INTC_PRIORITY19        DM320_REG(0x0566)
#define IO_INTC_PRIORITY20        DM320_REG(0x0568)
#define IO_INTC_PRIORITY21        DM320_REG(0x056A)
#define IO_INTC_PRIORITY22        DM320_REG(0x056C)

/* GIO Controller */
#define IO_GIO_DIR0               DM320_REG(0x0580)
#define IO_GIO_DIR1               DM320_REG(0x0582)
#define IO_GIO_DIR2               DM320_REG(0x0584)
#define IO_GIO_INV0               DM320_REG(0x0586)
#define IO_GIO_INV1               DM320_REG(0x0588)
#define IO_GIO_INV2               DM320_REG(0x058A)
#define IO_GIO_BITSET0            DM320_REG(0x058C)
#define IO_GIO_BITSET1            DM320_REG(0x058E)
#define IO_GIO_BITSET2            DM320_REG(0x0590)
#define IO_GIO_BITCLR0            DM320_REG(0x0592)
#define IO_GIO_BITCLR1            DM320_REG(0x0594)
#define IO_GIO_BITCLR2            DM320_REG(0x0596)
#define IO_GIO_IRQPORT            DM320_REG(0x0598)
#define IO_GIO_IRQEDGE            DM320_REG(0x059A)
#define IO_GIO_CHAT0              DM320_REG(0x059C)
#define IO_GIO_CHAT1              DM320_REG(0x059E)
#define IO_GIO_CHAT2              DM320_REG(0x05A0)
#define IO_GIO_NCHAT              DM320_REG(0x05A2)
#define IO_GIO_FSEL0              DM320_REG(0x05A4)
#define IO_GIO_FSEL1              DM320_REG(0x05A6)
#define IO_GIO_FSEL2              DM320_REG(0x05A8)
#define IO_GIO_FSEL3              DM320_REG(0x05AA)
#define IO_GIO_FSEL4              DM320_REG(0x05AC)
#define IO_GIO_CARD_SET           DM320_REG(0x05AE)
#define IO_GIO_CARD_ST            DM320_REG(0x05B0)

/* DSP Controller */
#define IO_DSPC_HPIB_CONTROL      DM320_REG(0x0600)
#define IO_DSPC_HPIB_STATUS       DM320_REG(0x0602)

/* OSD Controller */
#define IO_OSD_MODE               DM320_REG(0x0680)
#define IO_OSD_VIDWINMD           DM320_REG(0x0682)
#define IO_OSD_OSDWINMD0          DM320_REG(0x0684)
#define IO_OSD_OSDWINMD1          DM320_REG(0x0686)
#define IO_OSD_ATRMD              DM320_REG(0x0688)
#define IO_OSD_RECTCUR            DM320_REG(0x0688)
#define IO_OSD_RESERVED           DM320_REG(0x068A)
#define IO_OSD_VIDWIN0OFST        DM320_REG(0x068C)
#define IO_OSD_VIDWIN1OFST        DM320_REG(0x068E)
#define IO_OSD_OSDWIN0OFST        DM320_REG(0x0690)
#define IO_OSD_OSDWIN1OFST        DM320_REG(0x0692)
#define IO_OSD_VIDWINADH          DM320_REG(0x0694)
#define IO_OSD_VIDWIN0ADL         DM320_REG(0x0696)
#define IO_OSD_VIDWIN1ADL         DM320_REG(0x0698)
#define IO_OSD_OSDWINADH          DM320_REG(0x069A)
#define IO_OSD_OSDWIN0ADL         DM320_REG(0x069C)
#define IO_OSD_OSDWIN1ADL         DM320_REG(0x069E)
#define IO_OSD_BASEPX             DM320_REG(0x06A0)
#define IO_OSD_BASEPY             DM320_REG(0x06A2)
#define IO_OSD_VIDWIN0XP          DM320_REG(0x06A4)
#define IO_OSD_VIDWIN0YP          DM320_REG(0x06A6)
#define IO_OSD_VIDWIN0XL          DM320_REG(0x06A8)
#define IO_OSD_VIDWIN0YL          DM320_REG(0x06AA)
#define IO_OSD_VIDWIN1XP          DM320_REG(0x06AC)
#define IO_OSD_VIDWIN1YP          DM320_REG(0x06AE)
#define IO_OSD_VIDWIN1XL          DM320_REG(0x06B0)
#define IO_OSD_VIDWIN1YL          DM320_REG(0x06B2)

#define IO_OSD_OSDWIN0XP          DM320_REG(0x06B4)
#define IO_OSD_OSDWIN0YP          DM320_REG(0x06B6)
#define IO_OSD_OSDWIN0XL          DM320_REG(0x06B8)
#define IO_OSD_OSDWIN0YL          DM320_REG(0x06BA)
#define IO_OSD_OSDWIN1XP          DM320_REG(0x06BC)
#define IO_OSD_OSDWIN1YP          DM320_REG(0x06BE)
#define IO_OSD_OSDWIN1XL          DM320_REG(0x06C0)
#define IO_OSD_OSDWIN1YL          DM320_REG(0x06C2)
#define IO_OSD_CURXP              DM320_REG(0x06C4)
#define IO_OSD_CURYP              DM320_REG(0x06C6)
#define IO_OSD_CURXL              DM320_REG(0x06C8)
#define IO_OSD_CURYL              DM320_REG(0x06CA)

#define IO_OSD_W0BMP01            DM320_REG(0x06D0)
#define IO_OSD_W0BMP23            DM320_REG(0x06D2)
#define IO_OSD_W0BMP45            DM320_REG(0x06D4)
#define IO_OSD_W0BMP67            DM320_REG(0x06D6)
#define IO_OSD_W0BMP89            DM320_REG(0x06D8)
#define IO_OSD_W0BMPAB            DM320_REG(0x06DA)
#define IO_OSD_W0BMPCD            DM320_REG(0x06DC)
#define IO_OSD_W0BMPEF            DM320_REG(0x06DE)

#define IO_OSD_W1BMP01            DM320_REG(0x06E0)
#define IO_OSD_W1BMP23            DM320_REG(0x06E2)
#define IO_OSD_W1BMP45            DM320_REG(0x06E4)
#define IO_OSD_W1BMP67            DM320_REG(0x06E6)
#define IO_OSD_W1BMP89            DM320_REG(0x06E8)
#define IO_OSD_W1BMPAB            DM320_REG(0x06EA)
#define IO_OSD_W1BMPCD            DM320_REG(0x06EC)
#define IO_OSD_W1BMPEF            DM320_REG(0x06EE)

#define IO_OSD_MISCCTL            DM320_REG(0x06F4)
#define IO_OSD_CLUTRAMYCB         DM320_REG(0x06F6)
#define IO_OSD_CLUTRAMCR          DM320_REG(0x06F8)

#define IO_OSD_PPWIN0ADH          DM320_REG(0x06FC)
#define IO_OSD_PPWIN0ADL          DM320_REG(0x06FE)


/* CCD Controller */
#define IO_CCD_SYNCEN             DM320_REG(0x0700)
#define IO_CCD_MODESET            DM320_REG(0x0702)
#define IO_CCD_HDWIDTH            DM320_REG(0x0704)
#define IO_CCD_VDWIDTH            DM320_REG(0x0706)
#define IO_CCD_PPLN               DM320_REG(0x0708)
#define IO_CCD_LPFR               DM320_REG(0x070A)
#define IO_CCD_SPH                DM320_REG(0x070C)
#define IO_CCD_NPH                DM320_REG(0x070E)
#define IO_CCD_SLV0               DM320_REG(0x0710)
#define IO_CCD_SLV1               DM320_REG(0x0712)
#define IO_CCD_NLV                DM320_REG(0x0714)
#define IO_CCD_CULH               DM320_REG(0x0716)
#define IO_CCD_CULV               DM320_REG(0x0718)
#define IO_CCD_HSIZE              DM320_REG(0x071A)
#define IO_CCD_SDOFST             DM320_REG(0x071C)
#define IO_CCD_STADRH             DM320_REG(0x071E)
#define IO_CCD_STADRL             DM320_REG(0x0720)
#define IO_CCD_CLAMP              DM320_REG(0x0722)
#define IO_CCD_DCSUB              DM320_REG(0x0724)
#define IO_CCD_COLPTN             DM320_REG(0x0726)
#define IO_CCD_BLKCMP0            DM320_REG(0x0728)
#define IO_CCD_BLKCMP1            DM320_REG(0x072A)
#define IO_CCD_MEDFILT            DM320_REG(0x072C)
#define IO_CCD_RYEGAIN            DM320_REG(0x072E)
#define IO_CCD_GRCYGAIN           DM320_REG(0x0730)
#define IO_CCD_GBGGAIN            DM320_REG(0x0732)
#define IO_CCD_BMGGAIN            DM320_REG(0x0734)
#define IO_CCD_OFFSET             DM320_REG(0x0736)
#define IO_CCD_OUTCLP             DM320_REG(0x0738)
#define IO_CCD_VDINT0             DM320_REG(0x073A)
#define IO_CCD_VDINT1             DM320_REG(0x073C)
#define IO_CCD_RSV0               DM320_REG(0x073E)
#define IO_CCD_GAMMAWD            DM320_REG(0x0740)
#define IO_CCD_REC656IF           DM320_REG(0x0742)
#define IO_CCD_CCDFG              DM320_REG(0x0744)
#define IO_CCD_FMTCFG             DM320_REG(0x0746)
#define IO_CCD_FMTSPH             DM320_REG(0x0748)
#define IO_CCD_FMTLNH             DM320_REG(0x074A)
#define IO_CCD_FMTSLV             DM320_REG(0x074C)
#define IO_CCD_FMTSNV             DM320_REG(0x074E)
#define IO_CCD_FMTOFST            DM320_REG(0x0750)
#define IO_CCD_FMTRLEN            DM320_REG(0x0752)
#define IO_CCD_FMTHCNT            DM320_REG(0x0754)
#define IO_CCD_FMTPTNA            DM320_REG(0x0756)
#define IO_CCD_FMTPTNB            DM320_REG(0x0758)

/* NTSC/PAL Encoder */
#define IO_VID_ENC_VMOD           DM320_REG(0x0800)
#define IO_VID_ENC_VDCTL          DM320_REG(0x0802)
#define IO_VID_ENC_VDPRO          DM320_REG(0x0804)
#define IO_VID_ENC_SYNCTL         DM320_REG(0x0806)
#define IO_VID_ENC_HSPLS          DM320_REG(0x0808)
#define IO_VID_ENC_VSPLS          DM320_REG(0x080A)
#define IO_VID_ENC_HINT           DM320_REG(0x080C)
#define IO_VID_ENC_HSTART         DM320_REG(0x080E)
#define IO_VID_ENC_HVALID         DM320_REG(0x0810)
#define IO_VID_ENC_VINT           DM320_REG(0x0812)
#define IO_VID_ENC_VSTART         DM320_REG(0x0814)
#define IO_VID_ENC_VVALID         DM320_REG(0x0816)
#define IO_VID_ENC_HSDLY          DM320_REG(0x0818)
#define IO_VID_ENC_VSDLY          DM320_REG(0x081A)
#define IO_VID_ENC_YCCTL          DM320_REG(0x081C)
#define IO_VID_ENC_RGBCTL         DM320_REG(0x081E)
#define IO_VID_ENC_RGBCLP         DM320_REG(0x0820)
#define IO_VID_ENC_LNECTL         DM320_REG(0x0822)
#define IO_VID_ENC_CULLLNE        DM320_REG(0x0824)
#define IO_VID_ENC_LCDOUT         DM320_REG(0x0826)
#define IO_VID_ENC_BRTS           DM320_REG(0x0828)
#define IO_VID_ENC_BRTW           DM320_REG(0x082A)
#define IO_VID_ENC_ACCTL          DM320_REG(0x082C)
#define IO_VID_ENC_PWMP           DM320_REG(0x082E)
#define IO_VID_ENC_PWMW           DM320_REG(0x0830)
#define IO_VID_ENC_DCLKCTL        DM320_REG(0x0832)
#define IO_VID_ENC_DCLKPTN0       DM320_REG(0x0834)
#define IO_VID_ENC_DCLKPTN1       DM320_REG(0x0836)
#define IO_VID_ENC_DCLKPTN2       DM320_REG(0x0838)
#define IO_VID_ENC_DCLKPTN3       DM320_REG(0x083A)
#define IO_VID_ENC_DCLKPTN0A      DM320_REG(0x083C)
#define IO_VID_ENC_DCLKPTN1A      DM320_REG(0x083E)
#define IO_VID_ENC_DCLKPTN2A      DM320_REG(0x0840)
#define IO_VID_ENC_DCLKPTN3A      DM320_REG(0x0842)
#define IO_VID_ENC_DCLKHS         DM320_REG(0x0844)
#define IO_VID_ENC_DCLKHSA        DM320_REG(0x0846)
#define IO_VID_ENC_DCLKHR         DM320_REG(0x0848)
#define IO_VID_ENC_DCLKVS         DM320_REG(0x084A)
#define IO_VID_ENC_DCLKVR         DM320_REG(0x084C)
#define IO_VID_ENC_CAPCTL         DM320_REG(0x084E)
#define IO_VID_ENC_CAPDO          DM320_REG(0x0850)
#define IO_VID_ENC_CAPDE          DM320_REG(0x0852)
#define IO_VID_ENC_ATR0           DM320_REG(0x0854)

/* Clock Controller */
#define IO_CLK_PLLA               DM320_REG(0x0880)
#define IO_CLK_PLLB               DM320_REG(0x0882)
#define IO_CLK_SEL0               DM320_REG(0x0884)
#define IO_CLK_SEL1               DM320_REG(0x0886)
#define IO_CLK_SEL2               DM320_REG(0x0888)
#define IO_CLK_DIV0               DM320_REG(0x088A)
#define IO_CLK_DIV1               DM320_REG(0x088C)
#define IO_CLK_DIV2               DM320_REG(0x088E)
#define IO_CLK_DIV3               DM320_REG(0x0890)
#define IO_CLK_DIV4               DM320_REG(0x0892)
#define IO_CLK_BYP                DM320_REG(0x0894)
#define IO_CLK_INV                DM320_REG(0x0896)
#define IO_CLK_MOD0               DM320_REG(0x0898)
#define IO_CLK_MOD1               DM320_REG(0x089A)
#define IO_CLK_MOD2               DM320_REG(0x089C)
#define IO_CLK_LPCTL0             DM320_REG(0x089E)
#define IO_CLK_LPCTL1             DM320_REG(0x08A0)
#define IO_CLK_OSEL               DM320_REG(0x08A2)
#define IO_CLK_O0DIV              DM320_REG(0x08A4)
#define IO_CLK_O1DIV              DM320_REG(0x08A6)
#define IO_CLK_O2DIV              DM320_REG(0x08A8)
#define IO_CLK_PWM0C              DM320_REG(0x08AA)
#define IO_CLK_PWM0H              DM320_REG(0x08AC)
#define IO_CLK_PWM1C              DM320_REG(0x08AE)
#define IO_CLK_PWM1H              DM320_REG(0x08B0)

/* Bus Controller */
#define IO_BUSC_ECR               DM320_REG(0x0900)
#define IO_BUSC_EBYTER            DM320_REG(0x0902)
#define IO_BUSC_EBITR             DM320_REG(0x0904)
#define IO_BUSC_REVR              DM320_REG(0x0906)

/* SDRAM Controller */
#define IO_SDRAM_SDBUFD0L         DM320_REG(0x0980)
#define IO_SDRAM_SDBUFD0H         DM320_REG(0x0982)
#define IO_SDRAM_SDBUFD1L         DM320_REG(0x0984)
#define IO_SDRAM_SDBUFD1H         DM320_REG(0x0986)
#define IO_SDRAM_SDBUFD2L         DM320_REG(0x0988)
#define IO_SDRAM_SDBUFD2H         DM320_REG(0x098A)
#define IO_SDRAM_SDBUFD3L         DM320_REG(0x098C)
#define IO_SDRAM_SDBUFD3H         DM320_REG(0x098E)
#define IO_SDRAM_SDBUFD4L         DM320_REG(0x0990)
#define IO_SDRAM_SDBUFD4H         DM320_REG(0x0992)
#define IO_SDRAM_SDBUFD5L         DM320_REG(0x0994)
#define IO_SDRAM_SDBUFD5H         DM320_REG(0x0996)
#define IO_SDRAM_SDBUFD6L         DM320_REG(0x0998)
#define IO_SDRAM_SDBUFD6H         DM320_REG(0x099A)
#define IO_SDRAM_SDBUFD7L         DM320_REG(0x099C)
#define IO_SDRAM_SDBUFD7H         DM320_REG(0x099E)
#define IO_SDRAM_SDBUFAD1         DM320_REG(0x09A0)
#define IO_SDRAM_SDBUFAD2         DM320_REG(0x09A2)
#define IO_SDRAM_SDBUFCTL         DM320_REG(0x09A4)
#define IO_SDRAM_SDMODE           DM320_REG(0x09A6)
#define IO_SDRAM_REFCTL           DM320_REG(0x09A8)
#define IO_SDRAM_SDPRTY1          DM320_REG(0x09AA)
#define IO_SDRAM_SDPRTY2          DM320_REG(0x09AC)
#define IO_SDRAM_SDPRTY3          DM320_REG(0x09AE)
#define IO_SDRAM_SDPRTY4          DM320_REG(0x09B0)
#define IO_SDRAM_SDPRTY5          DM320_REG(0x09B2)
#define IO_SDRAM_SDPRTY6          DM320_REG(0x09B4)
#define IO_SDRAM_SDPRTY7          DM320_REG(0x09B6)
#define IO_SDRAM_SDPRTY8          DM320_REG(0x09B8)
#define IO_SDRAM_SDPRTY9          DM320_REG(0x09BA)
#define IO_SDRAM_SDPRTY10         DM320_REG(0x09BC)
#define IO_SDRAM_SDPRTY11         DM320_REG(0x09BE)
#define IO_SDRAM_SDPRTY12         DM320_REG(0x09C0)
#define IO_SDRAM_RSV              DM320_REG(0x09C2)
#define IO_SDRAM_SDPRTYON         DM320_REG(0x09C4)
#define IO_SDRAM_SDDMASEL         DM320_REG(0x09C6)

/* EMIF Controller */
#define IO_EMIF_CS0CTRL1          DM320_REG(0x0A00)
#define IO_EMIF_CS0CTRL2          DM320_REG(0x0A02)
#define IO_EMIF_CS0CTRL3          DM320_REG(0x0A04)
#define IO_EMIF_CS1CTRL1A         DM320_REG(0x0A06)
#define IO_EMIF_CS1CTRL1B         DM320_REG(0x0A08)
#define IO_EMIF_CS1CTRL2          DM320_REG(0x0A0A)
#define IO_EMIF_CS2CTRL1          DM320_REG(0x0A0C)
#define IO_EMIF_CS2CTRL2          DM320_REG(0x0A0E)
#define IO_EMIF_CS3CTRL1          DM320_REG(0x0A10)
#define IO_EMIF_CS3CTRL2          DM320_REG(0x0A12)
#define IO_EMIF_CS4CTRL1          DM320_REG(0x0A14)
#define IO_EMIF_CS4CTRL2          DM320_REG(0x0A16)
#define IO_EMIF_BUSCTRL           DM320_REG(0x0A18)
#define IO_EMIF_BUSRLS            DM320_REG(0x0A1A)
#define IO_EMIF_CFCTRL1           DM320_REG(0x0A1C)
#define IO_EMIF_CFCTRL2           DM320_REG(0x0A1E)
#define IO_EMIF_SMCTRL            DM320_REG(0x0A20)
#define IO_EMIF_BUSINTEN          DM320_REG(0x0A22)
#define IO_EMIF_BUSSTS            DM320_REG(0x0A24)
#define IO_EMIF_BUSWAITMD         DM320_REG(0x0A26)
#define IO_EMIF_ECC1CP            DM320_REG(0x0A28)
#define IO_EMIF_ECC1LP            DM320_REG(0x0A2A)
#define IO_EMIF_ECC2CP            DM320_REG(0x0A2C)
#define IO_EMIF_ECC2LP            DM320_REG(0x0A2E)
#define IO_EMIF_ECC3CP            DM320_REG(0x0A30)
#define IO_EMIF_ECC3LP            DM320_REG(0x0A32)
#define IO_EMIF_ECC4CP            DM320_REG(0x0A34)
#define IO_EMIF_ECC4LP            DM320_REG(0x0A36)
#define IO_EMIF_ECC5CP            DM320_REG(0x0A38)
#define IO_EMIF_ECC5LP            DM320_REG(0x0A3A)
#define IO_EMIF_ECC6CP            DM320_REG(0x0A3C)
#define IO_EMIF_ECC6LP            DM320_REG(0x0A3E)
#define IO_EMIF_ECC7CP            DM320_REG(0x0A40)
#define IO_EMIF_ECC7LP            DM320_REG(0x0A42)
#define IO_EMIF_ECC8CP            DM320_REG(0x0A44)
#define IO_EMIF_ECC8LP            DM320_REG(0x0A46)
#define IO_EMIF_ECCCLR            DM320_REG(0x0A48)
#define IO_EMIF_PAGESZ            DM320_REG(0x0A4A)
#define IO_EMIF_PRIORCTL          DM320_REG(0x0A4C)
#define IO_EMIF_MGDSPDEST         DM320_REG(0x0A4E)
#define IO_EMIF_MGDSPADDH         DM320_REG(0x0A50)
#define IO_EMIF_MGDSPADDL         DM320_REG(0x0A52)
#define IO_EMIF_AHBADDH           DM320_REG(0x0A54)
#define IO_EMIF_AHBADDL           DM320_REG(0x0A56)
#define IO_EMIF_MTCADDH           DM320_REG(0x0A58)
#define IO_EMIF_MTCADDL           DM320_REG(0x0A5A)
#define IO_EMIF_DMASIZE           DM320_REG(0x0A5C)
#define IO_EMIF_DMAMTCSEL         DM320_REG(0x0A5E)
#define IO_EMIF_DMACTL            DM320_REG(0x0A60)

/* Preivew Engine */
#define IO_PREV_ENG_PVEN          DM320_REG(0x0A80)
#define IO_PREV_ENG_PVSET1        DM320_REG(0x0A82)
#define IO_PREV_ENG_RADRH         DM320_REG(0x0A84)
#define IO_PREV_ENG_RADRL         DM320_REG(0x0A86)
#define IO_PREV_ENG_WADRH         DM320_REG(0x0A88)
#define IO_PREV_ENG_WADRL         DM320_REG(0x0A8A)
#define IO_PREV_ENG_HSTART        DM320_REG(0x0A8C)
#define IO_PREV_ENG_HSIZE         DM320_REG(0x0A8E)
#define IO_PREV_ENG_VSTART        DM320_REG(0x0A90)
#define IO_PREV_ENG_VSIZE         DM320_REG(0x0A92)
#define IO_PREV_ENG_PVSET2        DM320_REG(0x0A94)
#define IO_PREV_ENG_NFILT         DM320_REG(0x0A96)
#define IO_PREV_ENG_DGAIN         DM320_REG(0x0A98)
#define IO_PREV_ENG_WBGAIN0       DM320_REG(0x0A9A)
#define IO_PREV_ENG_WBGAIN1       DM320_REG(0x0A9C)
#define IO_PREV_ENG_SMTH          DM320_REG(0x0A9E)
#define IO_PREV_ENG_HRSZ          DM320_REG(0x0AA0)
#define IO_PREV_ENG_VRSZ          DM320_REG(0x0AA2)
#define IO_PREV_ENG_BLOFST0       DM320_REG(0x0AA4)
#define IO_PREV_ENG_BLOFST1       DM320_REG(0x0AA6)
#define IO_PREV_ENG_MTXGAIN0      DM320_REG(0x0AA8)
#define IO_PREV_ENG_MTXGAIN1      DM320_REG(0x0AAA)
#define IO_PREV_ENG_MTXGAIN2      DM320_REG(0x0AAC)
#define IO_PREV_ENG_MTXGAIN3      DM320_REG(0x0AAE)
#define IO_PREV_ENG_MTXGAIN4      DM320_REG(0x0AB0)
#define IO_PREV_ENG_MTXGAIN5      DM320_REG(0x0AB2)
#define IO_PREV_ENG_MTXGAIN6      DM320_REG(0x0AB4)
#define IO_PREV_ENG_MTXGAIN7      DM320_REG(0x0AB6)
#define IO_PREV_ENG_MTXGAIN8      DM320_REG(0x0AB8)
#define IO_PREV_ENG_MTXOFST0      DM320_REG(0x0ABA)
#define IO_PREV_ENG_MTXOFST1      DM320_REG(0x0ABC)
#define IO_PREV_ENG_MTXOFST2      DM320_REG(0x0ABE)
#define IO_PREV_ENG_GAMTBYP       DM320_REG(0x0AC0)
#define IO_PREV_ENG_CSC0          DM320_REG(0x0AC2)
#define IO_PREV_ENG_CSC1          DM320_REG(0x0AC4)
#define IO_PREV_ENG_CSC2          DM320_REG(0x0AC6)
#define IO_PREV_ENG_CSC3          DM320_REG(0x0AC8)
#define IO_PREV_ENG_CSC4          DM320_REG(0x0ACA)
#define IO_PREV_ENG_YOFST         DM320_REG(0x0ACC)
#define IO_PREV_ENG_COFST         DM320_REG(0x0ACE)
#define IO_PREV_ENG_CNTBRT        DM320_REG(0x0AD0)
#define IO_PREV_ENG_CSUP0         DM320_REG(0x0AD2)
#define IO_PREV_ENG_CSUP1         DM320_REG(0x0AD4)
#define IO_PREV_ENG_SETUPY        DM320_REG(0x0AD4)
#define IO_PREV_ENG_SETUPC        DM320_REG(0x0AD8)
#define IO_PREV_ENG_TABLE_ADDR    DM320_REG(0x0ADA)
#define IO_PREV_ENG_TABLE_DATA    DM320_REG(0x0ADC)
#define IO_PREV_ENG_HG_CTL        DM320_REG(0x0ADE)
#define IO_PREV_ENG_HG_R0_HSTART  DM320_REG(0x0AE0)
#define IO_PREV_ENG_HG_R0_HSIZE   DM320_REG(0x0AE2)
#define IO_PREV_ENG_HG_R0_VSTART  DM320_REG(0x0AE4)
#define IO_PREV_ENG_HR_R0_VSIZE   DM320_REG(0x0AE6)
#define IO_PREV_ENG_HG_R1_HSTART  DM320_REG(0x0AE8)
#define IO_PREV_ENG_HG_R1_HSIZE   DM320_REG(0x0AEA)
#define IO_PREV_ENG_HG_R1_VSTART  DM320_REG(0x0AEC)
#define IO_PREV_ENG_HG_R1_VSIZE   DM320_REG(0x0AEE)
#define IO_PREV_ENG_HG_R2_HSTART  DM320_REG(0x0AF0)
#define IO_PREV_ENG_HG_R2_HSIZE   DM320_REG(0x0AF2)
#define IO_PREV_ENG_HG_R2_VSTART  DM320_REG(0x0AF4)
#define IO_PREV_ENG_HG_R2_VSIZE   DM320_REG(0x0AF6)
#define IO_PREV_ENG_HG_R3_HSTART  DM320_REG(0x0AF8)
#define IO_PREV_ENG_HG_R3_HSIZE   DM320_REG(0x0AFA)
#define IO_PREV_ENG_HG_R3_VSTART  DM320_REG(0x0AFC)
#define IO_PREV_ENG_HG_R3_VSIZE   DM320_REG(0x0AFE)
#define IO_PREV_ENG_HG_ADDR       DM320_REG(0x0B00)
#define IO_PREV_ENG_HG_DATA       DM320_REG(0x0B02)

/* H3A Hardware */
#define IO_H3A_H3ACTRL            DM320_REG(0x0B80)
#define IO_H3A_AFCTRL             DM320_REG(0x0B82)
#define IO_H3A_AFPAX1             DM320_REG(0x0B84)
#define IO_H3A_AFPAX2             DM320_REG(0x0B86)
#define IO_H3A_AFPAX3             DM320_REG(0x0B88)
#define IO_H3A_AFPAX4             DM320_REG(0x0B8A)
#define IO_H3A_AFIRSH             DM320_REG(0x0B8C)
#define IO_H3A_AFPAX5             DM320_REG(0x0B8E)
#define IO_H3A_AFSDRA1            DM320_REG(0x0B90)
#define IO_H3A_AFSDRA2            DM320_REG(0x0B92)
#define IO_H3A_AFSDRFLG           DM320_REG(0x0B94)
#define IO_H3A_AFCOEFF10          DM320_REG(0x0B96)
#define IO_H3A_AFCOEFF11          DM320_REG(0x0B98)
#define IO_H3A_AFCOEFF12          DM320_REG(0x0B9A)
#define IO_H3A_AFCOEFF13          DM320_REG(0x0B9C)
#define IO_H3A_AFCOEFF14          DM320_REG(0x0B9E)
#define IO_H3A_AFCOEFF15          DM320_REG(0x0BA0)
#define IO_H3A_AFCOEFF16          DM320_REG(0x0BA2)
#define IO_H3A_AFCOEFF17          DM320_REG(0x0BA4)
#define IO_H3A_AFCOEFF18          DM320_REG(0x0BA6)
#define IO_H3A_AFCOEFF19          DM320_REG(0x0BA8)
#define IO_H3A_AFCOEFF110         DM320_REG(0x0BAA)
#define IO_H3A_AFCOEFF20          DM320_REG(0x0BAC)
#define IO_H3A_AFCOEFF21          DM320_REG(0x0BAE)
#define IO_H3A_AFCOEFF22          DM320_REG(0x0BB0)
#define IO_H3A_AFCOEFF23          DM320_REG(0x0BB2)
#define IO_H3A_AFCOEFF24          DM320_REG(0x0BB4)
#define IO_H3A_AFCOEFF25          DM320_REG(0x0BB6)
#define IO_H3A_AFCOEFF26          DM320_REG(0x0BB8)
#define IO_H3A_AFCOEFF27          DM320_REG(0x0BBA)
#define IO_H3A_AFCOEFF28          DM320_REG(0x0BBC)
#define IO_H3A_AFCOEFF29          DM320_REG(0x0BBE)
#define IO_H3A_AFCOEFF210         DM320_REG(0x0BC0)
#define IO_H3A_AEWCTRL            DM320_REG(0x0BC2)
#define IO_H3A_AEWWIN1            DM320_REG(0x0BC4)
#define IO_H3A_AEWWIN2            DM320_REG(0x0BC6)
#define IO_H3A_AEWWIN3            DM320_REG(0x0BC8)
#define IO_H3A_AEWWIN4            DM320_REG(0x0BCA)
#define IO_H3A_AEWWIN5            DM320_REG(0x0BCC)
#define IO_H3A_AEWSDRA1           DM320_REG(0x0BCE)
#define IO_H3A_AEWSDRA2           DM320_REG(0x0BD0)
#define IO_H3A_AEWSDRFLG          DM320_REG(0x0BD2)

/* Reserved 0x0C00 - 0x0CCFF */

/* Memory Stick Controller : */
#define IO_MEM_STICK_MODE         DM320_REG(0x0C80)
#define IO_MEM_STICK_CMD          DM320_REG(0x0C82)
#define IO_MEM_STICK_DATA         DM320_REG(0x0C84)
#define IO_MEM_STICK_STATUS       DM320_REG(0x0C86)
#define IO_MEM_STICK_SYS          DM320_REG(0x0C88)
#define IO_MEM_STICK_ENDIAN       DM320_REG(0x0C8A)
#define IO_MEM_STICK_INT_STATUS   DM320_REG(0x0C8C)
#define IO_MEM_STICK_DMA_TRG      DM320_REG(0x0C8E)
#define IO_MEM_STICK_DMA_MODE     DM320_REG(0x0C90)
#define IO_MEM_STICK_SDRAM_ADDL   DM320_REG(0x0C92)
#define IO_MEM_STICK_SDRAM_ADDH   DM320_REG(0x0C94)
#define IO_MEM_STICK_DMA_STATUS   DM320_REG(0x0C96)

/* ATM : WBB Need to find these Register values */
#define IO_ATM_                   DM320_REG(0x0D00

/* I2C */
#define IO_I2C_TXDATA             DM320_REG(0x0D80)
#define IO_I2C_RXDATA             DM320_REG(0x0D82)
#define IO_I2C_SCS                DM320_REG(0x0D84)

/* VLYNQ */
#define VL_ID                     DM320_REG2(0x0300)
#define VL_CTRL                   DM320_REG2(0x0304)
#define VL_STAT                   DM320_REG2(0x0308)
#define VL_INTPRI                 DM320_REG2(0x030c)
#define VL_INTST                  DM320_REG2(0x0310)
#define VL_INTPND                 DM320_REG2(0x0314)
#define VL_INTPTR                 DM320_REG2(0x0318)
#define VL_TXMAP                  DM320_REG2(0x031c)
#define VL_RXMAPSZ1               DM320_REG2(0x0320)
#define VL_RXMAPOF1               DM320_REG2(0x0324)
#define VL_RXMAPSZ2               DM320_REG2(0x0328)
#define VL_RXMAPOF2               DM320_REG2(0x032c)
#define VL_RXMAPSZ3               DM320_REG2(0x0330)
#define VL_RXMAPOF3               DM320_REG2(0x0334)
#define VL_RXMAPSZ4               DM320_REG2(0x0338)
#define VL_RXMAPOF4               DM320_REG2(0x033c)
#define VL_CHIPVER                DM320_REG2(0x0340)
#define VL_AUTONEG                DM320_REG2(0x0344)
#define VL_MANNEG                 DM320_REG2(0x0348)
#define VL_NEGSTAT                DM320_REG2(0x034c)
#define VL_ENDIAN                 DM320_REG2(0x035c)
#define VL_INTVEC30               DM320_REG2(0x0360)
#define VL_INTVEC74               DM320_REG2(0x0364)
#define VL_ID_R                   DM320_REG2(0x0380)
#define VL_CTRL_R                 DM320_REG2(0x0384)
#define VL_STAT_R                 DM320_REG2(0x0388)
#define VL_INTPRI_R               DM320_REG2(0x038c)
#define VL_INTST_R                DM320_REG2(0x0390)
#define VL_INTPND_R               DM320_REG2(0x0394)
#define VL_INTPTR_R               DM320_REG2(0x0398)
#define VL_TXMAP_R                DM320_REG2(0x039c)
#define VL_RXMAPSZ1_R             DM320_REG2(0x03a0)
#define VL_RXMAPOF1_R             DM320_REG2(0x03a4)
#define VL_RXMAPSZ2_R             DM320_REG2(0x03a8)
#define VL_RXMAPOF2_R             DM320_REG2(0x03ac)
#define VL_RXMAPSZ3_R             DM320_REG2(0x03b0)
#define VL_RXMAPOF3_R             DM320_REG2(0x03b4)
#define VL_RXMAPSZ4_R             DM320_REG2(0x03b8)
#define VL_RXMAPOF4_R             DM320_REG2(0x03bc)
#define VL_CHIPVER_R              DM320_REG2(0x03c0)
#define VL_AUTONEG_R              DM320_REG2(0x03c4)
#define VL_MANNEG_R               DM320_REG2(0x03c8)
#define VL_NEGSTAT_R              DM320_REG2(0x03cc)
#define VL_ENDIAN_R               DM320_REG2(0x03dc)
#define VL_INTVEC30_R             DM320_REG2(0x03e0)
#define VL_INTVEC74_R             DM320_REG2(0x03e4)

/* Taken from linux/include/asm-arm/arch-itdm320/irqs.h
 *
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2004 Ingenient Technologies
 */

/*
 *  Interrupt numbers
 */
#define IRQ_TIMER0     0
#define IRQ_TIMER1     1
#define IRQ_TIMER2     2
#define IRQ_TIMER3     3
#define IRQ_CCD_VD0    4
#define IRQ_CCD_VD1    5
#define IRQ_CCD_WEN    6
#define IRQ_VENC       7
#define IRQ_SERIAL0    8
#define IRQ_SERIAL1    9
#define IRQ_EXT_HOST   10
#define IRQ_DSPHINT    11
#define IRQ_UART0      12
#define IRQ_UART1      13
#define IRQ_USB_DMA    14
#define IRQ_USB_CORE   15
#define IRQ_VLYNQ      16
#define IRQ_MTC0       17
#define IRQ_MTC1       18
#define IRQ_SD_MMC     19
#define IRQ_SDIO_MS    20
#define IRQ_GIO0       21
#define IRQ_GIO1       22
#define IRQ_GIO2       23
#define IRQ_GIO3       24
#define IRQ_GIO4       25
#define IRQ_GIO5       26
#define IRQ_GIO6       27
#define IRQ_GIO7       28
#define IRQ_GIO8       29
#define IRQ_GIO9       30
#define IRQ_GIO10      31
#define IRQ_GIO11      32
#define IRQ_GIO12      33
#define IRQ_GIO13      34
#define IRQ_GIO14      35
#define IRQ_GIO15      36
#define IRQ_PREVIEW0   37
#define IRQ_PREVIEW1   38
#define IRQ_WATCHDOG   39
#define IRQ_I2C        40
#define IRQ_CLKC       41

/* Embedded Debugging Interrupts */
#define IRQ_ICE        42
#define IRQ_ARMCOM_RX  43
#define IRQ_ARMCOM_TX  44

#define IRQ_RESERVED   45

#define NR_IRQS        46

/*  Taken from linux/include/asm-arm/arch-integrator/timex.h
 *
 *  Copyright (C) 1999 ARM Limited
 */

#define CONFIG_TIMER0_TMMD_STOP        0x0000
#define CONFIG_TIMER0_TMMD_ONE_SHOT    0x0001
#define CONFIG_TIMER0_TMMD_FREE_RUN    0x0002

#define CONFIG_TIMER1_TMMD_STOP        0x0000
#define CONFIG_TIMER1_TMMD_ONE_SHOT    0x0001
#define CONFIG_TIMER1_TMMD_FREE_RUN    0x0002

#define CONFIG_TIMER2_TMMD_STOP        0x0000
#define CONFIG_TIMER2_TMMD_ONE_SHOT    0x0001
#define CONFIG_TIMER2_TMMD_FREE_RUN    0x0002
#define CONFIG_TIMER2_TMMD_CCD_SHUTTER 0x0100
#define CONFIG_TIMER2_TMMD_CCD_STROBE  0x0200
#define CONFIG_TIMER2_TMMD_POLARITY    0x0400
#define CONFIG_TIMER2_TMMD_TRG_SELECT  0x0800
#define CONFIG_TIMER2_TMMD_TRG_READY   0x1000
#define CONFIG_TIMER2_TMMD_SIGNAL      0x2000

#define CONFIG_TIMER3_TMMD_STOP        0x0000
#define CONFIG_TIMER3_TMMD_ONE_SHOT    0x0001
#define CONFIG_TIMER3_TMMD_FREE_RUN    0x0002
#define CONFIG_TIMER3_TMMD_CCD_SHUTTER 0x0100
#define CONFIG_TIMER3_TMMD_CCD_STROBE  0x0200
#define CONFIG_TIMER3_TMMD_POLARITY    0x0400
#define CONFIG_TIMER3_TMMD_TRG_SELECT  0x0800
#define CONFIG_TIMER3_TMMD_TRG_READY   0x1000
#define CONFIG_TIMER3_TMMD_SIGNAL      0x2000

/*
 *  IO_MODx bits
 */
#define CLK_MOD0_HPIB                  (1 << 11)
#define CLK_MOD0_DSP                   (1 << 10)
#define CLK_MOD0_EXTHOST               (1 << 9)
#define CLK_MOD0_SDRAMC                (1 << 8)
#define CLK_MOD0_EMIF                  (1 << 7)
#define CLK_MOD0_INTC                  (1 << 6)
#define CLK_MOD0_AIM                   (1 << 5)
#define CLK_MOD0_E2ICE                 (1 << 4)
#define CLK_MOD0_ETM                   (1 << 3)
#define CLK_MOD0_AHB                   (1 << 2)
#define CLK_MOD0_BUSC                  (1 << 1)
#define CLK_MOD0_ARM                   (1 << 0)

#define CLK_MOD1_CPBUS                 (1 << 11)
#define CLK_MOD1_SEQ                   (1 << 10)
#define CLK_MOD1_DCT                   (1 << 9)
#define CLK_MOD1_IMGBUF                (1 << 8)
#define CLK_MOD1_IMX                   (1 << 7)
#define CLK_MOD1_VLCD                  (1 << 6)
#define CLK_MOD1_DAC                   (1 << 5)
#define CLK_MOD1_VENC                  (1 << 4)
#define CLK_MOD1_OSD                   (1 << 3)
#define CLK_MOD1_PRV                   (1 << 2)
#define CLK_MOD1_H3A                   (1 << 1)
#define CLK_MOD1_CCDC                  (1 << 0)

#define CLK_MOD2_TEST                  (1 << 15)
#define CLK_MOD2_MS                    (1 << 14)
#define CLK_MOD2_VLYNQ                 (1 << 13)
#define CLK_MOD2_I2C                   (1 << 12)
#define CLK_MOD2_MMC                   (1 << 11)
#define CLK_MOD2_SIF1                  (1 << 10)
#define CLK_MOD2_SIF0                  (1 << 9)
#define CLK_MOD2_UART1                 (1 << 8)
#define CLK_MOD2_UART0                 (1 << 7)
#define CLK_MOD2_USB                   (1 << 6)
#define CLK_MOD2_GIO                   (1 << 5)
#define CLK_MOD2_CCDTMR1               (1 << 4)
#define CLK_MOD2_CCDTMR0               (1 << 3)
#define CLK_MOD2_TMR1                  (1 << 2)
#define CLK_MOD2_TMR0                  (1 << 1)
#define CLK_MOD2_WDT                   (1 << 0)

#define CLK_SEL1_OSD                   (1 << 12)
#define CLK_SEL1_CCD                   (1 << 8)
#define CLK_SEL1_VENCPLL               (1 << 4)
#define CLK_SEL1_VENC(x)               (x << 0)

#define CLK_OSEL_O2SEL(x)              (x << 8)
#define CLK_OSEL_O1SEL(x)              (x << 4)
#define CLK_OSEL_O0SEL(x)              (x << 0)

#define CLK_BYP_AXL                    (1 << 12)
#define CLK_BYP_SDRAM                  (1 << 8)
#define CLK_BYP_DSP                    (1 << 4)
#define CLK_BYP_ARM                    (1 << 0)

/*
 *  IO_EINTx bits
 */
#define INTR_EINT0_USB1                (1 << 15)
#define INTR_EINT0_USB0                (1 << 14)
#define INTR_EINT0_UART1               (1 << 13)
#define INTR_EINT0_UART0               (1 << 12)
#define INTR_EINT0_IMGBUF              (1 << 11)
#define INTR_EINT0_EXTHOST             (1 << 10)
#define INTR_EINT0_SP1                 (1 << 9)
#define INTR_EINT0_SP0                 (1 << 8)
#define INTR_EINT0_VENC                (1 << 7)
#define INTR_EINT0_CCDWEN              (1 << 6)
#define INTR_EINT0_CCDVD1              (1 << 5)
#define INTR_EINT0_CCDVD0              (1 << 4)
#define INTR_EINT0_TMR3                (1 << 3)
#define INTR_EINT0_TMR2                (1 << 2)
#define INTR_EINT0_TMR1                (1 << 1)
#define INTR_EINT0_TMR0                (1 << 0)

#define INTR_EINT1_EXT10               (1 << 15)
#define INTR_EINT1_EXT9                (1 << 14)
#define INTR_EINT1_EXT8                (1 << 13)
#define INTR_EINT1_EXT7                (1 << 12)
#define INTR_EINT1_EXT6                (1 << 11)
#define INTR_EINT1_EXT5                (1 << 10)
#define INTR_EINT1_EXT4                (1 << 9)
#define INTR_EINT1_EXT3                (1 << 8)
#define INTR_EINT1_EXT2                (1 << 7)
#define INTR_EINT1_EXT1                (1 << 6)
#define INTR_EINT1_EXT0                (1 << 5)
#define INTR_EINT1_MMCSDMS1            (1 << 4)
#define INTR_EINT1_MMCSDMS0            (1 << 3)
#define INTR_EINT1_MTC1                (1 << 2)
#define INTR_EINT1_MTC0                (1 << 1)
#define INTR_EINT1_VLYNQ               (1 << 0)

#define INTR_EINT2_RSVINT              (1 << 13)
#define INTR_EINT2_ARMCOMTX            (1 << 12)
#define INTR_EINT2_ARMCOMRX            (1 << 11)
#define INTR_EINT2_E2ICE               (1 << 10)
#define INTR_EINT2_INTRC               (1 << 9)
#define INTR_EINT2_I2C                 (1 << 8)
#define INTR_EINT2_WDT                 (1 << 7)
#define INTR_EINT2_PREV1               (1 << 6)
#define INTR_EINT2_PREV0               (1 << 5)
#define INTR_EINT2_EXT15               (1 << 4)
#define INTR_EINT2_EXT14               (1 << 3)
#define INTR_EINT2_EXT13               (1 << 2)
#define INTR_EINT2_EXT12               (1 << 1)
#define INTR_EINT2_EXT11               (1 << 0)

/*
* IO_IRQx bits
*/
#define INTR_IRQ0_TMR0        INTR_EINT0_TMR0
#define INTR_IRQ0_TMR1        INTR_EINT0_TMR1
#define INTR_IRQ0_TMR2        INTR_EINT0_TMR2
#define INTR_IRQ0_TMR3        INTR_EINT0_TMR3
#define INTR_IRQ0_UART1       INTR_EINT0_UART1
#define INTR_IRQ0_CCDVD1      INTR_EINT0_CCDVD1
#define INTR_IRQ0_IMGBUF      INTR_EINT0_IMGBUF

#define INTR_IRQ1_EXT0        INTR_EINT1_EXT0
#define INTR_IRQ1_EXT2        INTR_EINT1_EXT2
#define INTR_IRQ1_EXT7        INTR_EINT1_EXT7
#define INTR_IRQ1_MTC0        INTR_EINT1_MTC0

/*
* HPIBCTL bits
*/
#define HPIBCTL_DBIO          (1 << 10)
#define HPIBCTL_DHOLD         (1 << 9)
#define HPIBCTL_DRST          (1 << 8)
#define HPIBCTL_DINT0         (1 << 7)
#define HPIBCTL_EXCHG         (1 << 5)
#define HPIBCTL_HPNMI         (1 << 3)
#define HPIBCTL_HPIEN         (1 << 0)

/*
* Video Encoder bits
*/
#define VENC_VMOD_VDMD(x)     (x << 12)
#define VENC_VMOD_ITLC        (1 << 10)
#define VENC_VMOD_CBTYP       (1 << 9)
#define VENC_VMOD_CBMD        (1 << 8)
#define VENC_VMOD_NTPLS(x)    (x << 6)
#define VENC_VMOD_SLAVE       (1 << 5)
#define VENC_VMOD_VMD         (1 << 4)
#define VENC_VMOD_BLNK        (1 << 3)
#define VENC_VMOD_DACPD       (1 << 2)
#define VENC_VMOD_VIE         (1 << 1)
#define VENC_VMOD_VENC        (1 << 0)

#define VENC_VDCTL_VCLKP      (1 << 14)
#define VENC_VDCTL_VCLKE      (1 << 13)
#define VENC_VDCTL_VCLKZ      (1 << 12)
#define VENC_VDCTL_DOMD(x)    (x << 4)
#define VENC_VDCTL_YCDC       (1 << 2)
#define VENC_VDCTL_INPTRU     (1 << 1)
#define VENC_VDCTL_YCDIR      (1 << 0)

#define VENC_VDPRO_PFLTY(x)   (x << 12)
#define VENC_VDPRO_PFLTR      (1 << 11)
#define VENC_VDPRO_YCDLY(x)   (x << 8)
#define VENC_VDPRO_RGBMAT     (1 << 7)
#define VENC_VDPRO_ATRGB      (1 << 6)
#define VENC_VDPRO_ATYCC      (1 << 5)
#define VENC_VDPRO_ATCOM      (1 << 4)
#define VENC_VDPRO_STUP       (1 << 3)
#define VENC_VDPRO_CRCUT      (1 << 2)
#define VENC_VDPRO_CUPS       (1 << 1)
#define VENC_VDPRO_YUPS       (1 << 0)

#define VENC_SYNCTL_EXFEN     (1 << 12)
#define VENC_SYNCTL_EXFIV     (1 << 11)
#define VENC_SYNCTL_EXSYNC    (1 << 10)
#define VENC_SYNCTL_EXVIV     (1 << 9)
#define VENC_SYNCTL_EXHIV     (1 << 8)
#define VENC_SYNCTL_CSP       (1 << 7)
#define VENC_SYNCTL_CSE       (1 << 6)
#define VENC_SYNCTL_SYSW      (1 << 5)
#define VENC_SYNCTL_VSYNCS    (1 << 4)
#define VENC_SYNCTL_VPL       (1 << 3)
#define VENC_SYNCTL_HPL       (1 << 2)
#define VENC_SYNCTL_SYE       (1 << 1)
#define VENC_SYNCTL_SYDIR     (1 << 0)

#define VENC_RGBCTL_IRONM     (1 << 11)
#define VENC_RGBCTL_DFLTR     (1 << 10)
#define VENC_RGBCTL_DFLTS(x)  (x << 8)
#define VENC_RGBCTL_RGBEF(x)  (x << 4)
#define VENC_RGBCTL_RGBOF(x)  (x << 0)

#define VENC_RGBCLP_UCLIP(x)  (x << 8)
#define VENC_RGBCLP_OFST(x)   (x << 0)

#define VENC_LCDOUT_FIDS      (1 << 8)
#define VENC_LCDOUT_FIDP      (1 << 7)
#define VENC_LCDOUT_PWMP      (1 << 6)
#define VENC_LCDOUT_PWME      (1 << 5)
#define VENC_LCDOUT_ACE       (1 << 4)
#define VENC_LCDOUT_BRP       (1 << 3)
#define VENC_LCDOUT_BRE       (1 << 2)
#define VENC_LCDOUT_OEP       (1 << 1)
#define VENC_LCDOUT_OEE       (1 << 0)

#define VENC_DCLKCTL_DOFST(x) (x << 12)
#define VENC_DCLKCTL_DCKEC    (1 << 11)
#define VENC_DCLKCTL_DCKME    (1 << 10)
#define VENC_DCLKCTL_DCKOH    (1 << 9)
#define VENC_DCLKCTL_DCKIH    (1 << 8)
#define VENC_DCLKCTL_DCKPW(x) (x << 0)

#endif
