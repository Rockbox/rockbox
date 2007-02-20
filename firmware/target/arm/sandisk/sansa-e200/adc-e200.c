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
#include "i2c-pp.h"
#include "logf.h"

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    unsigned char bat[2];
    unsigned short data = 0;

    switch( channel)
    {
        case ADC_UNREG_POWER:
            pp_i2c_send( 0x46, 0x2e, 0x0);
            pp_i2c_send( 0x46, 0x27, 0x1);
            i2c_readbytes( 0x46, 0x2e, 2, bat);
            data = ((bat[0]<<8) | bat[1]);
            break;
    }
    return data;
}

void adc_init(void)
{
    /* FIXME: Add initialization of the ADC */
}
