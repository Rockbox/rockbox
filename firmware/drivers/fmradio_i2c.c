/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Physical interface of the Philips TEA5767 in Archos Ondio
 *
 * Copyright (C) 2004 by Jörg Hohensohn
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "debug.h"
#include "system.h"

#ifdef CONFIG_TUNER

/* reads 5 byte */
void fmradio_i2c_read(unsigned char* p_data)
{
    (void)p_data;
}

/* writes 5 bytes */
void fmradio_i2c_set(const unsigned char* p_data)
{
    (void)p_data;
}

#endif
