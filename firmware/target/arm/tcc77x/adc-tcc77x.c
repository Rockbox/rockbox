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

/* 
  TODO: We probably want to do this on the timer interrupt once we get
        interrupts going - see the sh-adc.c implementation for an example which
        looks like it should work well with the TCC77x.
*/

static unsigned short adcdata[8];

static void adc_do_read(void)
{
    int i;
    uint32_t adc_status;

    PCLKCFG6 |= (1<<15);   /* Enable ADC clock */

    /* Start converting the first 4 channels */
    for (i = 0; i < 4; i++)
        ADCCON = i;

    /* Wait for data to become stable */
    while ((ADCDATA & 0x1) == 0);

    /* Now read the values back */
    for (i=0;i < 4; i++) {
        adc_status = ADCSTATUS;
        adcdata[(adc_status >> 16) & 0x7] = adc_status & 0x3ff;
    }

    PCLKCFG6 &= ~(1<<15);   /* Disable ADC clock */
}

unsigned short adc_read(int channel)
{
    adc_do_read();

    return adcdata[channel];
}

void adc_init(void)
{
    int i;

    ADCCON = (1<<4);         /* Leave standby mode */
    ADCCFG |= 0x00000003;    /* Single-mode, auto power-down */
}
