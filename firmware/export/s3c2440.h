/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Marcoen Hirschberg
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
#ifndef __S3C2440_H__
#define __S3C2440_H__

#define LCD_BUFFER_SIZE (320*240*2)
#define TTB_SIZE (0x4000)
/* must be 16Kb (0x4000) aligned */
#define TTB_BASE_ADDR (0x30000000 + (32*1024*1024) - TTB_SIZE)
#define TTB_BASE   ((unsigned long *)TTB_BASE_ADDR) /* End of memory */
#define FRAME   ((unsigned short *)(TTB_BASE_ADDR - LCD_BUFFER_SIZE))  /* Right before TTB */

/* Memory Controllers */

#define BWSCON (*(volatile unsigned long *)0x48000000) /* Bus width & wait status control */
#define BANKCON0 (*(volatile unsigned long *)0x48000004) /* Boot ROM control */
#define BANKCON1 (*(volatile unsigned long *)0x48000008) /* BANK1 control */
#define BANKCON2 (*(volatile unsigned long *)0x4800000C) /* BANK2 control */
#define BANKCON3 (*(volatile unsigned long *)0x48000010) /* BANK3 control */
#define BANKCON4 (*(volatile unsigned long *)0x48000014) /* BANK4 control */
#define BANKCON5 (*(volatile unsigned long *)0x48000018) /* BANK5 control */
#define BANKCON6 (*(volatile unsigned long *)0x4800001C) /* BANK6 control */
#define BANKCON7 (*(volatile unsigned long *)0x48000020) /* BANK7 control */
#define REFRESH (*(volatile unsigned long *)0x48000024) /* DRAM/SDRAM refresh control */
#define BANKSIZE (*(volatile unsigned long *)0x48000028) /* Flexible bank size */
#define MRSRB6 (*(volatile unsigned long *)0x4800002C) /* Mode register set for SDRAM BANK6 */
#define MRSRB7 (*(volatile unsigned long *)0x48000030) /* Mode register set for SDRAM BANK7 */

/* USB Host Controller */

/* Control and status group */
#define HcRevision (*(volatile unsigned long *)0x49000000)
#define HcControl (*(volatile unsigned long *)0x49000004)
#define HcCommonStatus (*(volatile unsigned long *)0x49000008)
#define HcInterruptStatus (*(volatile unsigned long *)0x4900000C)
#define HcInterruptEnable (*(volatile unsigned long *)0x49000010)
#define HcInterruptDisable (*(volatile unsigned long *)0x49000014)
/* Memory pointer group */
#define HcHCCA (*(volatile unsigned long *)0x49000018)
#define HcPeriodCuttentED (*(volatile unsigned long *)0x4900001C)
#define HcControlHeadED (*(volatile unsigned long *)0x49000020)
#define HcControlCurrentED (*(volatile unsigned long *)0x49000024)
#define HcBulkHeadED (*(volatile unsigned long *)0x49000028)
#define HcBulkCurrentED (*(volatile unsigned long *)0x4900002C)
/* Frame counter group */
#define HcDoneHead (*(volatile unsigned long *)0x49000030)
#define HcRmInterval (*(volatile unsigned long *)0x49000034)
#define HcFmRemaining (*(volatile unsigned long *)0x49000038)
#define HcFmNumber (*(volatile unsigned long *)0x4900003C)
#define HcPeriodicStart (*(volatile unsigned long *)0x49000040)
#define HcLSThreshold (*(volatile unsigned long *)0x49000044)
/* Root hub group */
#define HcRhDescriptorA (*(volatile unsigned long *)0x49000048)
#define HcRhDescriptorB (*(volatile unsigned long *)0x4900004C)
#define HcRhStatus (*(volatile unsigned long *)0x49000050)
#define HcRhPortStatus1 (*(volatile unsigned long *)0x49000054)
#define HcRhPortStatus2 (*(volatile unsigned long *)0x49000058)

/* Interrupt Controller */

#define SRCPND (*(volatile unsigned long *)0x4A000000) /* Interrupt request status */
#define INTMOD (*(volatile unsigned long *)0x4A000004) /* Interrupt mode control */
#define INTMSK (*(volatile unsigned long *)0x4A000008) /* Interrupt mask control */
#define PRIORITY (*(volatile unsigned long *)0x4A00000C) /* IRQ priority control */
#define INTPND (*(volatile unsigned long *)0x4A000010) /* Interrupt request status */
#define INTOFFSET (*(volatile unsigned long *)0x4A000014) /* Interrupt request source offset */
#define SUBSRCPND (*(volatile unsigned long *)0x4A000018) /* Sub source pending */
#define INTSUBMSK (*(volatile unsigned long *)0x4A00001C) /* Interrupt sub mask */

