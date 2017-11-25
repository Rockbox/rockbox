/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#include "usb_drv.h"
#include "config.h"
#include "memory.h"
#include "target.h"

//extern uint32_t USB_BASE;
#define USB_BASE        0xf0010000
//uint32_t TIMER_BASE = 0xF3003000;
#define PCLK_BASE   0xf3000000

/* the following only applies to TCC78x */
#define BCLKCTR     (*(volatile unsigned long *)(PCLK_BASE + 0x18))
#define SWRESET     (*(volatile unsigned long *)(PCLK_BASE + 0x1c))
/* applies to both BCLKCTR and SWRESET */
#define DEV_USBD    (1<<1)

#define MMR_REG16(base, x)  (*(volatile unsigned short *) ((base) + (x)))

/* USB PHY registers */
#define USB_PHY_CFG       MMR_REG16(USB_BASE, 0xc4)
    #define USB_PHY_CFG_XSEL  (1<<13) /* FS/HS Transceiver enable */
    #define USB_PHY_CFG_DWS   (1<<6)  /* Host mode */
    #define USB_PHY_XO        (1<<5)  /* Enable XO_OUT */
    #define USB_PHY_CKSEL_12  0
    #define USB_PHY_CKSEL_24  1
    #define USB_PHY_CKSEL_48  2

/* USB 2.0 device registers */
#define USB_INDEX    MMR_REG16(USB_BASE, 0x00) /* Endpoint Index register */
#define USB_EPIF     MMR_REG16(USB_BASE, 0x04) /* Endpoint interrupt flag register */
#define USB_EPIE     MMR_REG16(USB_BASE, 0x08) /* Endpoint interrupt enable register */
#define USB_FUNC     MMR_REG16(USB_BASE, 0x0c) /* Function address register */
#define USB_EP_DIR   MMR_REG16(USB_BASE, 0x14) /* Endpoint direction register */
#define USB_TST      MMR_REG16(USB_BASE, 0x14) /* Test registerregister */
#define USB_SYS_STAT MMR_REG16(USB_BASE, 0x1c) /* System status register */
    #define USB_SYS_STAT_RESET   (1<<0) /* Host forced reced */
    #define USB_SYS_STAT_SUSPEND (1<<1) /* Host forced suspend */
    #define USB_SYS_STAT_RESUME  (1<<2) /* Host forced resume */
    #define USB_SYS_STAT_HIGH    (1<<4) /* High speed */
    #define USB_SYS_STAT_SPD_END (1<<6) /* Speed detection end */
    #define USB_SYS_STAT_VBON    (1<<8)
    #define USB_SYS_STAT_VBOF    (1<<9)
    #define USB_SYS_STAT_EOERR   (1<<10) /* overrun error */
    #define USB_SYS_STAT_DCERR   (1<<11) /* Data CRC error */
    #define USB_SYS_STAT_TCERR   (1<<12) /* Token CRC error */
    #define USB_SYS_STAT_BSERR   (1<<13) /* Bit-stuff error */
    #define USB_SYS_STAT_TMERR   (1<<14) /* Timeout error */
    #define USB_SYS_STAT_BAERR   (1<<15) /* Byte align error */

#define USB_SYS_STAT_ERRORS (USB_SYS_STAT_EOERR | \
                                    USB_SYS_STAT_DCERR | \
                                    USB_SYS_STAT_TCERR | \
                                    USB_SYS_STAT_BSERR | \
                                    USB_SYS_STAT_TMERR | \
                                    USB_SYS_STAT_BAERR)

