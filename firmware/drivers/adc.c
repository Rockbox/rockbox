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

static int current_channel;
static unsigned short adcdata[NUM_ADC_CHANNELS];
static unsigned int adcreg[NUM_ADC_CHANNELS] =
{
    ADDRAH_ADDR, ADDRBH_ADDR, ADDRCH_ADDR, ADDRDH_ADDR,
    ADDRAH_ADDR, ADDRBH_ADDR, ADDRCH_ADDR, ADDRDH_ADDR
};

static void adc_tick(void)
{
    /* Read the data that has bee converted since the last tick */
    adcdata[current_channel] =
	*(unsigned short *)adcreg[current_channel] >> 6;

    /* Start a conversion on the next channel */
    current_channel++;
    if(current_channel == NUM_ADC_CHANNELS)
        current_channel = 0;
    ADCSR = ADCSR_ADST | current_channel;
}

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

/* Batch convert 4 analog channels. If lower is true, convert AN0-AN3, 
 * otherwise AN4-AN7. 
 */
static void adc_batch_convert(bool lower)
{
    int reg = lower ? 0 : 4;
    volatile unsigned short* ANx = ((unsigned short*) ADDRAH_ADDR);
    int i;

    ADCSR = ADCSR_ADST | ADCSR_SCAN | ADCSR_CKS | reg | 3;

    /* Busy wait until conversion is complete. A bit ugly perhaps, but 
     * we should only need to wait about 4 * 14 µs  */
    while(!(ADCSR & ADCSR_ADF))
    {
    }

    /* Stop scanning */
    ADCSR = 0;

    for (i = 0; i < 4; i++)
    {
        /* Read converted values */
        adcdata[reg++] = ANx[i] >> 6;
    }
}

void adc_init(void)
{
    int i;
    ADCR  = 0x7f; /* No external trigger; other bits should be 1 according to the manual... */

    current_channel = 0;

    /* Do a first scan to initialize all values */
    /* AN0 to AN3 */
    adc_batch_convert(true);
    /* AN4 to AN7 */
    adc_batch_convert(false);
    
    tick_add_task(adc_tick);
}
