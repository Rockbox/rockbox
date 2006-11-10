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
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "adc.h"

static unsigned short adcdata[NUM_ADC_CHANNELS];

unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static void adc_tick(void)
{
    if (ADCST & 0x10) {
        adcdata[0] = ADCCH0 & 0x3ff;
        adcdata[1] = ADCCH1 & 0x3ff;
        adcdata[2] = ADCCH2 & 0x3ff;
        adcdata[3] = ADCCH3 & 0x3ff;
        adcdata[4] = ADCCH4 & 0x3ff;
        ADCST = 0xa;
    }
}

void adc_init(void)
{
    ADCR24 = 0xaaaaa;
    ADCR28 = 0;
    ADCST = 2;
    ADCST = 0xa;

    while (!(ADCST & 0x10));
    adc_tick();
  
    tick_add_task(adc_tick);
}

