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

static unsigned short adcdata[NUM_ADC_CHANNELS];

static int channelnum[] =
{
    5,   /* ADC_BUTTONS (ADCIN2) */
    6,   /* ADC_REMOTE  (ADCIN3) */
    0,   /* ADC_BATTERY (BATVOLT, resistive divider) */
};

unsigned short adc_scan(int channel)
{
    unsigned char data;
    
    pcf50606_write(0x2f, 0x80 | (channelnum[channel] << 1) | 1);
    data = pcf50606_read(0x30);

    adcdata[channel] = data;

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
    }
}

void adc_init(void)
{
    adc_scan(ADC_BATTERY);
    tick_add_task(adc_tick);
}