#define USB_SYS_CTRL MMR_REG16(USB_BASE, 0x20) /* System control register */
    #define USB_SYS_CTRL_RESET   (1<<0) /* Reset enable */
    #define USB_SYS_CTRL_SUSPEND (1<<1) /* Suspend enable */
    #define USB_SYS_CTRL_RESUME  (1<<2) /* Resume enable */
    #define USB_SYS_CTRL_IPS     (1<<4) /* Interrupt polarity */
    #define USB_SYS_CTRL_RFRE    (1<<5) /* Reverse read data enable */
    #define USB_SYS_CTRL_SPDEN   (1<<6) /* Speed detection interrupt enable */
    #define USB_SYS_CTRL_BUS16   (1<<7) /* Select bus width 8/16 */
    #define USB_SYS_CTRL_EIEN    (1<<8) /* Error interrupt enable */
    #define USB_SYS_CTRL_RWDE    (1<<9) /* Reverse write data enable */
    #define USB_SYS_CTRL_VBONE   (1<<10) /* VBus On enable */
    #define USB_SYS_CTRL_VBOFE   (1<<11) /* VBus Off enable */
    #define USB_SYS_CTRL_DUAL    (1<<12) /* Dual interrupt enable*/
    #define USB_SYS_CTRL_DMAZ    (1<<14) /* DMA total count zero int */

#define USB_EP0_STAT MMR_REG16(USB_BASE, 0x24) /* EP0 status register */
    #define USB_EP0_STAT_RSR    (1 << 0)
    #define USB_EP0_STAT_TST    (1 << 1)
    #define USB_EP0_STAT_SHT    (1 << 4)
#define USB_EP0_CTRL MMR_REG16(USB_BASE, 0x28) /* EP0 control register */

#define USB_EP0_BUF  MMR_REG16(USB_BASE, 0x60) /* EP0 buffer register */
#define USB_EP1_BUF  MMR_REG16(USB_BASE, 0x64) /* EP1 buffer register */
#define USB_EP2_BUF  MMR_REG16(USB_BASE, 0x68) /* EP2 buffer register */
#define USB_EP3_BUF  MMR_REG16(USB_BASE, 0x6c) /* EP3 buffer register */

/* Indexed registers,  write endpoint number to USB_INDEX */
#define USB_EP_STAT  MMR_REG16(USB_BASE, 0x2c) /* EP status register */
    #define USP_EP_STAT_RPS   (1 << 0) /* Packet received */
    #define USP_EP_STAT_TPS   (1 << 1) /* Packet transmited */
    #define USP_EP_STAT_LWO   (1 << 4) /* Last word odd */
#define USB_EP_CTRL  MMR_REG16(USB_BASE, 0x30) /* EP control register */
    #define USB_EP_CTRL_TZLS  (1 << 0)  /* TX Zero Length Set */
    #define USB_EP_CTRL_ESS   (1 << 1)  /* Endpoint Stall Set */
    #define USB_EP_CTRL_CDP   (1 << 2)  /* Clear Data PID */
    #define USB_EP_CTRL_TTE   (1 << 5)  /* TX Toggle Enable */
    #define USB_EP_CTRL_FLUSH (1 << 6)  /* Flush FIFO */
    #define USB_EP_CTRL_DUEN  (1 << 7)  /* Dual FIFO Mode */
    #define USB_EP_CTRL_IME   (1 << 8)  /* ISO Mode */
    #define USB_EP_CTRL_OUTHD (1 << 11) /* OUT Packet Hold */
    #define USB_EP_CTRL_INHLD (1 << 12) /* IN Packet Hold */

#define USB_EP_BRCR  MMR_REG16(USB_BASE, 0x34) /* EP byte read count register */
#define USB_EP_BWCR  MMR_REG16(USB_BASE, 0x38) /* EP byte write count register */
#define USB_EP_MAXP  MMR_REG16(USB_BASE, 0x3c) /* EP max packet register */

