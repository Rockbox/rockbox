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

/**************************************************************************
 ** The A/D conversion is done every tick, in three steps:
 **
 ** 1) On the tick interrupt, the conversion of channels 0-3 is started, and
 **    the A/D interrupt is enabled.
 **
 ** 2) After the conversion is done (approx. 256*4 cycles later), an interrupt
 **    is generated at level 1, which is the same level as the tick interrupt
 **    itself. This interrupt will be pending until the tick interrupt is
 **    finished.
 **    When the A/D interrupt is finally served, it will read the results
 **    from the first conversion and start the conversion of channels 4-7.
 **
 ** 3) When the conversion of channels 4-7 is finished, the interrupt is
 **    triggered again, and the results are read. This time, no new
 **    conversion is started, it will be done in the next tick interrupt.
 **
 ** Thus, each channel will be updated HZ times per second.
 **
 *************************************************************************/

static int current_channel;
static unsigned short adcdata[NUM_ADC_CHANNELS];

static void adc_tick(void)
{
    /* Start a conversion of channel group 0. This will trigger an interrupt,
       and the interrupt handler will take care of group 1. */

    current_channel = 0;
    ADCSR = ADCSR_ADST | ADCSR_ADIE | ADCSR_SCAN | 3;
}

#pragma interrupt
void ADITI(void)
{
    if(ADCSR & ADCSR_ADF)
    {
        ADCSR = 0;

        if(current_channel == 0)
        {
            adcdata[0] = ADDRA >> 6;
            adcdata[1] = ADDRB >> 6;
            adcdata[2] = ADDRC >> 6;
            adcdata[3] = ADDRD >> 6;
            current_channel = 4;
            
            /* Convert the next group */
            ADCSR = ADCSR_ADST | ADCSR_ADIE | ADCSR_SCAN | 7;
        }
        else
        {
            adcdata[4] = ADDRA >> 6;
            adcdata[5] = ADDRB >> 6;
            adcdata[6] = ADDRC >> 6;
            adcdata[7] = ADDRD >> 6;
        }
    }
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

void adc_init(void)
{
    ADCR  = 0x7f; /* No external trigger; other bits should be 1 according
                     to the manual... */

    ADCSR = 0;
    
    current_channel = 0;

    /* Enable the A/D IRQ on level 1 */
    IPRE = (IPRE & 0xf0ff) | 0x0100;
    
    tick_add_task(adc_tick);
}
