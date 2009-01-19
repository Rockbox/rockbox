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
    
    /* disable USB-devices until USB is detected via GPIO */
#ifndef BOOTLOADER
    /* Disabling USB0 in the bootloader makes the OF not load,
       Also something here breaks usb pin detect in bootloader.
       leave it all enabled untill rockbox main loads */
    DEV_EN &= ~DEV_USB0;
    DEV_EN &= ~DEV_USB1;
    DEV_INIT2 &= ~INIT_USB;
#endif

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
    /* GPIO C bit 1 is firewire detect */
    GPIOC_ENABLE    |=  0x02;
    GPIOC_OUTPUT_EN &= ~0x02;
#endif

#ifdef HAVE_USBSTACK
    /* Do one-time inits */
    usb_drv_startup();
#endif
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
#ifdef USB_DETECT_BY_DRV
    usb_drv_attach();
#else
    usb_enable(true);
#endif
}

#ifdef USB_DETECT_BY_DRV
/* Cannot tell charger pin from USB pin */
static int usb_status = USB_EXTRACTED;

void usb_connect_event(bool inserted)
{
    usb_status = inserted ? USB_INSERTED : USB_EXTRACTED;
    usb_status_event(inserted ? USB_POWERED : USB_UNPOWERED);
}

/* Called during the bus reset interrupt when in detect mode */
void usb_drv_usb_detect_event(void)
{
    usb_status_event(USB_INSERTED);
}
#else /* !USB_DETECT_BY_DRV */
static bool usb_pin_detect(void)
{
    bool retval = false;
    
#if defined(IPOD_4G) || defined(IPOD_COLOR) \
 || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    /* GPIO D bit 3 is usb detect */
    if (GPIOD_INPUT_VAL & 0x08)
        retval = true;

#elif defined(IPOD_NANO) || defined(IPOD_VIDEO)
    /* GPIO L bit 4 is usb detect */
    if (GPIOL_INPUT_VAL & 0x10)
        retval = true;

#elif defined(SANSA_C200)
    /* GPIO H bit 1 is usb/charger detect */
    if (GPIOH_INPUT_VAL & 0x02)
        retval = true;

#elif defined(SANSA_E200)
    /* GPIO B bit 4 is usb/charger detect */
    if (GPIOB_INPUT_VAL & 0x10)
        retval = true;

#elif defined(IRIVER_H10) || defined(IRIVER_H10_5GB) || defined(MROBE_100)
    /* GPIO L bit 2 is usb detect */
    if (GPIOL_INPUT_VAL & 0x4)
        retval = true;

#elif defined(PHILIPS_SA9200)
    /* GPIO F bit 7 is usb detect */
    if (!(GPIOF_INPUT_VAL & 0x80))
        retval = true;

#elif defined(PHILIPS_HDD1630)
    /* GPIO E bit 2 is usb detect */
    if (GPIOE_INPUT_VAL & 0x4)
        retval = true;
#endif

    return retval;
}
#endif /* USB_DETECT_BY_DRV */

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
#ifdef USB_DETECT_BY_DRV
    return usb_status;
#else
    if(usb_pin_detect()) {
        return USB_INSERTED;
    }
    else {
        return USB_EXTRACTED;
    }
#endif
}

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
bool firewire_detect(void)
{
    /* GPIO C bit 1 is firewire detect */
    if (!(GPIOC_INPUT_VAL & 0x02))
        /* no charger detection needed for firewire */
        return true;
    else
        return false;
}
#endif
