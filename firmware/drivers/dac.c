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
#include "config.h"
#include "stdbool.h"
#include "i2c.h"
#include "debug.h"
#include "dac.h"

#ifdef HAVE_DAC3550A

static bool line_in_enabled = false;
static bool dac_enabled = false;


int dac_volume(unsigned int left, unsigned int right, bool deemph)
{
    int ret = 0;
    unsigned char buf[3];

    i2c_begin();

    if (left > 0x38)
        left = 0x38;
    if (right > 0x38)
        right = 0x38;

    buf[0] = DAC_REG_WRITE | DAC_AVOL;
    buf[1] = (left & 0x3f) | (deemph ? 0x40 : 0);
    buf[2] = right & 0x3f;

    /* send write command */
    if (i2c_write(DAC_DEV_WRITE,buf,3))
    {
        ret = -1;
    }

    i2c_end();
    return ret;
}

/******************************************************************
** Bit6:  0 = 3V        1 = 5V
** Bit5:  0 = normal    1 = low power
** Bit4:  0 = AUX2 off  1 = AUX2 on
** Bit3:  0 = AUX1 off  1 = AUX1 on
** Bit2:  0 = DAC off   1 = DAC on
** Bit1:  0 = stereo    1 = mono
** Bit0:  0 = normal right amp   1 = inverted right amp
******************************************************************/
/* dac_config is called once to initialize it. we will apply
   our static settings because of the init flow.
   dac_init -> dac_line_in -> mpeg_init -> dac_config
*/
static int dac_config(void)
{
    int ret = 0;
    unsigned char buf[2];

    i2c_begin();

    buf[0] = DAC_REG_WRITE | DAC_GCFG;
    buf[1] = (dac_enabled ? 0x04 : 0) |
        (line_in_enabled ? 0x08 : 0);

    /* send write command */
    if (i2c_write(DAC_DEV_WRITE,buf,2))
    {
        ret = -1;
    }

    i2c_end();
    return ret;
}

void dac_enable(bool enable)
{
    dac_enabled = enable;
    dac_config();
}

void dac_line_in(bool enable)
{
    line_in_enabled = enable;
    dac_config();
}

void dac_init(void)
{
    unsigned char buf[2];

    i2c_begin();

    buf[0] = DAC_REG_WRITE | DAC_SR_REG;
    buf[1] = 0x07;

    /* send write command */
    i2c_write(DAC_DEV_WRITE,buf,2);
    i2c_end();
}

#endif
