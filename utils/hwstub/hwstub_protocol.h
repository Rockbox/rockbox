/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __HWSTUB_PROTOCOL__
#define __HWSTUB_PROTOCOL__

#define HWSTUB_CLASS        0xfe
#define HWSTUB_SUBCLASS     0xac
#define HWSTUB_PROTOCOL     0x1d

#define HWSTUB_VERSION_MAJOR    2
#define HWSTUB_VERSION_MINOR    11
#define HWSTUB_VERSION_REV      2

#define HWSTUB_USB_VID  0xfee1
#define HWSTUB_USB_PID  0xdead

/**
 * Control commands
 *
 * These commands are sent to the device, using the standard bRequest field
 * of the SETUP packet. This is to take advantage of both wIndex and wValue
 * although it would have been more correct to send them to the interface.
 */

/* list of commands */
#define HWSTUB_GET_INFO     0 /* mandatory */
#define HWSTUB_GET_LOG      1 /* optional */
#define HWSTUB_RW_MEM       2 /* optional */
#define HWSTUB_CALL         3 /* optional */
#define HWSTUB_JUMP         4 /* optional */
#define HWSTUB_EXIT         5 /* optional */
#define HWSTUB_ATEXIT       6 /* optional */

/**
 * HWSTUB_GET_INFO: get some information about an aspect of the device.
 * The wIndex field of the SETUP specifies which information to get. */

/* list of possible information */
#define HWSTUB_INFO_VERSION     0 /* mandatory */
#define HWSTUB_INFO_LAYOUT      1 /* mandatory */
#define HWSTUB_INFO_STMP        2 /* optional */
#define HWSTUB_INFO_FEATURES    3 /* mandatory */
#define HWSTUB_INFO_TARGET      4 /* mandatory */

struct usb_resp_info_version_t
{
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
} __attribute__((packed));

struct usb_resp_info_layout_t
{
    /* describe the range of memory used by the running code */
    uint32_t oc_code_start;
    uint32_t oc_code_size;
    /* describe the range of memory used by the stack */
    uint32_t oc_stack_start;
    uint32_t oc_stack_size;
    /* describe the range of memory available as a buffer */
    uint32_t oc_buffer_start;
    uint32_t oc_buffer_size;
} __attribute__((packed));

struct usb_resp_info_stmp_t
{
    uint16_t chipid; /* 0x3780 for STMP3780 for example */
    uint8_t rev; /* 0=TA1 on STMP3780 for example */
    uint8_t is_supported; /* 1 if the chip is supported */
} __attribute__((packed));

/* list of possible features */
#define HWSTUB_FEATURE_LOG      (1 << 0)
#define HWSTUB_FEATURE_MEM      (1 << 1)
#define HWSTUB_FEATURE_CALL     (1 << 2)
#define HWSTUB_FEATURE_JUMP     (1 << 3)
#define HWSTUB_FEATURE_EXIT     (1 << 4)

struct usb_resp_info_features_t
{
    uint32_t feature_mask;
};

#define HWSTUB_TARGET_UNK       ('U' | 'N' << 8 | 'K' << 16 | ' ' << 24)
#define HWSTUB_TARGET_STMP      ('S' | 'T' << 8 | 'M' << 16 | 'P' << 24)

struct usb_resp_info_target_t
{
    uint32_t id;
    char name[60];
};

/**
 * HWSTUB_GET_LOG: only if has HWSTUB_FEATURE_LOG.
 * The log is returned as part of the control transfer.
 */

/**
 * HWSTUB_RW_MEM: only if has HWSTUB_FEATURE_MEM.
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write.
 * The read/write on the device are guaranteed to be 16-bit/32-bit when
 * possible, making it suitable to read/write registers. */

/**
 * HWSTUB_x: only if has HWSTUB_FEATURE_x where x=CALL or JUMP.
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write. */

/**
 * HWSTUB_EXIT: only if has HWSTUB_FEATURE_EXIT.
 * Stop hwstub now, performing the atexit action. Default exit action
 * is target dependent. */

/**
 * HWSTUB_ATEXIT: only if has HWSTUB_FEATURE_EXIT.
 * Sets the action to perform at exit. Exit happens by sending HWSTUB_EXIT
 * or on USB disconnection. The following actions are available:
 * - nop: don't do anything, wait for next connection
 * - reboot: reboot the device
 * - off: power off
 * NOTE the power off action might have to wait for USB disconnection as some
 * targets cannot power off while plugged.
 * NOTE appart from nop which is mandatory, all other methods can be
 * unavailable and thus the atexit command can fail. */
#define HWSTUB_ATEXIT_REBOOT    0
#define HWSTUB_ATEXIT_OFF       1
#define HWSTUB_ATEXIT_NOP       2

#endif /* __HWSTUB_PROTOCOL__ */
