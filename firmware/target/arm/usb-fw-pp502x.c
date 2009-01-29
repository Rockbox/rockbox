/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * iPod driver based on code from the ipodlinux project - http://ipodlinux.org
 * Adapted for Rockbox in January 2006
 * Original file: podzilla/usb.c
 * Copyright (C) 2005 Adam Johnston
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
#include "config.h"
#include "system.h"
#include "usb-target.h"
#include "usb.h"
#include "button.h"
#include "ata.h"
#include "string.h"
#include "usb_core.h"
#include "usb_drv.h"

#if defined(IPOD_4G) || defined(IPOD_COLOR) \
 || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    /* GPIO D bit 3 is usb detect */
#define USB_GPIO        GPIOD
#define USB_GPIO_MASK   0x08
#define USB_GPIO_VAL    0x08

#elif defined(IPOD_NANO) || defined(IPOD_VIDEO)
    /* GPIO L bit 4 is usb detect */
#define USB_GPIO        GPIOL
#define USB_GPIO_MASK   0x10
#define USB_GPIO_VAL    0x10

#elif defined(SANSA_C200)
    /* GPIO H bit 1 is usb/charger detect */
#define USB_GPIO        GPIOH
#define USB_GPIO_MASK   0x02
#define USB_GPIO_VAL    0x02

#elif defined(SANSA_E200)
    /* GPIO B bit 4 is usb/charger detect */
#define USB_GPIO        GPIOB
#define USB_GPIO_MASK   0x10
#define USB_GPIO_VAL    0x10

#elif defined(IRIVER_H10) || defined(IRIVER_H10_5GB) || defined(MROBE_100)
    /* GPIO L bit 2 is usb detect */
#define USB_GPIO        GPIOL
#define USB_GPIO_MASK   0x04
#define USB_GPIO_VAL    0x04

#elif defined(PHILIPS_SA9200)
    /* GPIO F bit 7 (low) is usb detect */
#define USB_GPIO        GPIOF
#define USB_GPIO_MASK   0x80
#define USB_GPIO_VAL    0x00

#elif defined(PHILIPS_HDD1630)
    /* GPIO E bit 2 is usb detect */
#define USB_GPIO        GPIOE
#define USB_GPIO_MASK   0x04
#define USB_GPIO_VAL    0x04
#else
#error No USB GPIO config specified
#endif

#define USB_GPIO_ENABLE      GPIO_ENABLE(USB_GPIO)
#define USB_GPIO_OUTPUT_EN   GPIO_OUTPUT_EN(USB_GPIO)
#define USB_GPIO_INPUT_VAL   GPIO_INPUT_VAL(USB_GPIO)
#define USB_GPIO_INT_EN      GPIO_INT_EN(USB_GPIO)
#define USB_GPIO_INT_LEV     GPIO_INT_LEV(USB_GPIO)
#define USB_GPIO_INT_CLR     GPIO_INT_CLR(USB_GPIO)
#define USB_GPIO_HI_INT_MASK GPIO_HI_INT_MASK(USB_GPIO)

void usb_init_device(void)
{
    /* enable usb module */
    outl(inl(0x7000002C) | 0x3000000, 0x7000002C);  

    DEV_EN |= DEV_USB0;
    DEV_EN |= DEV_USB1;

    /* reset both USBs */
    DEV_RS |= DEV_USB0;
    DEV_RS &=~DEV_USB0;
    DEV_RS |= DEV_USB1;
    DEV_RS &=~DEV_USB1;

    DEV_INIT2 |= INIT_USB;

    while ((inl(0x70000028) & 0x80) == 0);
    outl(inl(0x70000028) | 0x2, 0x70000028);
    udelay(100000);

    /* Do one-time inits */
    usb_drv_startup();

    /* disable USB-devices until USB is detected via GPIO */
#ifndef BOOTLOADER
    /* Disabling USB0 in the bootloader makes the OF not load,
       Also something here breaks usb pin detect in bootloader.
       leave it all enabled untill rockbox main loads */
    DEV_EN &= ~DEV_USB0;
    DEV_EN &= ~DEV_USB1;
    DEV_INIT2 &= ~INIT_USB;
#endif

    /* These set INV_LEV to the inserted level so it will fire if already
     * inserted at the time they are enabled. */
#ifdef USB_STATUS_BY_EVENT
    GPIO_CLEAR_BITWISE(USB_GPIO_INT_EN, USB_GPIO_MASK);
    GPIO_CLEAR_BITWISE(USB_GPIO_OUTPUT_EN, USB_GPIO_MASK);
    GPIO_SET_BITWISE(USB_GPIO_ENABLE, USB_GPIO_MASK);
    GPIO_WRITE_BITWISE(USB_GPIO_INT_LEV, USB_GPIO_VAL, USB_GPIO_MASK);
    USB_GPIO_INT_CLR = USB_GPIO_MASK;
    GPIO_SET_BITWISE(USB_GPIO_INT_EN, USB_GPIO_MASK);
    CPU_HI_INT_EN = USB_GPIO_HI_INT_MASK;

#ifdef USB_FIREWIRE_HANDLING
    /* GPIO C bit 1 is firewire detect */
    GPIO_CLEAR_BITWISE(GPIOC_INT_EN, 0x02);
    GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_EN, 0x02);
    GPIO_SET_BITWISE(GPIOC_ENABLE, 0x02);
    GPIO_WRITE_BITWISE(GPIOC_INT_LEV, 0x00, 0x02);
    GPIOC_INT_CLR = 0x02;
    GPIO_SET_BITWISE(GPIOC_INT_EN, 0x02);
    CPU_HI_INT_EN = GPIO0_MASK;
