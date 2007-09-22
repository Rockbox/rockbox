/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "cpu.h"
#include "adc-target.h"
#include "kernel.h"

/* prototypes */
static void adc_tick(void);

void adc_init(void)
{
    /* attach the adc reading to the tick */
    tick_add_task(adc_tick);
}

/* Called to get the recent ADC reading */
inline unsigned short adc_read(int channel)
{
    return (short)channel;
}

/* add this to the tick so that the ADC converts are done in the background */
static void adc_tick(void)
{
}



  