#define USB_EP_DMA_CTRL  MMR_REG16(USB_BASE, 0x40) /* EP DMA control register */
#define USB_EP_DMA_TCNTR MMR_REG16(USB_BASE, 0x44) /* EP DMA transfer counter register */
#define USB_EP_DMA_FCNTR MMR_REG16(USB_BASE, 0x48) /* EP DMA fifo counter register */
#define USB_EP_DMA_TTCNTR1 MMR_REG16(USB_BASE, 0x4c) /* EP DMA total trasfer counter1 register */
#define USB_EP_DMA_TTCNTR2 MMR_REG16(USB_BASE, 0x50) /* EP DMA total trasfer counter2 register */
#define USB_EP_DMA_ADDR1 MMR_REG16(USB_BASE, 0xa0) /* EP DMA MCU addr1 register */
#define USB_EP_DMA_ADDR2 MMR_REG16(USB_BASE, 0xa4) /* EP DMA MCU addr2 register */
#define USB_EP_DMA_STAT  MMR_REG16(USB_BASE, 0xc0) /* EP DMA Transfer Status register */
#define USB_DELAY_CTRL   MMR_REG16(USB_BASE, 0x80)   /* Delay control register */

static inline void pullup_on(void)
{
    USB_PHY_CFG = 0x000c;
}

static inline void pullup_off(void)
{
    USB_PHY_CFG = 0x3e4c;
}

static void usb_reset(void)
{
    USB_DELAY_CTRL |= 0x81;

    USB_SYS_CTRL = 0xa000 |
        USB_SYS_CTRL_RESET |
        USB_SYS_CTRL_RFRE  |
        USB_SYS_CTRL_SPDEN |
        USB_SYS_CTRL_VBONE |
        USB_SYS_CTRL_VBOFE;
    /* setup endpoints */
    USB_EP0_CTRL = 0;
    USB_EP_DIR = 0;
    USB_INDEX = 0;
    pullup_on();
    /* enable interrupts for EP0 */
    USB_EPIF = 1 << 0;
    USB_EPIE = 1 << 0s;
}

static void do_reset(void)
{
    usb_reset();
    USB_SYS_STAT = USB_SYS_STAT_RESET;
}

void usb_drv_init(void)
{
    /* enable USB device block */
    BCLKCTR |= DEV_USBD;
    pullup_off();
    /* reset USB device */
    SWRESET |= DEV_USBD;
    target_udelay(50);
    SWRESET &= ~DEV_USBD;

    usb_reset();

int usb_drv_recv_setup(struct usb_ctrlrequest *req)
{
    if(USB_SYS_STAT & USB_SYS_STAT_RESET)
        do_reset();
    while(!(USB_EPIF & (1 << 0))) {}
    USB_INDEX = 0;
    if(USB_EP0_STAT & USB_EP0_STAT_SHT)
        USB_EP0_STAT = USB_EP0_STAT_SHT
    if(USB_EP0_STAT & USB_EP0_STAT_RSR)
    {
        if(USB_EP0_STAT & USB_EP0_STAT_TST)
            USB_EP0_STAT = USB_EP0_STAT_TST;
        int count = USB_EP_BRCR;
        if(count == 4)
        {
            uint16_t *ptr = (void *)req;
            for(i = 0; i < 4; i++)
                ptr[i] = USB_EP0_BUF;
        }
        else
        {
            /* flush fifo */
            for(i = 0; i < count; i++)
                ptr[0] = USB_EP0_BUF;
        }
        USB_EP0_STAT = USB_EP0_STAT_RSR;
    }
    else if(USB_EP0_STAT & USB_EP0_STAT_TST)
    {
        USB_EP0_STAT = USB_EP0_STAT_TST;
    }
    USB_EPIF = 1 << 0;
    return 0;
}

int usb_drv_port_speed(void)
{
    return (USB_SYS_STAT & USB_SYS_STAT_HIGH) ? 1 : 0;
}

void usb_drv_set_address(int address)
{
}

int usb_drv_send(int endpoint, void *ptr, int length)
{
    return 0;
}

int usb_drv_recv(int endpoint, void* ptr, int length)
{
    return 0;
}

void usb_drv_stall(int endpoint, bool stall, bool in)
{
    (void) endpoint;
    (void) in;
}

void usb_drv_exit(void)
{
    BCLKCTR &= ~DEV_USBD;
    SWRESET |= DEV_USBD;
    target_udelay(50);
    SWRESET &= ~DEV_USBD;

    pullup_off();
}
