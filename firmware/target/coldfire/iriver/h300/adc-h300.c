/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "adc.h"
#include "pcf50606.h"

static unsigned char adcdata[NUM_ADC_CHANNELS];

static int adcc2_parms[] =
{
    [ADC_BUTTONS] = 0x80 | (5 << 1) | 1, /* ADCIN2 */
    [ADC_REMOTE]  = 0x80 | (6 << 1) | 1, /* ADCIN3 */
    [ADC_BATTERY] = 0x80 | (0 << 1) | 1, /* BATVOLT, resistive divider */
    [ADC_REMOTEDETECT] = 0x80 | (2 << 1) | 1, /* ADCIN1, resistive divider */
};

unsigned short adc_scan(int channel)
{
    int level = set_irq_level(HIGHEST_IRQ_LEVEL);
    unsigned char data;

    pcf50606_write(0x2f, adcc2_parms[channel]);
    data = pcf50606_read(0x30);

    adcdata[channel] = data;

    set_irq_level(level);
    return data;
}


unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static int adc_counter;

static void adc_tick(void)
{
    if(++adc_counter == HZ)
    {
        adc_counter = 0;
        adc_scan(ADC_BATTERY);
        adc_scan(ADC_REMOTEDETECT); /* Temporary. Remove when the remote
                                       detection feels stable. */
    }
}

void adc_init(void)
{
    or_l(0x80600080, &GPIO_FUNCTION); /* GPIO7:  CS
                                         GPIO21: Data In (to the ADC)
                                         GPIO22: CLK
                                         GPIO31: Data Out (from the ADC) */
    or_l(0x00600080, &GPIO_ENABLE);
    or_l(0x80, &GPIO_OUT);          /* CS high */
    and_l(~0x00400000, &GPIO_OUT);  /* CLK low */

    adc_scan(ADC_BATTERY);
    
    tick_add_task(adc_tick);
}
