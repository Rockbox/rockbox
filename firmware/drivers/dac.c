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
#include "i2c.h"
#include "debug.h"
#include "dac.h"

int dac_volume(unsigned int volume)
{
    int ret = 0;
    unsigned char buf[3];

    i2c_begin();

    if(volume > 0x38)
        volume = 0x38;

    buf[0] = DAC_REG_WRITE | DAC_AVOL;
    buf[1] = (volume & 0x3f) | 0x40; /* Deemphasis ON */
    buf[2] = volume & 0x3f;

    /* send read command */
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
** Bit3:  0 = AUX1 off  1 = AUX2 on
** Bit2:  0 = DAC off   1 = DAC on
** Bit1:  0 = stereo    1 = mono
** Bit0:  0 = normal right amp   1 = inverted right amp
******************************************************************/
int dac_config(int value)
{
    int ret = 0;
    unsigned char buf[2];

    i2c_begin();

    buf[0] = DAC_REG_WRITE | DAC_GCFG;
    buf[1] = value;

    /* send read command */
    if (i2c_write(DAC_DEV_WRITE,buf,2))
    {
        ret = -1;
    }

    i2c_end();
    return 0;
}
