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

/**
 * HWStub protocol version
 */

#define HWSTUB_VERSION_MAJOR    4
#define HWSTUB_VERSION_MINOR    1

#define HWSTUB_VERSION__(maj, min) #maj"."#min
#define HWSTUB_VERSION_(maj, min) HWSTUB_VERSION__(maj, min)
#define HWSTUB_VERSION  HWSTUB_VERSION_(HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR)

/**
 * A device can use any VID:PID but in case hwstub is in full control of the
 * device, the preferred VID:PID is the following.
 */

#define HWSTUB_USB_VID  0xfee1
#define HWSTUB_USB_PID  0xdead

/**
 * The device class should be per interface and the hwstub interface must use
 * the following class, subclass and protocol.
 */

#define HWSTUB_CLASS        0xff
#define HWSTUB_SUBCLASS     0xde
#define HWSTUB_PROTOCOL     0xad

/**
 * Descriptors can be retrieved using configuration descriptor or individually
 * using the standard GetDescriptor request on the interface.
 */

#define HWSTUB_DT_VERSION   0x41 /* mandatory */
#define HWSTUB_DT_LAYOUT    0x42 /* mandatory */
#define HWSTUB_DT_TARGET    0x43 /* mandatory */
#define HWSTUB_DT_STMP      0x44 /* optional */
#define HWSTUB_DT_PP        0x45 /* optional */
#define HWSTUB_DT_DEVICE    0x46 /* optional */

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

struct hwstub_pp_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Chip ID and revision */
    uint16_t wChipID; /* 0x5002 for PP5002 for example */
    uint8_t bRevision[2]; /* 'B1' for B1 for example */
} __attribute__((packed));

#define HWSTUB_TARGET_UNK       ('U' | 'N' << 8 | 'K' << 16 | ' ' << 24)
#define HWSTUB_TARGET_STMP      ('S' | 'T' << 8 | 'M' << 16 | 'P' << 24)
#define HWSTUB_TARGET_RK27      ('R' | 'K' << 8 | '2' << 16 | '7' << 24)
#define HWSTUB_TARGET_PP        ('P' | 'P' << 8 | ' ' << 16 | ' ' << 24)
#define HWSTUB_TARGET_ATJ       ('A' | 'T' << 8 | 'J' << 16 | ' ' << 24)

struct hwstub_target_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Target ID and name */
    uint32_t dID;
    char bName[58];
} __attribute__((packed));

struct hwstub_device_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Give the bRequest value for */
} __attribute__((packed));

/**
 * Control commands
 *
 * These commands are sent to the interface, using the standard bRequest field
 * of the SETUP packet. The wIndex contains the interface number. The wValue
 * contains an ID which is used for requests requiring several transfers.
 */

#define HWSTUB_GET_LOG  0x40
#define HWSTUB_READ     0x41
#define HWSTUB_READ2    0x42
#define HWSTUB_WRITE    0x43
#define HWSTUB_EXEC     0x44
#define HWSTUB_READ2_ATOMIC 0x45
#define HWSTUB_WRITE_ATOMIC 0x46

/**
 * HWSTUB_GET_LOG:
 * The log is returned as part of the control transfer.
 */

/**
 * HWSTUB_READ and HWSTUB_READ2(_ATOMIC):
 * Read a range of memory. The request works in two steps: first the host
 * sends HWSTUB_READ with the parameters (address, length) and then
 * a HWSTUB_READ2 to retrieve the buffer. Both requests must use the same
 * ID in wValue, otherwise the second request will be STALLed.
 * HWSTUB_READ2_ATOMIC behaves the same as HWSTUB_READ2 except that the read
 * is guaranteed to be atomic (ie performed as a single memory access) and
 * will be STALLed if atomicity can not be ensured.
 */

struct hwstub_read_req_t
{
    uint32_t dAddress;
} __attribute__((packed));

/**
 * HWSTUB_WRITE
 * Write a range of memory. The payload starts with the following header, everything
 * which follows is data.
 * HWSTUB_WRITE_ATOMIC behaves the same except it is atomic. See HWSTUB_READ2_ATOMIC.
 */
struct hwstub_write_req_t
{
    uint32_t dAddress;
} __attribute__((packed));

/**
 * HWSTUB_EXEC:
 * Execute code at an address. Several options are available regarding ARM vs Thumb,
 * jump vs call.
 */

#define HWSTUB_EXEC_ARM     (0 << 0) /* target code is ARM */
#define HWSTUB_EXEC_THUMB   (1 << 0) /* target code is Thumb */
#define HWSTUB_EXEC_JUMP    (0 << 1) /* branch, code will never turn */
#define HWSTUB_EXEC_CALL    (1 << 1) /* call and expect return */

struct hwstub_exec_req_t
{
    uint32_t dAddress;
    uint16_t bmFlags;
} __attribute__((packed));

#endif /* __HWSTUB_PROTOCOL__ */
