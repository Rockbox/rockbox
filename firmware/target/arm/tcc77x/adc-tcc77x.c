/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "string.h"
#include "adc.h"

/**************************************************************************
 ** The A/D conversion is done every tick, in three steps:
 **
 ** 1) On the tick interrupt, the conversion of channels 0-3 is started, and
 **    the A/D interrupt is enabled.
 **
 ** 2) After the conversion is done, an interrupt
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

static int channel_group;
static unsigned short adcdata[8];

/* Tick task */
static void adc_tick(void)
{
    /* Start a conversion of channels 0-3. This will trigger an interrupt,
       and the interrupt handler will take care of channels 4-7. */

    int i;

    PCLKCFG6 |= (1<<15);   /* Enable ADC clock */

    channel_group = 0;

    /* Start converting the first 4 channels */
    for (i = 0; i < 4; i++)
        ADCCON = i;

}

/* IRQ handler */
void ADC(void)
{
    int num;
    int i;
    uint32_t adc_status;

    do
    {
        adc_status = ADCSTATUS;
        num = (adc_status>>24) & 7;
        if (num) adcdata[(adc_status >> 16) & 0x7] = adc_status & 0x3ff;
    } while (num);


    if (channel_group == 0)
    {
        /* Start conversion of channels 4-7 */
        for (i = 4; i < 8; i++)
            ADCCON = i;

        channel_group = 1;
    }
    else
    {
        PCLKCFG6 &= ~(1<<15);   /* Disable ADC clock */
    }
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

void adc_init(void)
{
    ADCCON = (1<<4);         /* Leave standby mode */

    /* IRQ enable, auto power-down, single-mode */
    ADCCFG |= (1<<3) | (1<<1) | (1<<0);

    /* Unmask ADC IRQ */
    IEN |= ADC_IRQ_MASK;

    tick_add_task(adc_tick);

    sleep(2);   /* Ensure adc_data[] contains data before returning */
}
