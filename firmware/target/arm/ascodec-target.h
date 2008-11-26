/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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

#ifndef _ASCODEC_TARGET_H
#define _ASCODEC_TARGET_H

#include "config.h"

#ifdef CPU_PP
/* TODO: This header is actually portalplayer specific, and should be
 * moved into an appropriate subdir  */

#include "as3514.h"
#include "i2c-pp.h"

static inline int ascodec_write(unsigned int reg, unsigned int value)
{
    return pp_i2c_send(AS3514_I2C_ADDR, reg, value);
}

static inline int ascodec_read(unsigned int reg)
{
    return i2c_readbyte(AS3514_I2C_ADDR, reg);
}

static inline int ascodec_readbytes(int addr, int len, unsigned char *data)
{
    return i2c_readbytes(AS3514_I2C_ADDR, addr, len, data);
}

static inline void ascodec_lock(void)
{
    i2c_lock();
}

static inline void ascodec_unlock(void)
{
    i2c_unlock();
}

extern void ascodec_suppressor_on(bool on);

#endif /* CPU_PP */

#endif /* !_ASCODEC_TARGET_H */
