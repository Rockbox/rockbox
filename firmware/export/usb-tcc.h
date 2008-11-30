/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Vitja Makarov
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
#ifndef USB_TCC7XX_H
#define USB_TCC7XX_H

#define MMR_REG16(base, x)  (*(volatile unsigned short *) ((base) + (x)))

/* USB PHY registers */
#define TCC7xx_USB_PHY_CFG       MMR_REG16(USB_BASE, 0xc4)
    #define TCC7xx_USB_PHY_CFG_XSEL  (1<<13) /* FS/HS Transceiver enable */
    #define TCC7xx_USB_PHY_CFG_DWS   (1<<6)  /* Host mode */
    #define TCC7xx_USB_PHY_XO        (1<<5)  /* Enable XO_OUT */
    #define TCC7xx_USB_PHY_CKSEL_12  0
    #define TCC7xx_USB_PHY_CKSEL_24  1
    #define TCC7xx_USB_PHY_CKSEL_48  2

/* USB 2.0 device registers */
#define TCC7xx_USB_INDEX    MMR_REG16(USB_BASE, 0x00) /* Endpoint Index register */
#define TCC7xx_USB_EPIF     MMR_REG16(USB_BASE, 0x04) /* Endpoint interrupt flag register */
#define TCC7xx_USB_EPIE     MMR_REG16(USB_BASE, 0x08) /* Endpoint interrupt enable register */
#define TCC7xx_USB_FUNC     MMR_REG16(USB_BASE, 0x0c) /* Function address register */
#define TCC7xx_USB_EP_DIR   MMR_REG16(USB_BASE, 0x14) /* Endpoint direction register */
#define TCC7xx_USB_TST      MMR_REG16(USB_BASE, 0x14) /* Test registerregister */
#define TCC7xx_USB_SYS_STAT MMR_REG16(USB_BASE, 0x1c) /* System status register */
    #define TCC7xx_USB_SYS_STAT_RESET   (1<<0) /* Host forced reced */
    #define TCC7xx_USB_SYS_STAT_SUSPEND (1<<1) /* Host forced suspend */
    #define TCC7xx_USB_SYS_STAT_RESUME  (1<<2) /* Host forced resume */
    #define TCC7xx_USB_SYS_STAT_HIGH    (1<<4) /* High speed */
    #define TCC7xx_USB_SYS_STAT_SPD_END (1<<6) /* Speed detection end */
    #define TCC7xx_USB_SYS_STAT_VBON    (1<<8)
    #define TCC7xx_USB_SYS_STAT_VBOF    (1<<9)
    #define TCC7xx_USB_SYS_STAT_EOERR   (1<<10) /* overrun error */
    #define TCC7xx_USB_SYS_STAT_DCERR   (1<<11) /* Data CRC error */
    #define TCC7xx_USB_SYS_STAT_TCERR   (1<<12) /* Token CRC error */
    #define TCC7xx_USB_SYS_STAT_BSERR   (1<<13) /* Bit-stuff error */
    #define TCC7xx_USB_SYS_STAT_TMERR   (1<<14) /* Timeout error */
    #define TCC7xx_USB_SYS_STAT_BAERR   (1<<15) /* Byte align error */

#define TCC7xx_USB_SYS_STAT_ERRORS (TCC7xx_USB_SYS_STAT_EOERR | \
                                    TCC7xx_USB_SYS_STAT_DCERR | \
                                    TCC7xx_USB_SYS_STAT_TCERR | \
                                    TCC7xx_USB_SYS_STAT_BSERR | \
                                    TCC7xx_USB_SYS_STAT_TMERR | \
                                    TCC7xx_USB_SYS_STAT_BAERR)

#define TCC7xx_USB_SYS_CTRL MMR_REG16(USB_BASE, 0x20) /* System control register */
    #define TCC7xx_USB_SYS_CTRL_RESET   (1<<0) /* Reset enable */
    #define TCC7xx_USB_SYS_CTRL_SUSPEND (1<<1) /* Suspend enable */
    #define TCC7xx_USB_SYS_CTRL_RESUME  (1<<2) /* Resume enable */
    #define TCC7xx_USB_SYS_CTRL_IPS     (1<<4) /* Interrupt polarity */
    #define TCC7xx_USB_SYS_CTRL_RFRE    (1<<5) /* Reverse read data enable */
    #define TCC7xx_USB_SYS_CTRL_SPDEN   (1<<6) /* Speed detection interrupt enable */
    #define TCC7xx_USB_SYS_CTRL_BUS16   (1<<7) /* Select bus width 8/16 */
    #define TCC7xx_USB_SYS_CTRL_EIEN    (1<<8) /* Error interrupt enable */
    #define TCC7xx_USB_SYS_CTRL_RWDE    (1<<9) /* Reverse write data enable */
    #define TCC7xx_USB_SYS_CTRL_VBONE   (1<<10) /* VBus On enable */
    #define TCC7xx_USB_SYS_CTRL_VBOFE   (1<<11) /* VBus Off enable */
    #define TCC7xx_USB_SYS_CTRL_DUAL    (1<<12) /* Dual interrupt enable*/
    #define TCC7xx_USB_SYS_CTRL_DMAZ    (1<<14) /* DMA total count zero int */