/* Interrupt indexes - INTOFFSET - IRQ mode only */
/* Arbiter 5 => Arbiter 6 Req 5 */
#define ADC_INTOFFSET       31 /* REQ4 */
#define RTC_INTOFFSET       30 /* REQ3 */
#define SPI1_INTOFFSET      29 /* REQ2 */
#define UART0_INTOFFSET     28 /* REQ1 */
/* Arbiter 4 => Arbiter 6 Req 4 */
#define IIC_INTOFFSET       27 /* REQ5 */
#define USBH_INTOFFSET      26 /* REQ4 */
#define USBD_INTOFFSET      25 /* REQ3 */
#define NFCON_INTOFFSET     24 /* REQ2 */
#define UART1_INTOFFSET     23 /* REQ1 */
#define SPI0_INTOFFSET      22 /* REQ0 */
/* Arbiter 3 => Arbiter 6 Req 3  */
#define SDI_INTOFFSET       21 /* REQ5 */
#define DMA3_INTOFFSET      20 /* REQ4 */
#define DMA2_INTOFFSET      19 /* REQ3 */
#define DMA1_INTOFFSET      18 /* REQ2 */
#define DMA0_INTOFFSET      17 /* REQ1 */
#define LCD_INTOFFSET       16 /* REQ0 */
/* Arbiter 2 => Arbiter 6 Req 2 */
#define UART2_INTOFFSET     15 /* REQ5 */
#define TIMER4_INTOFFSET    14 /* REQ4 */
#define TIMER3_INTOFFSET    13 /* REQ3 */
#define TIMER2_INTOFFSET    12 /* REQ2 */
#define TIMER1_INTOFFSET    11 /* REQ1 */
#define TIMER0_INTOFFSET    10 /* REQ0 */
/* Arbiter 1 => Arbiter 6 Req 1  */
#define WDT_AC97_INTOFFSET   9 /* REQ5 */
#define TICK_INTOFFSET       8 /* REQ4 */
#define nBATT_FLT_INTOFFSET  7 /* REQ3 */
#define CAM_INTOFFSET        6 /* REQ2 */
#define EINT8_23_INTOFFSET   5 /* REQ1 */
#define EINT4_7_INTOFFSET    4 /* REQ0 */
/* Arbiter 0  => Arbiter 6 Req 0 */
#define EINT3_INTOFFSET      3 /* REQ4 */
#define EINT2_INTOFFSET      2 /* REQ3 */
#define EINT1_INTOFFSET      1 /* REQ2 */
#define EINT0_INTOFFSET      0 /* REQ1 */

/* Interrupt bitmasks - SRCPND, INTMOD, INTMSK, INTPND */
/* Arbiter 5 => Arbiter 6 Req 5 */
#define ADC_MASK       (1 << 31) /* REQ4 */
#define RTC_MASK       (1 << 30) /* REQ3 */
#define SPI1_MASK      (1 << 29) /* REQ2 */
#define UART0_MASK     (1 << 28) /* REQ1 */
/* Arbiter 4 => Arbiter 6 Req 4 */
#define IIC_MASK       (1 << 27) /* REQ5 */
#define USBH_MASK      (1 << 26) /* REQ4 */
#define USBD_MASK      (1 << 25) /* REQ3 */
#define NFCON_MASK     (1 << 24) /* REQ2 */
#define UART1_MASK     (1 << 23) /* REQ1 */
#define SPI0_MASK      (1 << 22) /* REQ0 */
/* Arbiter 3 => Arbiter 6 Req 3  */
#define SDI_MASK       (1 << 21) /* REQ5 */
#define DMA3_MASK      (1 << 20) /* REQ4 */
#define DMA2_MASK      (1 << 19) /* REQ3 */
#define DMA1_MASK      (1 << 18) /* REQ2 */
#define DMA0_MASK      (1 << 17) /* REQ1 */
#define LCD_MASK       (1 << 16) /* REQ0 */
/* Arbiter 2 => Arbiter 6 Req 2 */
#define UART2_MASK     (1 << 15) /* REQ5 */
#define TIMER4_MASK    (1 << 14) /* REQ4 */
#define TIMER3_MASK    (1 << 13) /* REQ3 */
#define TIMER2_MASK    (1 << 12) /* REQ2 */
#define TIMER1_MASK    (1 << 11) /* REQ1 */
#define TIMER0_MASK    (1 << 10) /* REQ0 */
/* Arbiter 1 => Arbiter 6 Req 1  */
#define WDT_AC97_MASK  (1 <<  9) /* REQ5 */
#define TICK_MASK      (1 <<  8) /* REQ4 */
#define nBATT_FLT_MASK (1 <<  7) /* REQ3 */
#define CAM_MASK       (1 <<  6) /* REQ2 */
#define EINT8_23_MASK  (1 <<  5) /* REQ1 */
#define EINT4_7_MASK   (1 <<  4) /* REQ0 */
/* Arbiter 0  => Arbiter 6 Req 0 */
#define EINT3_MASK     (1 <<  3) /* REQ4 */
#define EINT2_MASK     (1 <<  2) /* REQ3 */
#define EINT1_MASK     (1 <<  1) /* REQ2 */
#define EINT0_MASK     (1 <<  0) /* REQ1 */

/* DMA */

