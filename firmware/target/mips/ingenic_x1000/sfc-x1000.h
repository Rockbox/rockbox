/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include <stdint.h>
#include <stdbool.h>
#include "clk-x1000.h"
#include "x1000/sfc.h"

/* SPI flash controller interface -- this is a low-level driver upon which
 * you can build NAND/NOR flash drivers. The main function is sfc_exec(),
 * used to issue commands, transfer data, etc.
 */

#define SFC_FLAG_READ       0x01 /* Read data */
#define SFC_FLAG_WRITE      0x02 /* Write data */
#define SFC_FLAG_DUMMYFIRST 0x04 /* Do dummy bits before sending address.
                                  * Default is dummy bits after address.
                                  */

/* SPI transfer mode. If in doubt, check with the X1000 manual and confirm
 * the transfer format is what you expect.
 */
#define SFC_MODE_STANDARD           0
#define SFC_MODE_DUAL_IN_DUAL_OUT   1
#define SFC_MODE_DUAL_IO            2
#define SFC_MODE_FULL_DUAL_IO       3
#define SFC_MODE_QUAD_IN_QUAD_OUT   4
#define SFC_MODE_QUAD_IO            5
#define SFC_MODE_FULL_QUAD_IO       6

/* Return status codes for sfc_exec() */
#define SFC_STATUS_OK        0
#define SFC_STATUS_OVERFLOW  1
#define SFC_STATUS_UNDERFLOW 2
#define SFC_STATUS_LOCKUP    3

typedef struct sfc_op {
    int command;        /* Command number */
    int mode;           /* SPI transfer mode */
    int flags;          /* Flags for this op */
    int addr_bytes;     /* Number of address bytes */
    int dummy_bits;     /* Number of dummy bits (yes: bits, not bytes) */
    uint32_t addr_lo;   /* Lower 32 bits of address */
    uint32_t addr_hi;   /* Upper 32 bits of address */
    int data_bytes;     /* Number of data bytes to read/write */
    void* buffer;       /* Data buffer -- MUST be word-aligned */
} sfc_op;

/* One-time driver init for mutexes/etc needed for handling interrupts.
 * This can be safely called multiple times; only the first call will
 * actually perform the init.
 */
extern void sfc_init(void);

/* Controller mutex -- lock before touching the driver */
extern void sfc_lock(void);
extern void sfc_unlock(void);

/* Open/close the driver. The driver must be open in order to do operations.
 * Closing the driver shuts off the hardware; the driver can be re-opened at
 * a later time when it's needed again.
 *
 * After opening the driver, you must also program a valid device configuration
 * and clock rate using sfc_set_dev_conf() and sfc_set_clock().
 */
extern void sfc_open(void);
extern void sfc_close(void);

/* These functions can be called at any time while the driver is open, but
 * must not be called while there is an operation in progress. It's the
 * caller's job to ensure the configuration will work with the device and
 * be capable of reading back data correctly.
 *
 * - sfc_set_dev_conf() writes its argument to the SFC_DEV_CONF register.
 * - sfc_set_wp_enable() sets the state of the write-protect pin (WP).
 * - sfc_set_clock() sets the controller clock frequency (in Hz).
 */
#define sfc_set_dev_conf(dev_conf) \
    do { REG_SFC_DEV_CONF = (dev_conf); } while(0)

#define sfc_set_wp_enable(en) \
    jz_writef(SFC_GLB, WP_EN((en) ? 1 : 0))

extern void sfc_set_clock(x1000_clk_t clksrc, uint32_t freq);

/* Execute an operation. Returns zero on success, nonzero on failure. */
extern int sfc_exec(const sfc_op* op);