#define TCC7xx_USB_EP0_STAT MMR_REG16(USB_BASE, 0x24) /* EP0 status register */
#define TCC7xx_USB_EP0_CTRL MMR_REG16(USB_BASE, 0x28) /* EP0 control register */

#define TCC7xx_USB_EP0_BUF  MMR_REG16(USB_BASE, 0x60) /* EP0 buffer register */
#define TCC7xx_USB_EP1_BUF  MMR_REG16(USB_BASE, 0x64) /* EP1 buffer register */
#define TCC7xx_USB_EP2_BUF  MMR_REG16(USB_BASE, 0x68) /* EP2 buffer register */
#define TCC7xx_USB_EP3_BUF  MMR_REG16(USB_BASE, 0x6c) /* EP3 buffer register */

/* Indexed registers,  write endpoint number to TCC7xx_USB_INDEX */
#define TCC7xx_USB_EP_STAT  MMR_REG16(USB_BASE, 0x2c) /* EP status register */
    #define TCC7xx_USP_EP_STAT_RPS   (1 << 0) /* Packet received */
    #define TCC7xx_USP_EP_STAT_TPS   (1 << 1) /* Packet transmited */
    #define TCC7xx_USP_EP_STAT_LWO   (1 << 4) /* Last word odd */
#define TCC7xx_USB_EP_CTRL  MMR_REG16(USB_BASE, 0x30) /* EP control register */
    #define TCC7xx_USB_EP_CTRL_TZLS  (1 << 0)  /* TX Zero Length Set */
    #define TCC7xx_USB_EP_CTRL_ESS   (1 << 1)  /* Endpoint Stall Set */
    #define TCC7xx_USB_EP_CTRL_CDP   (1 << 2)  /* Clear Data PID */
    #define TCC7xx_USB_EP_CTRL_TTE   (1 << 5)  /* TX Toggle Enable */
    #define TCC7xx_USB_EP_CTRL_FLUSH (1 << 6)  /* Flush FIFO */
    #define TCC7xx_USB_EP_CTRL_DUEN  (1 << 7)  /* Dual FIFO Mode */
    #define TCC7xx_USB_EP_CTRL_IME   (1 << 8)  /* ISO Mode */
    #define TCC7xx_USB_EP_CTRL_OUTHD (1 << 11) /* OUT Packet Hold */
    #define TCC7xx_USB_EP_CTRL_INHLD (1 << 12) /* IN Packet Hold */

#define TCC7xx_USB_EP_BRCR  MMR_REG16(USB_BASE, 0x34) /* EP byte read count register */
#define TCC7xx_USB_EP_BWCR  MMR_REG16(USB_BASE, 0x38) /* EP byte write count register */
#define TCC7xx_USB_EP_MAXP  MMR_REG16(USB_BASE, 0x3c) /* EP max packet register */

#define TCC7xx_USB_EP_DMA_CTRL  MMR_REG16(USB_BASE, 0x40) /* EP DMA control register */
#define TCC7xx_USB_EP_DMA_TCNTR MMR_REG16(USB_BASE, 0x44) /* EP DMA transfer counter register */
#define TCC7xx_USB_EP_DMA_FCNTR MMR_REG16(USB_BASE, 0x48) /* EP DMA fifo counter register */
#define TCC7xx_USB_EP_DMA_TTCNTR1 MMR_REG16(USB_BASE, 0x4c) /* EP DMA total trasfer counter1 register */
#define TCC7xx_USB_EP_DMA_TTCNTR2 MMR_REG16(USB_BASE, 0x50) /* EP DMA total trasfer counter2 register */
#define TCC7xx_USB_EP_DMA_ADDR1 MMR_REG16(USB_BASE, 0xa0) /* EP DMA MCU addr1 register */
#define TCC7xx_USB_EP_DMA_ADDR2 MMR_REG16(USB_BASE, 0xa4) /* EP DMA MCU addr2 register */
#define TCC7xx_USB_EP_DMA_STAT  MMR_REG16(USB_BASE, 0xc0) /* EP DMA Transfer Status register */
#define TCC7xx_USB_DELAY_CTRL   MMR_REG16(USB_BASE, 0x80)	/* Delay control register */
#endif /* USB_TCC7XX_H */
