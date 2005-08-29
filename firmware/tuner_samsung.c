/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner "middleware" for Samsung S1A0903X01 chip
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h" /* physical interface driver */
#include "mpeg.h"
#include "sound.h"

#define DEFAULT_IN1 0x100003 /* Mute */
#define DEFAULT_IN2 0x140884 /* 5kHz, 7.2MHz crystal */
#define PLL_FREQ_STEP 10000

static int fm_in1;
static int fm_in2;

/* tuner abstraction layer: set something to the tuner */
void samsung_set(int setting, int value)
{
    switch(setting)
    {
        case RADIO_SLEEP:
            if (!value)
            {   /* wakeup: just unit */
                fm_in1 = DEFAULT_IN1;
                fm_in2 = DEFAULT_IN2;
                fmradio_set(1, fm_in1);
                fmradio_set(2, fm_in2);
            }
            /* else we have no sleep mode? */
            break;

        case RADIO_FREQUENCY:
        {
            int pll_cnt;
#if CONFIG_CODEC == MAS3587F
            /* Shift the MAS internal clock away for certain frequencies to
             * avoid interference. */
            int pitch = 1000;
            
            /* 4th harmonic falls in the FM frequency range */
            int if_freq = 4 * mpeg_get_mas_pllfreq();

            /* shift the mas harmonic >= 300 kHz away using the direction
             * which needs less shifting. */
            if (value < if_freq)
            {
                if (if_freq - value < 300000)
                    pitch = 1003 - (if_freq - value) / 100000;
            }
            else
            {
                if (value - if_freq < 300000)
                    pitch = 997 + (value - if_freq) / 100000;
            }
            sound_set_pitch(pitch);
#endif
            /* We add the standard Intermediate Frequency 10.7MHz
            ** before calculating the divisor
            ** The reference frequency is set to 50kHz, and the VCO
            ** output is prescaled by 2.
            */
    
            pll_cnt = (value + 10700000) / (PLL_FREQ_STEP/2) / 2;

            /* 0x100000 == FM mode
            ** 0x000002 == Microprocessor controlled Mute
            */
            fm_in1 = (fm_in1 & 0xfff00007) | (pll_cnt << 3);
            fmradio_set(1, fm_in1);
            break;
        }

        case RADIO_MUTE:
            fm_in1 = (fm_in1 & 0xfffffffe) | (value?1:0);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_IF_MEASUREMENT:
            fm_in1 = (fm_in1 & 0xfffffffb) | (value?4:0);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_SENSITIVITY:
            fm_in2 = (fm_in2 & 0xffff9fff) | ((value & 3) << 13);
            fmradio_set(2, fm_in2);
            break;

        case RADIO_FORCE_MONO:
            fm_in2 = (fm_in2 & 0xfffffffb) | (value?0:4);
            fmradio_set(2, fm_in2);
            break;
    }
}

/* tuner abstraction layer: read something from the tuner */
int samsung_get(int setting)
{
    int val = -1;
    switch(setting)
    {
        case RADIO_PRESENT:
            fmradio_set(2, 0x140885); /* 5kHz, 7.2MHz crystal, test mode 1 */
            val = (fmradio_read(0) == 0x140885);
            break;

        case RADIO_TUNED:
            val = fmradio_read(3);
            val = abs(10700 - ((val & 0x7ffff) / 8)) < 50; /* convert to kHz */
            break;

        case RADIO_STEREO:
            val = fmradio_read(3);
            val = ((val & 0x100000) ? true : false);
    }
    return val;
}