#define DISRC0 (*(volatile unsigned long *)0x4B000000) /* DMA 0 initial source */
#define DISRCC0 (*(volatile unsigned long *)0x4B000004) /* DMA 0 initial source control */
#define DIDST0 (*(volatile unsigned long *)0x4B000008) /* DMA 0 initial destination */
#define DIDSTC0 (*(volatile unsigned long *)0x4B00000C) /* DMA 0 initial destination control */
#define DCON0 (*(volatile unsigned long *)0x4B000010) /* DMA 0 control */
#define DSTAT0 (*(volatile unsigned long *)0x4B000014) /* DMA 0 count */
#define DCSRC0 (*(volatile unsigned long *)0x4B000018) /* DMA 0 current source */
#define DCDST0 (*(volatile unsigned long *)0x4B00001C) /* DMA 0 current destination */
#define DMASKTRIG0 (*(volatile unsigned long *)0x4B000020) /* DMA 0 mask trigger */
#define DISRC1 (*(volatile unsigned long *)0x4B000040) /* DMA 1 initial source */
#define DISRCC1 (*(volatile unsigned long *)0x4B000044) /* DMA 1 initial source control */
#define DIDST1 (*(volatile unsigned long *)0x4B000048) /* DMA 1 initial destination */
#define DIDSTC1 (*(volatile unsigned long *)0x4B00004C) /* DMA 1 initial destination control */
#define DCON1 (*(volatile unsigned long *)0x4B000050) /* DMA 1 control */
#define DSTAT1 (*(volatile unsigned long *)0x4B000054) /* DMA 1 count */
#define DCSRC1 (*(volatile unsigned long *)0x4B000058) /* DMA 1 current source */
#define DCDST1 (*(volatile unsigned long *)0x4B00005C) /* DMA 1 current destination */
#define DMASKTRIG1 (*(volatile unsigned long *)0x4B000060) /* DMA 1 mask trigger */
#define DISRC2 (*(volatile unsigned long *)0x4B000080) /* DMA 2 initial source */
#define DISRCC2 (*(volatile unsigned long *)0x4B000084) /* DMA 2 initial source control */
#define DIDST2 (*(volatile unsigned long *)0x4B000088) /* DMA 2 initial destination */
#define DIDSTC2 (*(volatile unsigned long *)0x4B00008C) /* DMA 2 initial destination control */
#define DCON2 (*(volatile unsigned long *)0x4B000090) /* DMA 2 control */
#define DSTAT2 (*(volatile unsigned long *)0x4B000094) /* DMA 2 count */
#define DCSRC2 (*(volatile unsigned long *)0x4B000098) /* DMA 2 current source */
#define DCDST2 (*(volatile unsigned long *)0x4B00009C) /* DMA 2 current destination */
#define DMASKTRIG2 (*(volatile unsigned long *)0x4B0000A0) /* DMA 2 mask trigger */
#define DISRC3 (*(volatile unsigned long *)0x4B0000C0) /* DMA 3 initial source */
#define DISRCC3 (*(volatile unsigned long *)0x4B0000C4) /* DMA 3 initial source control */
#define DIDST3 (*(volatile unsigned long *)0x4B0000C8) /* DMA 3 initial destination */
#define DIDSTC3 (*(volatile unsigned long *)0x4B0000CC) /* DMA 3 initial destination control */
#define DCON3 (*(volatile unsigned long *)0x4B0000D0) /* DMA 3 control */
#define DSTAT3 (*(volatile unsigned long *)0x4B0000D4) /* DMA 3 count */
#define DCSRC3 (*(volatile unsigned long *)0x4B0000D8) /* DMA 3 current source */
#define DCDST3 (*(volatile unsigned long *)0x4B0000DC) /* DMA 3 current destination */
#define DMASKTRIG3 (*(volatile unsigned long *)0x4B0000E0) /* DMA 3 mask trigger */

/* Clock & Power Management */

#define LOCKTIME (*(volatile unsigned long *)0x4C000000) /* PLL lock time counter */
#define MPLLCON (*(volatile unsigned long *)0x4C000004) /* MPLL control */
#define UPLLCON (*(volatile unsigned long *)0x4C000008) /* UPLL control */
#define CLKCON (*(volatile unsigned long *)0x4C00000C) /* Clock generator control */
#define CLKSLOW (*(volatile unsigned long *)0x4C000010) /* Slow clock control */
#define CLKDIVN (*(volatile unsigned long *)0x4C000014) /* Clock divider control */
#define CAMDIVN (*(volatile unsigned long *)0x4C000018) /* Camera clock divider control */

/* LCD Controller */

#define LCDCON1 (*(volatile unsigned long *)0x4D000000) /* LCD control 1 */
#define LCDCON2 (*(volatile unsigned long *)0x4D000004) /* LCD control 2 */
#define LCDCON3 (*(volatile unsigned long *)0x4D000008) /* LCD control 3 */
#define LCDCON4 (*(volatile unsigned long *)0x4D00000C) /* LCD control 4 */
#define LCDCON5 (*(volatile unsigned long *)0x4D000010) /* LCD control 5 */
#define LCDSADDR1 (*(volatile unsigned long *)0x4D000014) /* STN/TFT: frame buffer start address 1 */
#define LCDSADDR2 (*(volatile unsigned long *)0x4D000018) /* STN/TFT: frame buffer start address 2 */
#define LCDSADDR3 (*(volatile unsigned long *)0x4D00001C) /* STN/TFT: virtual screen address set */
#define REDLUT (*(volatile unsigned long *)0x4D000020) /* STN: red lookup table */
#define GREENLUT (*(volatile unsigned long *)0x4D000024) /* STN: green lookup table */
#define BLUELUT (*(volatile unsigned long *)0x4D000028) /* STN: blue lookup table */
#define DITHMODE (*(volatile unsigned long *)0x4D00004C) /* STN: dithering mode */
#define TPAL (*(volatile unsigned long *)0x4D000050) /* TFT: temporary palette */
#define LCDINTPND (*(volatile unsigned long *)0x4D000054) /* LCD interrupt pending */
#define LCDSRCPND (*(volatile unsigned long *)0x4D000058) /* LCD interrupt source */
#define LCDINTMSK (*(volatile unsigned long *)0x4D00005C) /* LCD interrupt mask */
#define TCONSEL (*(volatile unsigned long *)0x4D000060) /* TCON(LPC3600/LCC3600) control */

/* NAND Flash */

