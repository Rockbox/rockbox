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
#ifndef __HWEMUL_PROTOCOL__
#define __HWEMUL_PROTOCOL__

#define HWEMUL_CLASS        0xfe
#define HWEMUL_SUBCLASS     0xac
#define HWEMUL_PROTOCOL     0x1d

#define HWEMUL_VERSION_MAJOR    2
#define HWEMUL_VERSION_MINOR    8
#define HWEMUL_VERSION_REV      2

#define HWEMUL_USB_VID  0xfee1
#define HWEMUL_USB_PID  0xdead

/**
 * Control commands
 *
 * These commands are sent to the device, using the standard bRequest field
 * of the SETUP packet. This is to take advantage of both wIndex and wValue
 * although it would have been more correct to send them to the interface.
 */

/* list of commands */
#define HWEMUL_GET_INFO     0 /* mandatory */
#define HWEMUL_GET_LOG      1 /* optional */
#define HWEMUL_RW_MEM       2 /* optional */
#define HWEMUL_CALL         3 /* optional */
#define HWEMUL_JUMP         4 /* optional */
#define HWEMUL_AES_OTP      5 /* optional */

/**
 * HWEMUL_GET_INFO: get some information about an aspect of the device.
 * The wIndex field of the SETUP specifies which information to get. */

/* list of possible information */
#define HWEMUL_INFO_VERSION     0
#define HWEMUL_INFO_LAYOUT      1
#define HWEMUL_INFO_STMP        2
#define HWEMUL_INFO_FEATURES    3

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
#define HWEMUL_FEATURE_LOG      (1 << 0)
#define HWEMUL_FEATURE_MEM      (1 << 1)
#define HWEMUL_FEATURE_CALL     (1 << 2)
#define HWEMUL_FEATURE_JUMP     (1 << 2)
#define HWEMUL_FEATURE_AES_OTP  (1 << 3)

struct usb_resp_info_features_t
{
    uint32_t feature_mask;
};

/**
 * HWEMUL_GET_LOG: only if has HWEMUL_FEATURE_LOG.
 * The log is returned as part of the control transfer.
 */

/**
 * HWEMUL_RW_MEM: only if has HWEMUL_FEATURE_MEM.
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write. */

/**
 * HWEMUL_x: only if has HWEMUL_FEATURE_x where x=CALL or JUMP.
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write. */

/**
 * HWEMUL_AES_OTP: only if has HWEMUL_FEATURE_AES_OTP.
 * The control transfer contains the data to be en/decrypted and the data
 * is sent back on the interrupt endpoint. The first 16-bytes of the data
 * are interpreted as the IV. The output format is the same.
 * The wValue field contains the parameters of the process. */
#define HWEMUL_AES_OTP_ENCRYPT  (1 << 0)

#endif /* __HWEMUL_PROTOCOL__ */
