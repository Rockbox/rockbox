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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "debug.h"

#ifdef CONFIG_TUNER

static int fmstatus = 0;

static int fmradio_reg[3];

int fmradio_read(int addr)
{
    if(addr == 0)
        return fmradio_reg[2]; /* To please the hardware detection */
    else
    {
        if(addr == 3)
        {
            /* Fake a good radio station at 99.4MHz */
            if(((fmradio_reg[1] >> 3) & 0xffff) == 11010)
                return 0x100000 | 85600;
        }
    }
    return 0;
}

void fmradio_set(int addr, int data)
{
    fmradio_reg[addr] = data;
}

void fmradio_set_status(int status)
{
    fmstatus = status;
}

int fmradio_get_status(void)
{
    return fmstatus;
}

#endif
