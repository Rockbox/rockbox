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

#include "config.h"
#include "kernel.h"
#include "button.h"

/** USB introduction
 * Targets which do not have any hardware support for USB, and cannot even detect
 * it must define USB_NONE. Otherwise, they must at least implement USB
 * detection.
 *
 * USB architecture
 * The USB code is split into several parts:
 * - usb: manages the USB connection
 * - usb_core: implements a software USB stack based on usb_drv
 * - usb_drv: implements the USB protocol based on some hardware transceiver/core
 * - usb_{hid,storage,...}: implement USB functionalities based on usb_core
 * Note that not all those are compiled in, in particular in the case of a
 * hardware USB stack, or when the driver doesn't support all types of transfers.
 *
 * Software versus hardware USB stack
 * A very important thing to keep in mind is that there are two very different
 * situations:
 * - software USB stack: the device only provides a USB transceiver and the
 *   USB stack must be implemented entirely in software. In this case the target
 *   must define HAVE_USBSTACK, correctly set CONFIG_USBOTG and implement a driver
 *   for the transceiver.
 * - hardware USB stack: the device has a dedicated chip which implements the
 *   USB stack in hardware. In this case the target must *NOT* define HAVE_USBSTACK
 *   but can still define CONFIG_USBOTG and implement a driver to enable/disable
 *   the USB hardware.
 *
 * USB ignore buttons
 * In some cases, the user wants to prevent Rockbox from entering USB mode. It
 * can do so by holding a button while inserting the cable. By default any button
 * will prevent the USB mode from kicking-in, so targets can optionally define
 * USBPOWER_BTN_IGNORE to a mask of buttons to ignore in this check.
 *
 * USB states
 * It is important to understand that the usb code can be in one of three states:
 * - extracted: no USB cable is plugged
 * - powered-only: a USB cable is plugged but the USB mode will not be entered,
 *   either because no host was detected or because the user requested so.
 * - inserted: a USB cable is plugged and the USB mode has been entered, either
 *   the software or hardware stack is running.
 *
 * USB exclusive mode
 * Either in hardware or software stack, if the USB was configured to run in
 * mass storage mode, it will require exclusive access to the disk and ask all
 * threads to release any file handle and stop using the disks. It does so by
 * broadcasting a SYS_USB_CONNECTED message, which threads must acknowledge using
 * usb_acknowledge(SYS_USB_CONNECTED_ACK). They must not access the disk until
 * SYS_USB_DISCONNECTED is broadcast. To ease waiting, threads can call
 * usb_wait_for_disconnect() or usb_wait_for_disconnect_w_tmo() on their waiting
 * queue.
 *
 * USB detection
 * Except when no usb code is compiled at all (USB_NONE), the usb thread keeps
 * track of the USB insertion state, which can be either USB_INSERTED (meaning
 * 5v is present) or USB_EXTRACTED. Each target must implement usb_detect()
 * to report the insertion state.
 * Targets which support insertion/extraction interrupts must define
 * USB_STATUS_BY_EVENT and notify the thread on changes by calling
 * usb_status_event() with the new state. Other targets must *not* define
 * USB_STATUS_BY_EVENT and the usb thread by regularly poll the insertion state
 * using usb_detect().
 *
 * USB powering & charging
 * Device which can be powered from USB must define HAVE_USB_POWER. Note that
 * powering doesn't imply charging (for example a AA-powered device can be
 * powered from USB but not charged), charging sources are reported by the
 * power subsystem (see power.h). The USB specification mandates the maximum
 * current which can be drawn under which cirmcunstances. Device which cannot
 * control the charge current should make sure it is always <100mA to meet the
 * USB specification. Device with configurable charging current which support
 * >=100mA must define HAVE_USB_CHARGING_ENABLE and implement
 * usb_charging_maxcurrent_change() to let the usb thread control the maximum
 * charging control.
 * */

#if defined(IPOD_COLOR) || defined(IPOD_4G) \
 || defined(IPOD_MINI)  || defined(IPOD_MINI2G)
#define USB_FIREWIRE_HANDLING
#endif

