/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2009 by Mark Arigo
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
#include "i2c-pp.h"
#include "fmradio_i2c.h"

/* The TEA5767 uses 5 bytes, but the pp-i2c will only read/write 4 bytes
   at a time. The tuner doesn't like it when the i2c resets to send the 5th
   byte. So, we can only read/write the first 4 bytes. Luckily, on read,
   the 5th byte is reserved and on write we only use that for the deemphasis
   bit (which we'll have to ignore). This is what the OF appears to do too. */

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    (void)count;
    return i2c_sendbytes(address, 4, buf);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    (void)count;
    return i2c_readbytes(address, -1, 4, buf);
}
#endif
