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
#include "config.h"
#include "sh7034.h"
#include "kernel.h"
#include "thread.h"
#include "adc.h"

/* This driver updates the adcdata[] array by converting one A/D channel
   group on each system tick. Each group is 4 channels, which means that
   it takes 2 ticks to convert all 8 channels. */

static int current_group;
static unsigned short adcdata[NUM_ADC_CHANNELS];

static void adc_tick(void)
{
    /* Copy the data from the previous conversion */
    if(current_group)
    {
        adcdata[4] = ADDRA >> 6;
        adcdata[5] = ADDRB >> 6;
        adcdata[6] = ADDRC >> 6;
        adcdata[7] = ADDRD >> 6;
    }
    else
    {
        adcdata[0] = ADDRA >> 6;
        adcdata[1] = ADDRB >> 6;
        adcdata[2] = ADDRC >> 6;
        adcdata[3] = ADDRD >> 6;
    }

    /* Start converting the next group */
    current_group = !current_group;
    ADCSR = ADCSR_ADST | ADCSR_SCAN | (current_group?4:0) | 3;

    /* The conversion will be ready when we serve the next tick interrupt.
       No need to check ADCSR for finished conversion since the conversion
       will be ready long before the next tick. */
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

void adc_init(void)
{
    ADCR  = 0x7f; /* No external trigger; other bits should be 1 according
                     to the manual... */

    /* Make sure that there is no conversion running */
    ADCSR = 0;

    /* Start with converting group 0 by setting current_group to 1 */
    current_group = 1;
    
    tick_add_task(adc_tick);

    /* Wait until both groups have been converted before we continue,
     so adcdata[] contains valid data */
    sleep(2);
}
