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

#define HWSTUB_CLASS        0xff
#define HWSTUB_SUBCLASS     0xac
#define HWSTUB_PROTOCOL     0x1d

#define HWSTUB_VERSION_MAJOR    3
#define HWSTUB_VERSION_MINOR    0
#define HWSTUB_VERSION_REV      0

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
#define HWSTUB_GET_LOG      0 /* optional */
#define HWSTUB_RW_MEM       1 /* optional */
#define HWSTUB_CALL         2 /* optional */
#define HWSTUB_JUMP         3 /* optional */

/**
 * Descriptors can be retrieve using configuration descriptor or individually
 * using the standard GetDescriptor request on the interface.
 */

/* list of possible information */
#define HWSTUB_DT_VERSION     0x41 /* mandatory */
#define HWSTUB_DT_LAYOUT      0x42 /* mandatory */
#define HWSTUB_DT_TARGET      0x43 /* mandatory */
#define HWSTUB_DT_STMP        0x44 /* optional */

struct hwstub_version_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* full version information */
    uint8_t bMajor;
    uint8_t bMinor;
    uint8_t bRevision;
} __attribute__((packed));

struct hwstub_layout_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* describe the range of memory used by the running code */
    uint32_t dCodeStart;
    uint32_t dCodeSize;
    /* describe the range of memory used by the stack */
    uint32_t dStackStart;
    uint32_t dStackSize;
    /* describe the range of memory available as a buffer */
    uint32_t dBufferStart;
    uint32_t dBufferSize;
} __attribute__((packed));

struct hwstub_stmp_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Chip ID and revision */
    uint16_t wChipID; /* 0x3780 for STMP3780 for example */
    uint8_t bRevision; /* 0=TA1 on STMP3780 for example */
    uint8_t bPackage; /* 0=169BGA for example */
} __attribute__((packed));

#define HWSTUB_TARGET_UNK       ('U' | 'N' << 8 | 'K' << 16 | ' ' << 24)
#define HWSTUB_TARGET_STMP      ('S' | 'T' << 8 | 'M' << 16 | 'P' << 24)
#define HWSTUB_TARGET_RK27      ('R' | 'K' << 8 | '2' << 16 | '7' << 24)
#define HWSTUB_TARGET_PP        ('P' | 'P' << 8 | ' ' << 16 | ' ' << 24)

struct hwstub_target_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Target ID and name */
    uint32_t dID;
    char bName[58];
} __attribute__((packed));

/**
 * HWSTUB_GET_LOG:
 * The log is returned as part of the control transfer.
 */

/**
 * HWSTUB_RW_MEM:
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write.
 * The read/write on the device are guaranteed to be 16-bit/32-bit when
 * possible, making it suitable to read/write registers. */

/**
 * HWSTUB_{CALL,JUMP}:
 * The 32-bit address is split into two parts.
 * The low 16-bit are stored in wValue and the upper
 * 16-bit are stored in wIndex. Depending on the transfer direction,
 * the transfer is either a read or a write. */

#endif /* __HWSTUB_PROTOCOL__ */
