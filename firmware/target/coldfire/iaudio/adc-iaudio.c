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

/* get remaining 2 bits and return 10 bit value */
static int get_10bit_voltage(int msbdata)
{
    int data = msbdata << 2;
    data |= pcf50606_read(0x31) & 0x3;
    return data;
}

unsigned short adc_scan(int channel)
{
    static const int adcc2_parms[] =
    {
        [ADC_BUTTONS] = 0x81 | (5 << 1), /* 8b  - ADCIN2 */
        [ADC_REMOTE]  = 0x81 | (6 << 1), /* 8b  - ADCIN3 */
        [ADC_BATTERY] = 0x01 | (0 << 1), /* 10b - BATVOLT, resistive divider */
    };

    int level;
    int data;

    level = disable_irq_save();

    pcf50606_write(0x2f, adcc2_parms[channel]);
    data = pcf50606_read(0x30);

    if (channel == ADC_BATTERY)
        data = get_10bit_voltage(data);

    restore_irq(level);

    return (unsigned short)data;
}