#define NFCONF (*(volatile unsigned long *)0x4E000000) /* NAND flash configuration */
#define NFCONT (*(volatile unsigned long *)0x4E000004) /* NAND flash control */
#define NFCMD (*(volatile unsigned long *)0x4E000008) /* NAND flash command */
#define NFADDR (*(volatile unsigned long *)0x4E00000C) /* NAND flash address */
#define NFDATA (*(volatile unsigned long *)0x4E000010) /* NAND flash data */
#define NFMECC0 (*(volatile unsigned long *)0x4E000014) /* NAND flash main area ECC0/1 */
#define NFMECC1 (*(volatile unsigned long *)0x4E000018) /* NAND flash main area ECC2/3 */
#define NFSECC (*(volatile unsigned long *)0x4E00001C) /* NAND flash spare area ECC */
#define NFSTAT (*(volatile unsigned long *)0x4E000020) /* NAND flash operation status */
#define NFESTAT0 (*(volatile unsigned long *)0x4E000024) /* NAND flash ECC status for I/O[7:0] */
#define NFESTAT1 (*(volatile unsigned long *)0x4E000028) /* NAND flash ECC status for I/O[15:8] */
#define NFMECCSTAT0 (*(volatile unsigned long *)0x4E00002C) /* NAND flash main area ECC0 status */
#define NFMECCSTAT1 (*(volatile unsigned long *)0x4E000030) /* NAND flash main area ECC1 status */
#define NFSECCSTAT (*(volatile unsigned long *)0x4E000034) /* NAND flash spare area ECC status */
#define NFSBLK (*(volatile unsigned long *)0x4E000038) /* NAND flash start block address */
#define NFEBLK (*(volatile unsigned long *)0x4E00003C) /* NAND flash end block address */

/* Camera Interface */

#define CISRCFMT (*(volatile unsigned long *)0x4F000000) /* Input source format */
#define CIWDOFST (*(volatile unsigned long *)0x4F000004) /* Window offset register */
#define CIGCTRL (*(volatile unsigned long *)0x4F000008) /* Global control register */
#define CICOYSA1 (*(volatile unsigned long *)0x4F000018) /* Y 1st frame start address for codec DMA */
#define CICOYSA2 (*(volatile unsigned long *)0x4F00001C) /* Y 2nd frame start address for codec DMA */
#define CICOYSA3 (*(volatile unsigned long *)0x4F000020) /* Y 3nd frame start address for codec DMA */
#define CICOYSA4 (*(volatile unsigned long *)0x4F000024) /* Y 4th frame start address for codec DMA */
#define CICOCBSA1 (*(volatile unsigned long *)0x4F000028) /* Cb 1st frame start address for codec DMA */
#define CICOCBSA2 (*(volatile unsigned long *)0x4F00002C) /* Cb 2nd frame start address for codec DMA */
#define CICOCBSA3 (*(volatile unsigned long *)0x4F000030) /* Cb 3nd frame start address for codec DMA */
#define CICOCBSA4 (*(volatile unsigned long *)0x4F000034) /* Cb 4th frame start address for codec DMA */
#define CICOCRSA1 (*(volatile unsigned long *)0x4F000038) /* Cr 1st frame start address for codec DMA */
#define CICOCRSA2 (*(volatile unsigned long *)0x4F00003C) /* Cr 2nd frame start address for codec DMA */
#define CICOCRSA3 (*(volatile unsigned long *)0x4F000040) /* Cr 3nd frame start address for codec DMA */
#define CICOCRSA4 (*(volatile unsigned long *)0x4F000044) /* Cr 4th frame start address for codec DMA */
#define CICOTRGFMT (*(volatile unsigned long *)0x4F000048) /* Target image format of codec DMA */
#define CICOCTRL (*(volatile unsigned long *)0x4F00004C)

/* Codec DMA control related */

#define CICOSCPRERATIO (*(volatile unsigned long *)0x4F000050) /* Codec pre-scaler ratio control */
#define CICOSCPREDST (*(volatile unsigned long *)0x4F000054) /* Codec pre-scaler destination format */
#define CICOSCCTRL (*(volatile unsigned long *)0x4F000058) /* Codec main-scaler control */
#define CICOTAREA (*(volatile unsigned long *)0x4F00005C) /* Codec scaler target area */
#define CICOSTATUS (*(volatile unsigned long *)0x4F000064) /* Codec path status */
#define CIPRCLRSA1 (*(volatile unsigned long *)0x4F00006C) /* RGB 1st frame start address for preview DMA */
#define CIPRCLRSA2 (*(volatile unsigned long *)0x4F000070) /* RGB 2nd frame start address for preview DMA */
#define CIPRCLRSA3 (*(volatile unsigned long *)0x4F000074) /* RGB 3nd frame start address for preview DMA */
#define CIPRCLRSA4 (*(volatile unsigned long *)0x4F000078) /* RGB 4th frame start address for preview DMA */
#define CIPRTRGFMT (*(volatile unsigned long *)0x4F00007C) /* Target image format of preview DMA */
#define CIPRCTRL (*(volatile unsigned long *)0x4F000080) /* Preview DMA control related */
#define CIPRSCPRERATIO (*(volatile unsigned long *)0x4F000084) /* Preview pre-scaler ratio control */
#define CIPRSCPREDST (*(volatile unsigned long *)0x4F000088) /* Preview pre-scaler destination format */
#define CIPRSCCTRL (*(volatile unsigned long *)0x4F00008C) /* Preview main-scaler control */
#define CIPRTAREA (*(volatile unsigned long *)0x4F000090) /* Preview scaler target area */
#define CIPRSTATUS (*(volatile unsigned long *)0x4F000098) /* Preview path status */
#define CIIMGCPT (*(volatile unsigned long *)0x4F0000A0) /* Image capture enable command */

