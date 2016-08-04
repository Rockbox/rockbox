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

#include <stdint.h>

/**
 * This file contains the data structures used in the USB and network protocol.
 * All USB data uses the standard USB byte order which is little-endian.
 */

/**
 * HWStub protocol version
 */

#define HWSTUB_VERSION_MAJOR    4
#define HWSTUB_VERSION_MINOR    3

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

/**********************************
 * Descriptors
 **********************************/

/**
 * Descriptors can be retrieved using configuration descriptor or individually
 * using the standard GetDescriptor request on the interface.
 */

#define HWSTUB_DT_VERSION   0x41 /* mandatory */
#define HWSTUB_DT_LAYOUT    0x42 /* mandatory */
#define HWSTUB_DT_TARGET    0x43 /* mandatory */
#define HWSTUB_DT_STMP      0x44 /* mandatory for STMP */
#define HWSTUB_DT_PP        0x45 /* mandatory for PP */
#define HWSTUB_DT_JZ        0x46 /* mandatory for JZ */

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

struct hwstub_jz_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Chip ID and revision */
    uint16_t wChipID; /* 0x4760 for Jz4760 for example */
    uint8_t bRevision; /* 0 for Jz4760, 'B' for JZ4760B */
} __attribute__((packed));

#define HWSTUB_TARGET_UNK       ('U' | 'N' << 8 | 'K' << 16 | ' ' << 24)
#define HWSTUB_TARGET_STMP      ('S' | 'T' << 8 | 'M' << 16 | 'P' << 24)
#define HWSTUB_TARGET_RK27      ('R' | 'K' << 8 | '2' << 16 | '7' << 24)
#define HWSTUB_TARGET_PP        ('P' | 'P' << 8 | ' ' << 16 | ' ' << 24)
#define HWSTUB_TARGET_ATJ       ('A' | 'T' << 8 | 'J' << 16 | ' ' << 24)
#define HWSTUB_TARGET_JZ        ('J' | 'Z' << 8 | '4' << 16 | '7' << 24)

struct hwstub_target_desc_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    /* Target ID and name */
    uint32_t dID;
    char bName[58];
} __attribute__((packed));

/**
 * Socket command packet header: any transfer (in both directions) start with this.
 * All data is transmitted in network byte order.
 */
#define HWSTUB_NET_ARGS 4

struct hwstub_net_hdr_t
{
    uint32_t magic; /* magic value (HWSERVER_MAGIC) */
    uint32_t cmd; /* command (OR'ed with (N)ACK on response) */
    uint32_t length; /* length of the data following this header */
    uint32_t args[HWSTUB_NET_ARGS]; /* command arguments */
} __attribute__((packed));

/**
 * Control commands
 *
 * These commands are sent to the interface, using the standard bRequest field
 * of the SETUP packet. The wIndex contains the interface number. The wValue
 * contains an ID which is used for requests requiring several transfers.
 */
#define HWSTUB_GET_LOG              0x40
#define HWSTUB_READ                 0x41
#define HWSTUB_READ2                0x42
#define HWSTUB_WRITE                0x43
#define HWSTUB_EXEC                 0x44
#define HWSTUB_READ2_ATOMIC         0x45
#define HWSTUB_WRITE_ATOMIC         0x46
#define HWSTUB_COPROCESSOR_OP       0x47

/* the following commands and the ACK/NACK mechanism are net only */
#define HWSERVER_ACK(n) (0x100|(n))
#define HWSERVER_ACK_MASK 0x100
#define HWSERVER_NACK(n) (0x200|(n))
#define HWSERVER_NACK_MASK 0x200

#define HWSERVER_MAGIC              ('h' << 24 | 'w' << 16 | 's' << 8 | 't')

#define HWSERVER_HELLO              0x400
#define HWSERVER_GET_DEV_LIST       0x401
#define HWSERVER_DEV_OPEN           0x402
#define HWSERVER_DEV_CLOSE          0x403
#define HWSERVER_BYE                0x404
#define HWSERVER_GET_DESC           0x405
#define HWSERVER_GET_LOG            0x406
#define HWSERVER_READ               0x407
#define HWSERVER_WRITE              0x408
#define HWSERVER_EXEC               0x409