#endif
    CPU_INT_EN = HI_MASK;
#else
    /* No interrupt - setup pin read only (BOOTLOADER) */
    GPIO_CLEAR_BITWISE(USB_GPIO_OUTPUT_EN, USB_GPIO_MASK);
    GPIO_SET_BITWISE(USB_GPIO_ENABLE, USB_GPIO_MASK);
#ifdef USB_FIREWIRE_HANDLING
    /* GPIO C bit 1 is firewire detect */
    GPIO_CLEAR_BITWISE(GPIOC_OUTPUT_EN, 0x02);
    GPIO_SET_BITWISE(GPIOC_ENABLE, 0x02);
#endif
#endif /* USB_STATUS_BY_EVENT */
}

void usb_enable(bool on)
{
    if (on) {
        /* if USB is detected, re-enable the USB-devices, otherwise make sure it's disabled */
        DEV_EN |= DEV_USB0;
        DEV_EN |= DEV_USB1;
        DEV_INIT2 |= INIT_USB;
        usb_core_init();
    }
    else {
        usb_core_exit();
        /* Disable USB devices */
        DEV_EN &=~ DEV_USB0;
        DEV_EN &=~ DEV_USB1;
        DEV_INIT2 &=~ INIT_USB;
    }
}

void usb_attach(void)
{
    usb_drv_attach();
}

static bool usb_pin_state(void)
{
    return (USB_GPIO_INPUT_VAL & USB_GPIO_MASK) == USB_GPIO_VAL;
}

#ifdef USB_STATUS_BY_EVENT
/* Cannot always tell power pin from USB pin */
static int usb_status = USB_EXTRACTED;

static int usb_timeout_event(struct timeout *tmo)
{
    usb_status_event(tmo->data == USB_GPIO_VAL ? USB_POWERED : USB_UNPOWERED);
    return 0;
}

void usb_insert_int(void)
{
    static struct timeout usb_oneshot;
    unsigned long val = USB_GPIO_INPUT_VAL & USB_GPIO_MASK;
    usb_status = (val == USB_GPIO_VAL) ? USB_INSERTED : USB_EXTRACTED;
    GPIO_WRITE_BITWISE(USB_GPIO_INT_LEV, val ^ USB_GPIO_MASK, USB_GPIO_MASK);
    USB_GPIO_INT_CLR = USB_GPIO_MASK;
    timeout_register(&usb_oneshot, usb_timeout_event, HZ/5, val);
}

/* Called during the bus reset interrupt when in detect mode - filter for
 * invalid bus reset when unplugging by checking the pin state. */
void usb_drv_usb_detect_event(void)
{
    if(usb_pin_state()) {
        usb_status_event(USB_INSERTED);
    }
}
#endif /* USB_STATUS_BY_EVENT */

void usb_drv_int_enable(bool enable)
{
    /* enable/disable USB IRQ in CPU */
    if(enable) {
        CPU_INT_EN = USB_MASK;
    }
    else {
        CPU_INT_DIS = USB_MASK;
    }
}

/* detect host or charger (INSERTED or EXTRACTED) */
int usb_detect(void)
{
#ifdef USB_STATUS_BY_EVENT
    return usb_status;
#else
    return usb_pin_state() ? USB_INSERTED : USB_EXTRACTED;
#endif
}

#ifdef USB_FIREWIRE_HANDLING
#ifdef USB_STATUS_BY_EVENT
static bool firewire_status = false;
#endif

bool firewire_detect(void)
{
#ifdef USB_STATUS_BY_EVENT
    return firewire_status;
#else
    /* GPIO C bit 1 is firewire detect */
    /* no charger detection needed for firewire */
    return (GPIOC_INPUT_VAL & 0x02) == 0x00;
#endif
}

#ifdef USB_STATUS_BY_EVENT
static int firewire_timeout_event(struct timeout *tmo)
{
    if (tmo->data == 0x00)
        usb_firewire_connect_event();
    return 0;
}

void firewire_insert_int(void)
{
    static struct timeout firewire_oneshot;
    unsigned long val = GPIOC_INPUT_VAL & 0x02;
    firewire_status = val == 0x00;
    GPIO_WRITE_BITWISE(GPIOC_INT_LEV, val ^ 0x02, 0x02);
    GPIOC_INT_CLR = 0x02;
    timeout_register(&firewire_oneshot, firewire_timeout_event, HZ/5, val);
}
#endif /* USB_STATUS_BY_EVENT */
#endif /* USB_FIREWIRE_HANDLING */
