/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "i2c-dm320.h"

#if 0
static int i2c_getack(void)
{
    return 0;
}

static int i2c_start(void)
{
    return 0;
}

static void i2c_stop(void)
{

}

static int i2c_outb(unsigned char byte)
{
    (void) byte;
    return 0;
}
#endif

void i2c_write(int addr, const unsigned char *buf, int count)
{
    (void) addr;
    (void) buf;
    (void) count;
}

void i2c_init(void)
{

}