/* Messages from usb_tick and thread states */
enum
{
#ifdef HAVE_LCD_BITMAP
    USB_SCREENDUMP = -1,     /* State */
#endif
    USB_EXTRACTED = 0,       /* Event+State */
    USB_INSERTED,            /* Event+State */
    USB_POWERED,             /* State - transitional indicator if no host */
#if (CONFIG_STORAGE & STORAGE_MMC)
    USB_REENABLE,            /* Event */
#endif
#ifdef HAVE_USBSTACK
    USB_TRANSFER_COMPLETION, /* Event */
    USB_NOTIFY_SET_ADDR,     /* Event */
    USB_NOTIFY_SET_CONFIG,   /* Event */
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

/* initialise the usb code and thread */
void usb_init(void) INIT_ATTR;
/* target must implement this to enable/disable the usb transceiver/core */
void usb_enable(bool on);
void usb_attach(void);
/* enable usb detection monitoring; before this function is called, all usb
 * detection changes are ignored */
void usb_start_monitoring(void) INIT_ATTR;
void usb_close(void);
/* acknowledge usb connection, typically with SYS_USB_CONNECTED_ACK */
void usb_acknowledge(long id);
/* block the current thread until SYS_USB_DISCONNECTED has been broadcast */
void usb_wait_for_disconnect(struct event_queue *q);
/* same as usb_wait_for_disconnect() but with a timeout, returns 1 on timeout */
int usb_wait_for_disconnect_w_tmo(struct event_queue *q, int ticks);
/* check whether USB is plugged, note that this is the official value which has
 * been reported to the thread */
bool usb_inserted(void);
/* check whether USB is plugged, note that this is the raw hardware value */
int usb_detect(void);
#ifdef USB_STATUS_BY_EVENT
/* Notify USB insertion state (USB_INSERTED or USB_EXTRACTED) */
void usb_status_event(int current_status);
#endif
#ifdef HAVE_USB_POWER
/* returns whether the USB is in powered-only state */
bool usb_powered_only(void);
#ifdef HAVE_USB_CHARGING_ENABLE
enum {
    USB_CHARGING_DISABLE, /* the USB code will never ask for more than 100mA */
    USB_CHARGING_ENABLE, /* the code will ask for the maximum possible value */
    USB_CHARGING_FORCE /* the code will always ask for 500mA */
};
/* select the USB charging mode, typically used by apps/ to reflect user setting,
 * implemented by usb_core on targets with a software stack, and by target code
 * on targets with a hardware stack */
void usb_charging_enable(int state);
#ifdef HAVE_USBSTACK
/* update the USB charging value based on the current USB state */
void usb_charger_update(void);
#endif /* HAVE_USBSTACK */
/* limit the maximum USB current the charger can draw */
void usb_charging_maxcurrent_change(int maxcurrent);
/* returns the maximum allowed USB current, based on USB charging mode and state */
int usb_charging_maxcurrent(void);
#endif /* HAVE_USB_CHARGING_ENABLE */
void usb_set_charge_setting(bool charge_only);
#endif /* HAVE_USB_POWER */
#ifdef HAVE_USBSTACK
/* USB driver call this function to notify that a transfer has completed */
void usb_signal_transfer_completion(
    struct usb_transfer_completion_event_data *event_data);
/* notify the USB code that some important event has occurred which influences the
 * USB state (like USB_NOTIFY_SET_ADDR). USB drivers should call usb_core_notify_*
 * functions and not this function. */
void usb_signal_notify(long id, intptr_t data);
/* returns whether a USB_DRIVER_* is enabled (like HID, mass storage, ...) */
bool usb_driver_enabled(int driver);
/* returns whether exclusive storage is available for USB */
bool usb_exclusive_storage(void);
#endif /* HAVE_USBSTACK */

#ifdef USB_FIREWIRE_HANDLING
bool firewire_detect(void);
void usb_firewire_connect_event(void);
#endif

#ifdef USB_ENABLE_HID
/* enable or disable the HID driver */
void usb_set_hid(bool enable);
#endif

#if defined(USB_ENABLE_STORAGE) && defined(HAVE_MULTIDRIVE)
/* when the target has several drives, decide whether mass storage should
 * skip the first drive. This is useful when the second drive is a SD card
 * and the host only supports access to the first USB drive (this is very common
 * in car tuners and USB speakers) */
void usb_set_skip_first_drive(bool skip);
#endif

#if !defined(SIMULATOR) && !defined(USB_NONE)
/* initialise the USB hardware, this is a one-time init and it should setup what
 * is necessary to do proper USB detection, and it should call usb_drv_startup()
 * to do the one-time initialisation of the USB driver */
void usb_init_device(void);
#endif

#endif
