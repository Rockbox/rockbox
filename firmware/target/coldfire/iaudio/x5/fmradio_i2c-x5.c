/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the Philips TEA5767 in iAudio x5
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

#if (CONFIG_TUNER & TEA5767)

#include "i2c-coldfire.h"

int fmradio_i2c_write(unsigned char address, const unsigned char* buf,
                      int count)
{
    return i2c_write(I2C_IFACE_0, address, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read(I2C_IFACE_0, address, buf, count);
}

#endif
