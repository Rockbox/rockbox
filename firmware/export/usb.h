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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _USB_H_
#define _USB_H_

#include "kernel.h"
#include "button.h"

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
#define USB_FIREWIRE_HANDLING
#endif

/* Messages from usb_tick and thread states */
enum {
    USB_EXTRACTED = 0,       /* Event+State */
    USB_INSERTED,            /* Event+State */
    USB_POWERED,             /* Event+State - transitional indicator if no power */
#if defined(USB_DETECT_BY_DRV) || defined(USB_DETECT_BY_CORE)
    USB_UNPOWERED,           /* Event */
#endif
#ifdef HAVE_LCD_BITMAP
    USB_SCREENDUMP,          /* State */
#endif
#if (CONFIG_STORAGE & STORAGE_MMC)
    USB_REENABLE,            /* Event */
#endif
#ifdef HAVE_USBSTACK
    USB_TRANSFER_COMPLETION, /* Event */
#endif
#ifdef USB_FIREWIRE_HANDLING
    USB_REQUEST_REBOOT,      /* Event */
#endif
    USB_QUIT,                /* Event */
#if defined(HAVE_USB_CHARGING_ENABLE) && defined(HAVE_USBSTACK)
    USB_CHARGER_UPDATE,      /* Event */
#endif
#ifdef HAVE_BOOTLOADER_USB_MODE
    USB_HANDLED,             /* Bootloader status code */
#endif
};
#ifdef HAVE_USB_POWER
#if CONFIG_KEYPAD == RECORDER_PAD
#define USBPOWER_BUTTON BUTTON_F1
#define USBPOWER_BTN_IGNORE BUTTON_ON
#elif CONFIG_KEYPAD == ONDIO_PAD
#define USBPOWER_BUTTON BUTTON_MENU
#define USBPOWER_BTN_IGNORE BUTTON_OFF
/*allow people to define this in config-target.h if they need it*/
#elif !defined(USBPOWER_BTN_IGNORE) 
#define USBPOWER_BTN_IGNORE 0
#endif
#endif

#ifdef HAVE_USBSTACK
/* USB class drivers */
enum {
#ifdef USB_ENABLE_STORAGE
    USB_DRIVER_MASS_STORAGE,
#endif
#ifdef USB_ENABLE_SERIAL
    USB_DRIVER_SERIAL,
#endif
#ifdef USB_ENABLE_CHARGING_ONLY
    USB_DRIVER_CHARGING_ONLY,
#endif
#ifdef USB_ENABLE_HID
    USB_DRIVER_HID,
#endif
    USB_NUM_DRIVERS
};

struct usb_transfer_completion_event_data
{
    unsigned char endpoint;
    int dir;
    int status;
    int length;
    void* data;
};
#endif /* HAVE_USBSTACK */

void usb_init(void) INIT_ATTR;
void usb_enable(bool on);
void usb_attach(void);
void usb_start_monitoring(void);
void usb_close(void);
void usb_acknowledge(long id);
void usb_wait_for_disconnect(struct event_queue *q);
int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks);
bool usb_inserted(void); /* return the official value, what's been reported to the threads */
int usb_detect(void); /* return the raw hardware value - nothing/pc/charger */
/* For tick-less USB monitoring (USB_STATUS_BY_EVENT) */
void usb_status_event(int current_status);
#ifdef HAVE_USB_POWER
bool usb_powered(void);
#ifdef HAVE_USB_CHARGING_ENABLE
enum {
    USB_CHARGING_DISABLE,
    USB_CHARGING_ENABLE,
    USB_CHARGING_FORCE
};
/* called by app, implemented by usb_core on targets with rockbox usb
 * or target-specific code on others
 */
void usb_charging_enable(int state);
#ifdef HAVE_USBSTACK
void usb_charger_update(void);
#endif /* HAVE_USBSTACK */
#endif /* HAVE_USB_CHARGING_ENABLE */
#endif /* HAVE_USB_POWER */
#ifdef HAVE_USBSTACK
void usb_signal_transfer_completion(
    struct usb_transfer_completion_event_data *event_data);
bool usb_driver_enabled(int driver);
bool usb_exclusive_storage(void); /* storage is available for usb */
void usb_storage_try_release_storage(void);
#endif
int usb_release_exclusive_storage(void);

#ifdef USB_FIREWIRE_HANDLING
bool firewire_detect(void);
void usb_firewire_connect_event(void);
#endif

#ifdef USB_ENABLE_HID
void usb_set_hid(bool enable);
#endif

#endif