/* net errors (always in arg[0] if command is NACKed) */
#define HWERR_OK            0    /* success */
#define HWERR_FAIL          1    /* general error from hwstub */
#define HWERR_INVALID_ID    2    /* invalid id of the device */
#define HWERR_DISCONNECTED  3    /* device got disconnected */

/* read/write flags */
#define HWSERVER_RW_ATOMIC          0x1

/**********************************
 * Control Protocol
 **********************************/

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
 * HWSTUB_WRITE:
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

/**
 * HWSTUB_COPROCESSOR_OP
 * Execute a coprocessor operation. The operation is describe in the header of
 * the structure. There are currently two supported operations:
 * - read: following the HWSTUB_COPROCESSOR_OP, the host can retrieve the data
 *         by sending a HWSTUB_READ2 request (just like a regular read) of
 *         the appropriate size
 * - write: the header is followed by the data to write.
 * If the request has two parts (second being READ2) then both requests must use
 * the same ID in wValue, otherwise the second request will be STALLed.
 * If a particular operation is not supported, it must be STALLed by the device.
 */

#define HWSTUB_COP_READ     0 /* read operation */
#define HWSTUB_COP_WRITE    1 /* write operation */
/* for MIPS */
#define HWSTUB_COP_MIPS_COP 0 /* coprocessor number */
#define HWSTUB_COP_MIPS_REG 1 /* coprocessor register */
#define HWSTUB_COP_MIPS_SEL 2 /* coprocessor select */

#define HWSTUB_COP_ARGS     7 /* maximum number of arguments */

struct hwstub_cop_req_t
{
    uint8_t bOp; /* operation to execute */
    uint8_t bArgs[HWSTUB_COP_ARGS]; /* arguments to the operation */
    /* followed by data for WRITE operation */
};

/**
 * HWSERVER_HELLO:
 * Say hello to the server, give protocol version and get server version.
 * Send: args[0] = major << 8 | minor, no data
 * Receive: args[0] = major << 8 | minor, no data
 */

/**
 * HWSERVER_GET_DEV_LIST:
 * Get device list.
 * Send: no argument, no data.
 * Receive: no argument, data contains a list of device IDs, each ID is a uint32_t
 *          transmitted in network byte order.
 */

/**
 * HWSERVER_DEV_OPEN:
 * Open a device and return a handle.
 * Send: args[0] = device ID, no data.
 * Receive: args[0] = handle ID, no data.
 */

/**
 * HWSERVER_DEV_CLOSE:
 * Close a device handle.
 * Send: args[0] = handle ID, no data.
 * Receive: no argument, no data.
 */

/**
 * HWSERVER_BYE:
 * Say bye to the server, closing all devices and effectively stopping the communication.
 * Send: no argument, no data
 * Receive: no argument, no data
 */

/**
 * HWSERVER_GET_DESC:
 * Query a descriptor.
 * Send: args[0] = handle ID, args[1] = desc ID, args[2] = requested length, no data
 * Receive: no argument, data contains RAW descriptor (ie all fields are in little-endian)
 */

/**
 * HWSERVER_GET_LOG:
 * Query a descriptor.
 * Send: args[0] = handle ID, args[1] = requested length, no data
 * Receive: no argument, data contains log data
 */

/**
 * HWSERVER_READ:
 * Read data.
 * Send: args[0] = handle ID, args[1] = addr, args[2] = length, args[3] = flags
 * Receive: no argument, data read on device
 */

/**
 * HWSERVER_WRITE:
 * Read data.
 * Send: args[0] = handle ID, args[1] = addr, args[2] = flags, data to write
 * Receive: no data
 */

/**
 * HWSERVER_EXEC:
 * Execute code.
 * Send: args[0] = handle ID, args[1] = addr, args[2] = flags, no data
 * Receive: no data
 */

#endif /* __HWSTUB_PROTOCOL__ */