/* UART */

#define ULCON0 (*(volatile unsigned long *)0x50000000) /* UART 0 line control */
#define UCON0 (*(volatile unsigned long *)0x50000004) /* UART 0 control */
#define UFCON0 (*(volatile unsigned long *)0x50000008) /* UART 0 FIFO control */
#define UMCON0 (*(volatile unsigned long *)0x5000000C) /* UART 0 modem control */
#define UTRSTAT0 (*(volatile unsigned long *)0x50000010) /* UART 0 Tx/Rx status */
#define UERSTAT0 (*(volatile unsigned long *)0x50000014) /* UART 0 Rx error status */
#define UFSTAT0 (*(volatile unsigned long *)0x50000018) /* UART 0 FIFO status */
#define UMSTAT0 (*(volatile unsigned long *)0x5000001C) /* UART 0 modem status */
#define UTXH0 (*(volatile unsigned char *)0x50000020) /* UART 0 transmission hold */
#define URXH0 (*(volatile unsigned char *)0x50000024) /* UART 0 receive buffer */
#define UBRDIV0 (*(volatile unsigned long *)0x50000028) /* UART 0 baud rate divisor */
#define ULCON1 (*(volatile unsigned long *)0x50004000) /* UART 1 line control */
#define UCON1 (*(volatile unsigned long *)0x50004004) /* UART 1 control */
#define UFCON1 (*(volatile unsigned long *)0x50004008) /* UART 1 FIFO control */
#define UMCON1 (*(volatile unsigned long *)0x5000400C) /* UART 1 modem control */
#define UTRSTAT1 (*(volatile unsigned long *)0x50004010) /* UART 1 Tx/Rx status */
#define UERSTAT1 (*(volatile unsigned long *)0x50004014) /* UART 1 Rx error status */
#define UFSTAT1 (*(volatile unsigned long *)0x50004018) /* UART 1 FIFO status */
#define UMSTAT1 (*(volatile unsigned long *)0x5000401C) /* UART 1 modem status */
#define UTXH1 (*(volatile unsigned char*)0x50004020) /* UART 1 transmission hold */
#define URXH1 (*(volatile unsigned char*)0x50004024) /* UART 1 receive buffer */
#define UBRDIV1 (*(volatile unsigned long *)0x50004028) /* UART 1 baud rate divisor */
#define ULCON2 (*(volatile unsigned long *)0x50008000) /* UART 2 line control */
#define UCON2 (*(volatile unsigned long *)0x50008004) /* UART 2 control */
#define UFCON2 (*(volatile unsigned long *)0x50008008) /* UART 2 FIFO control */
#define UTRSTAT2 (*(volatile unsigned long *)0x50008010) /* UART 2 Tx/Rx status */
#define UERSTAT2 (*(volatile unsigned long *)0x50008014) /* UART 2 Rx error status */
#define UFSTAT2 (*(volatile unsigned long *)0x50008018) /* UART 2 FIFO status */
#define UTXH2 (*(volatile unsigned char*)0x50008020) /* UART 2 transmission hold */
#define URXH2 (*(volatile unsigned char*)0x50008024) /* UART 2 receive buffer */
#define UBRDIV2 (*(volatile unsigned long *)0x50008028) /* UART 2 baud rate divisor */

/* PWM Timer */

#define TCFG0 (*(volatile unsigned long *)0x51000000) /* Timer configuration */
#define TCFG1 (*(volatile unsigned long *)0x51000004) /* Timer configuration */
#define TCON (*(volatile unsigned long *)0x51000008) /* Timer control */
#define TCNTB0 (*(volatile unsigned long *)0x5100000C) /* Timer count buffer 0 */
#define TCMPB0 (*(volatile unsigned long *)0x51000010) /* Timer compare buffer 0 */
#define TCNTO0 (*(volatile unsigned long *)0x51000014) /* Timer count observation 0 */
#define TCNTB1 (*(volatile unsigned long *)0x51000018) /* Timer count buffer 1 */
#define TCMPB1 (*(volatile unsigned long *)0x5100001C) /* Timer compare buffer 1 */
#define TCNTO1 (*(volatile unsigned long *)0x51000020) /* Timer count observation 1 */
#define TCNTB2 (*(volatile unsigned long *)0x51000024) /* Timer count buffer 2 */
#define TCMPB2 (*(volatile unsigned long *)0x51000028) /* Timer compare buffer 2 */
#define TCNTO2 (*(volatile unsigned long *)0x5100002C) /* Timer count observation 2 */
#define TCNTB3 (*(volatile unsigned long *)0x51000030) /* Timer count buffer 3 */
#define TCMPB3 (*(volatile unsigned long *)0x51000034) /* Timer compare buffer 3 */
#define TCNTO3 (*(volatile unsigned long *)0x51000038) /* Timer count observation 3 */
#define TCNTB4 (*(volatile unsigned long *)0x5100003C) /* Timer count buffer 4 */
#define TCNTO4 (*(volatile unsigned long *)0x51000040) /* Timer count observation 4 */

/* USB Device */

