/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "adc.h"
#include "kernel.h"
#include "i2c-pp.h"
#include "as3514.h"

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    unsigned short data = 0;

    if ((unsigned)channel < NUM_ADC_CHANNELS)
    {
        spinlock_lock(&i2c_spin);

        /* Select channel */
        if (pp_i2c_send( AS3514_I2C_ADDR, ADC_0, (channel << 4)) >= 0)
        {
            unsigned char buf[2];

            /* Read data */
            if (i2c_readbytes( AS3514_I2C_ADDR, ADC_0, 2, buf) >= 0)
            {
                data = (((buf[0] & 0x3) << 8) | buf[1]);
            }
        }

        spinlock_unlock(&i2c_spin);
    }
    
    return data;
}

void adc_init(void)
{
}
