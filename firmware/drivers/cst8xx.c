/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Firmware   |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Jukebox    |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

/* CST8xx capacitive touch controller. Read the first contact from the
 * seven-byte report at register 0x00:
 *   [2] finger count, [3] event (bits 7:6) and X[11:8], [4] X[7:0],
 *   [5] Y[11:8], [6] Y[7:0].
 *
 * This driver supports one contact. Some controllers can report two;
 * reports outside the supported count or coordinate range are ignored.
 * Read on interrupts to avoid sampling partially updated reports.
 * The board controls reset, power and the interrupt pin.
 */

#include "cst8xx.h"
#include "kernel.h"
#include "system.h"
#include "i2c-async.h"
#include <string.h>

#define CST8XX_BLOCK_LEN 7

struct cst8xx_driver {
    /* i2c bus data */
    int i2c_cookie;
    i2c_descriptor i2c_desc;
    bool enabled;
    bool accept_report;

    /* buffer for I2C transfers: [0] is the register address written,
     * [1..7] the block read back */
    uint8_t raw_data[1 + CST8XX_BLOCK_LEN];
};

static struct cst8xx_driver cst_drv;
struct cst8xx_state cst8xx_state;

static void cst8xx_park(void)
{
    cst8xx_state.nr_points = 0;
    for(int i = 0; i < CST8XX_NUM_POINTS; ++i)
        cst8xx_state.points[i].event = CST8XX_EVT_UP;
}

static bool cst8xx_valid(const uint8_t* reg, int count, int x, int y)
{
    /* Reject unsupported contact counts and invalid coordinates. */
    if(count > CST8XX_NUM_POINTS)
        return false;
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return false;

    /* The idle report has no valid coordinates. */
    if(reg[3] == 0xff && reg[4] == 0xff && reg[5] == 0xff && reg[6] == 0xff)
        return false;

    return true;
}

static void cst8xx_i2c_callback(int status, i2c_descriptor* desc)
{
    (void)desc;

    if(status != I2C_STATUS_OK || !cst_drv.accept_report)
        return;

    const uint8_t* reg = &cst_drv.raw_data[1];

    int count = reg[2];
    int x     = ((reg[3] & 0x0f) << 8) | reg[4];
    int y     = ((reg[5] & 0x0f) << 8) | reg[6];
    int event = (reg[3] >> 6) & 0x3;

    if(count == 0) {
        /* Explicit "no fingers" - report a release. */
        cst8xx_park();
        return;
    }

    if(!cst8xx_valid(reg, count, x, y))
        return;         /* unsupported or invalid report: keep the previous state */

    cst8xx_state.nr_points       = count;
    cst8xx_state.points[0].event = event;
    cst8xx_state.points[0].pos_x = x;
    cst8xx_state.points[0].pos_y = y;
}

void cst8xx_init(void)
{
    /* Initialize stuff */
    memset(&cst_drv, 0, sizeof(cst_drv));
    cst_drv.enabled = true;

    memset(&cst8xx_state, 0, sizeof(struct cst8xx_state));
    cst8xx_park();

    /* Reserve bus management cookie */
    cst_drv.i2c_cookie = i2c_async_reserve_cookies(CST8XX_BUS, 1);

    /* Prep an I2C descriptor to read touch data */
    cst_drv.i2c_desc.slave_addr = CST8XX_ADDR;
    cst_drv.i2c_desc.bus_cond   = I2C_START | I2C_STOP;
    cst_drv.i2c_desc.tran_mode  = I2C_READ;
    cst_drv.i2c_desc.buffer[0]  = &cst_drv.raw_data[0];
    cst_drv.i2c_desc.count[0]   = 1;
    cst_drv.i2c_desc.buffer[1]  = &cst_drv.raw_data[1];
    cst_drv.i2c_desc.count[1]   = CST8XX_BLOCK_LEN;
    cst_drv.i2c_desc.callback   = cst8xx_i2c_callback;
    cst_drv.i2c_desc.arg        = 0;
    cst_drv.i2c_desc.next       = NULL;

    /* Set I2C register address */
    cst_drv.raw_data[0] = 0x00;
}

void cst8xx_enable(bool en)
{
    int irq = disable_irq_save();
    cst_drv.enabled = en;
    if(!en) {
        /* An outstanding read stays invalid across re-enable. */
        cst_drv.accept_report = false;
        cst8xx_park();
    }
    restore_irq(irq);
}

void cst8xx_irq_handler(void)
{
    int irq = disable_irq_save();
    if(cst_drv.enabled) {
        /* I2C_Q_ONCE rejects new reads until the old callback has run. */
        if(i2c_async_queue(CST8XX_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                          cst_drv.i2c_cookie, &cst_drv.i2c_desc) == I2C_RC_OK)
            cst_drv.accept_report = true;
    }
    restore_irq(irq);
}