#define FUNC_ADDR_REG (*(volatile unsigned char *)0x52000140) /* Function address */
#define PWR_REG (*(volatile unsigned char *)0x52000144) /* Power management */
#define EP_INT_REG (*(volatile unsigned char *)0x52000148) /* EP interrupt pending and clear */
#define USB_INT_REG (*(volatile unsigned char *)0x52000158) /* USB interrupt pending and clear */
#define EP_INT_EN_REG (*(volatile unsigned char *)0x5200015C) /* Interrupt enable */
#define USB_INT_EN_REG (*(volatile unsigned char *)0x5200016C) /* Interrupt enable */
#define FRAME_NUM1_REG (*(volatile unsigned char *)0x52000170) /* Frame number lower byte */
#define FRAME_NUM2_REG (*(volatile unsigned char *)0x52000174) /* Frame number higher byte */
#define INDEX_REG (*(volatile unsigned char *)0x52000178) /* Register index */
#define EP0_CSR (*(volatile unsigned char *)0x52000184) /* Endpoint 0 status */
#define IN_CSR1_REG (*(volatile unsigned char *)0x52000184) /* In endpoint control status */
#define IN_CSR2_REG (*(volatile unsigned char *)0x52000188) /* In endpoint control status */
#define MAXP_REG (*(volatile unsigned char *)0x52000180) /* Endpoint max packet */
#define OUT_CSR1_REG (*(volatile unsigned char *)0x52000190) /* Out endpoint control status */
#define OUT_CSR2_REG (*(volatile unsigned char *)0x52000194) /* Out endpoint control status */
#define OUT_FIFO_CNT1_REG (*(volatile unsigned char *)0x52000198) /* Endpoint out write count */
#define OUT_FIFO_CNT2_REG (*(volatile unsigned char *)0x5200019C) /* Endpoint out write count */
#define EP0_FIFO (*(volatile unsigned char *)0x520001C0) /* Endpoint 0 FIFO */
#define EP1_FIFO (*(volatile unsigned char *)0x520001C4) /* Endpoint 1 FIFO */
#define EP2_FIFO (*(volatile unsigned char *)0x520001C8) /* Endpoint 2 FIFO */
#define EP3_FIFO (*(volatile unsigned char *)0x520001CC) /* Endpoint 3 FIFO */
#define EP4_FIFO (*(volatile unsigned char *)0x520001D0) /* Endpoint 4 FIFO */
#define EP1_DMA_CON (*(volatile unsigned char *)0x52000200) /* EP1 DMA Interface control */
#define EP1_DMA_UNIT (*(volatile unsigned char *)0x52000204) /* EP1 DMA Tx unit counter */
#define EP1_DMA_FIFO (*(volatile unsigned char *)0x52000208) /* EP1 DMA Tx FIFO counter */
#define EP1_DMA_TTC_L (*(volatile unsigned char *)0x5200020C) /* EP1 DMA Total Tx counter */
#define EP1_DMA_TTC_M (*(volatile unsigned char *)0x52000210) /* EP1 DMA Total Tx counter */
#define EP1_DMA_TTC_H (*(volatile unsigned char *)0x52000214) /* EP1 DMA Total Tx counter */
#define EP2_DMA_CON (*(volatile unsigned char *)0x52000218) /* EP2 DMA interface control */
#define EP2_DMA_UNIT (*(volatile unsigned char *)0x5200021C) /* EP2 DMA Tx Unit counter */
#define EP2_DMA_FIFO (*(volatile unsigned char *)0x52000220) /* EP2 DMA Tx FIFO counter */
#define EP2_DMA_TTC_L (*(volatile unsigned char *)0x52000224) /* EP2 DMA total Tx counter */
#define EP2_DMA_TTC_M (*(volatile unsigned char *)0x52000228) /* EP2 DMA total Tx counter */
#define EP2_DMA_TTC_H (*(volatile unsigned char *)0x5200022C) /* EP2 DMA Total Tx counter */
#define EP3_DMA_CON (*(volatile unsigned char *)0x52000240) /* EP3 DMA Interface control */
#define EP3_DMA_UNIT (*(volatile unsigned char *)0x52000244) /* EP3 DMA Tx Unit counter */
#define EP3_DMA_FIFO (*(volatile unsigned char *)0x52000248) /* EP3 DMA Tx FIFO counter */
#define EP3_DMA_TTC_L (*(volatile unsigned char *)0x5200024C) /* EP3 DMA Total Tx counter */
#define EP3_DMA_TTC_M (*(volatile unsigned char *)0x52000250) /* EP3 DMA Total Tx counter */
#define EP3_DMA_TTC_H (*(volatile unsigned char *)0x52000254) /* EP3 DMA Total Tx counter */
#define EP4_DMA_CON (*(volatile unsigned char *)0x52000258) /* EP4 DMA Interface control */
#define EP4_DMA_UNIT (*(volatile unsigned char *)0x5200025C) /* EP4 DMA Tx Unit counter */
#define EP4_DMA_FIFO (*(volatile unsigned char *)0x52000260) /* EP4 DMA Tx FIFO counter */
#define EP4_DMA_TTC_L (*(volatile unsigned char *)0x52000264) /* EP4 DMA Total Tx counter */
#define EP4_DMA_TTC_M (*(volatile unsigned char *)0x52000268) /* EP4 DMA Total Tx counter */
#define EP4_DMA_TTC_H (*(volatile unsigned char *)0x5200026C) /* EP4 DMA Total Tx counter */

/* Watchdog Timer */

#define WTCON (*(volatile unsigned long *)0x53000000) /* Watchdog timer mode */
#define WTDAT (*(volatile unsigned long *)0x53000004) /* Watchdog timer data */
#define WTCNT (*(volatile unsigned long *)0x53000008) /* Watchdog timer count */

/* IIC */

