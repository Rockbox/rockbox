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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "adc.h"
#include "kernel.h"
#include "ascodec.h"
#include "as3514.h"

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    unsigned short data = 0;

    if ((unsigned)channel < NUM_ADC_CHANNELS)
    {
        ascodec_lock();

        /* Select channel */
        if (ascodec_write(AS3514_ADC_0, (channel << 4)) >= 0)
        {
            unsigned char buf[2];

            /*
             * The AS3514 ADC will trigger an interrupt when the conversion
             * is finished, if the corresponding enable bit in IRQ_ENRD2
             * is set.
             * Previously the code did not wait and this apparently did
             * not pose any problems, but this should be more correct.
             * Without the wait the data read back may be completely or
             * partially (first one of the two bytes) stale.
             */
            ascodec_wait_adc_finished();


            /* Read data */
            if (ascodec_readbytes(AS3514_ADC_0, 2, buf) >= 0)
            {
                data = (((buf[0] & 0x3) << 8) | buf[1]);
            }
        }

        ascodec_unlock();
    }
    
    return data;
}

void adc_init(void)
{
}