#define IICCON (*(volatile unsigned long *)0x54000000) /* IIC control  */
#define IICSTAT (*(volatile unsigned long *)0x54000004) /* IIC status */
#define IICADD (*(volatile unsigned long *)0x54000008) /* IIC address */
#define IICDS (*(volatile unsigned long *)0x5400000C) /* IIC data shift */
#define IICLC (*(volatile unsigned long *)0x54000010) /* IIC multi-master line control */

/* IIS */

#define IISCON (*(volatile unsigned long *)0x55000000) /* IIS control */
#define IISMOD (*(volatile unsigned long *)0x55000004) /* IIS mode */
#define IISPSR (*(volatile unsigned long *)0x55000008) /* IIS prescaler */
#define IISFCON (*(volatile unsigned long *)0x5500000C) /* IIS FIFO control */
#define IISFIFO (*(volatile unsigned short *)0x55000010) /* IIS FIFO entry */

/* I/O port */

#define GPACON (*(volatile unsigned long *)0x56000000) /* Port A control */
#define GPADAT (*(volatile unsigned long *)0x56000004) /* Port A data */
#define GPBCON (*(volatile unsigned long *)0x56000010) /* Port B control */
#define GPBDAT (*(volatile unsigned long *)0x56000014) /* Port B data */
#define GPBUP (*(volatile unsigned long *)0x56000018) /* Pull-up control B */
#define GPCCON (*(volatile unsigned long *)0x56000020) /* Port C control */
#define GPCDAT (*(volatile unsigned long *)0x56000024) /* Port C data */
#define GPCUP (*(volatile unsigned long *)0x56000028) /* Pull-up control C */
#define GPDCON (*(volatile unsigned long *)0x56000030) /* Port D control */
#define GPDDAT (*(volatile unsigned long *)0x56000034) /* Port D data */
#define GPDUP (*(volatile unsigned long *)0x56000038) /* Pull-up control D */
#define GPECON (*(volatile unsigned long *)0x56000040) /* Port E control */
#define GPEDAT (*(volatile unsigned long *)0x56000044) /* Port E data */
#define GPEUP (*(volatile unsigned long *)0x56000048) /* Pull-up control E */
#define GPFCON (*(volatile unsigned long *)0x56000050) /* Port F control */
#define GPFDAT (*(volatile unsigned long *)0x56000054) /* Port F data */
#define GPFUP (*(volatile unsigned long *)0x56000058) /* Pull-up control F */
#define GPGCON (*(volatile unsigned long *)0x56000060) /* Port G control */
#define GPGDAT (*(volatile unsigned long *)0x56000064) /* Port G data */
#define GPGUP (*(volatile unsigned long *)0x56000068) /* Pull-up control G */
#define GPHCON (*(volatile unsigned long *)0x56000070) /* Port H control */
#define GPHDAT (*(volatile unsigned long *)0x56000074) /* Port H data */
#define GPHUP (*(volatile unsigned long *)0x56000078) /* Pull-up control H */
#define MISCCR (*(volatile unsigned long *)0x56000080) /* Miscellaneous control */
#define DCLKCON (*(volatile unsigned long *)0x56000084) /* DCLK0/1 control */
#define EXTINT0 (*(volatile unsigned long *)0x56000088) /* External interrupt control register 0 */
#define EXTINT1 (*(volatile unsigned long *)0x5600008C) /* External interrupt control register 1 */
#define EXTINT2 (*(volatile unsigned long *)0x56000090) /* External interrupt control register 2 */
#define EINTFLT0 (*(volatile unsigned long *)0x56000094) /* Reserved */
#define EINTFLT1 (*(volatile unsigned long *)0x56000098) /* Reserved */
#define EINTFLT2 (*(volatile unsigned long *)0x5600009C) /* External interrupt filter control register 2 */
#define EINTFLT3 (*(volatile unsigned long *)0x560000A0) /* External interrupt filter control register 3 */
#define EINTMASK (*(volatile unsigned long *)0x560000A4) /* External interrupt mask */
#define EINTPEND (*(volatile unsigned long *)0x560000A8) /* External interrupt pending */
#define GSTATUS0 (*(volatile unsigned long *)0x560000AC) /* External pin status */
#define GSTATUS1 (*(volatile unsigned long *)0x560000B0) /* Chip ID */
#define GSTATUS2 (*(volatile unsigned long *)0x560000B4) /* Reset status */
#define GSTATUS3 (*(volatile unsigned long *)0x560000B8) /* Inform register */
#define GSTATUS4 (*(volatile unsigned long *)0x560000BC) /* Inform register */
#define MSLCON (*(volatile unsigned long *)0x560000CC) /* Memory sleep control register */
#define GPJCON (*(volatile unsigned long *)0x560000D0) /* Port J control */
#define GPJDAT (*(volatile unsigned long *)0x560000D4) /* Port J data */
#define GPJUP (*(volatile unsigned long *)0x560000D8) /* Pull-up control J */

/* RTC */

#define RTCCON (*(volatile unsigned char *)0x57000040) /* RTC control */
#define TICNT (*(volatile unsigned char *)0x57000044) /* Tick time count */
#define RTCALM (*(volatile unsigned char *)0x57000050) /* RTC alarm control */
#define ALMSEC (*(volatile unsigned char *)0x57000054) /* Alarm second */
#define ALMMIN (*(volatile unsigned char *)0x57000058) /* Alarm minute */
#define ALMHOUR (*(volatile unsigned char *)0x5700005C) /* Alarm hour */
#define ALMDATE (*(volatile unsigned char *)0x57000060) /* alarm day */
#define ALMMON (*(volatile unsigned char *)0x57000064) /* Alarm month */
#define ALMYEAR (*(volatile unsigned char *)0x57000068) /* Alarm year */
#define BCDSEC (*(volatile unsigned char *)0x57000070) /* BCD second */
#define BCDMIN (*(volatile unsigned char *)0x57000074) /* BCD minute */
#define BCDHOUR (*(volatile unsigned char *)0x57000078) /* BCD hour */
#define BCDDATE (*(volatile unsigned char *)0x5700007C) /* BCD day */
#define BCDDAY (*(volatile unsigned char *)0x57000080) /* BCD date */
#define BCDMON (*(volatile unsigned char *)0x57000084) /* BCD month */
#define BCDYEAR (*(volatile unsigned char *)0x57000088) /* BCD year */

/* A/D Converter */

#define ADCCON (*(volatile unsigned long *)0x58000000) /* ADC control */
#define ADCTSC (*(volatile unsigned long *)0x58000004) /* ADC touch screen control */
#define ADCDLY (*(volatile unsigned long *)0x58000008) /* ADC start or interval delay */
#define ADCDAT0 (*(volatile unsigned long *)0x5800000C) /* ADC conversion data */
#define ADCDAT1 (*(volatile unsigned long *)0x58000010) /* ADC conversion data */
#define ADCUPDN (*(volatile unsigned long *)0x58000014) /* Stylus up or down interrupt status */

/* SPI */

#define SPCON0 (*(volatile unsigned long *)0x59000000) /* SPI control */
#define SPSTA0 (*(volatile unsigned long *)0x59000004) /* SPI status */
#define SPPIN0 (*(volatile unsigned long *)0x59000008) /* SPI pin control */
#define SPPRE0 (*(volatile unsigned long *)0x5900000C) /* SPI baud rate prescaler */
#define SPTDAT0 (*(volatile unsigned long *)0x59000010) /* SPI Tx data */
#define SPRDAT0 (*(volatile unsigned long *)0x59000014) /* SPI Rx data */
#define SPCON1 (*(volatile unsigned long *)0x59000020) /* SPI control */
#define SPSTA1 (*(volatile unsigned long *)0x59000024) /* SPI status */
#define SPPIN1 (*(volatile unsigned long *)0x59000028) /* SPI pin control */
#define SPPRE1 (*(volatile unsigned long *)0x5900002C) /* SPI baud rate prescaler */
#define SPTDAT1 (*(volatile unsigned long *)0x59000030) /* SPI Tx data */
#define SPRDAT1 (*(volatile unsigned long *)0x59000034) /* SPI Rx data */

/* SD Interface */

#define SDICON (*(volatile unsigned long *)0x5A000000) /* SDI control */
#define SDIPRE (*(volatile unsigned long *)0x5A000004) /* SDI baud rate prescaler */
#define SDICARG (*(volatile unsigned long *)0x5A000008) /* SDI command argument */
#define SDICCON (*(volatile unsigned long *)0x5A00000C) /* SDI command control */
#define SDICSTA (*(volatile unsigned long *)0x5A000010) /* SDI command status */
#define SDIRSP0 (*(volatile unsigned long *)0x5A000014) /* SDI response */
#define SDIRSP1 (*(volatile unsigned long *)0x5A000018) /* SDI response */
#define SDIRSP2 (*(volatile unsigned long *)0x5A00001C) /* SDI response */
#define SDIRSP3 (*(volatile unsigned long *)0x5A000020) /* SDI response */
#define SDIDTIMER (*(volatile unsigned long *)0x5A000024) /* SDI data / busy timer */
#define SDIBSIZE (*(volatile unsigned long *)0x5A000028) /* SDI block size */
#define SDIDCON (*(volatile unsigned long *)0x5A00002C) /* SDI data control */
#define SDIDCNT (*(volatile unsigned long *)0x5A000030) /* SDI data remain counter */
#define SDIDSTA (*(volatile unsigned long *)0x5A000034) /* SDI data status */
#define SDIFSTA (*(volatile unsigned long *)0x5A000038) /* SDI FIFO status */
#define SDIIMSK (*(volatile unsigned long *)0x5A00003C) /* SDI interrupt mask */
#define SDIDAT (*(volatile unsigned char *)0x5A000040) /* SDI data */

/* AC97 Audio-CODEC Interface */

#define AC_GLBCTRL (*(volatile unsigned long *)0x5B000000) /* AC97 global control register */
#define AC_GLBSTAT (*(volatile unsigned long *)0x5B000004) /* AC97 global status register */
#define AC_CODEC_CMD (*(volatile unsigned long *)0x5B000008) /* AC97 codec command register */
#define AC_CODEC_STAT (*(volatile unsigned long *)0x5B00000C) /* AC97 codec status register */
#define AC_PCMADDR (*(volatile unsigned long *)0x5B000010) /* AC97 PCM out/in channel FIFO address register */
#define AC_MICADDR (*(volatile unsigned long *)0x5B000014) /* AC97 mic in channel FIFO address register */
#define AC_PCMDATA (*(volatile unsigned long *)0x5B000018) /* AC97 PCM out/in channel FIFO data register */
#define AC_MICDATA (*(volatile unsigned long *)0x5B00001C) /* AC97 MIC in channel FIFO data register */

/* Memory banks */

#define BANK0 0x00000000
#define BANK1 0x08000000
#define BANK2 0x10000000
#define BANK3 0x18000000
#define BANK4 0x20000000
#define BANK5 0x28000000
#define DRAM0 0x30000000
#define DRAM1 0x31000000
#define BOOTRAM 0x40000000

#endif /* __S3C2440_H__ */
