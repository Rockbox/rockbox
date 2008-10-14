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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "kernel.h"
#include "tuner.h" /* tuner abstraction interface */
#include "fmradio.h" /* physical interface driver */
#include "sound.h"
#include "mas.h"

#define DEFAULT_IN1 0x100003 /* Mute */
#define DEFAULT_IN2 0x140884 /* 5kHz, 7.2MHz crystal */
#define PLL_FREQ_STEP 10000

static int fm_in1;
static int fm_in2;
static int fm_present = -1; /* unknown */

/* tuner abstraction layer: set something to the tuner */
int s1a0903x01_set(int setting, int value)
{
    int val = 1;

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
            int if_freq = 4 * mas_get_pllfreq();

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

        case RADIO_SCAN_FREQUENCY:
            /* Tune in and delay */
            s1a0903x01_set(RADIO_FREQUENCY, value);
            sleep(1);
            /* Start IF measurement */
            fm_in1 |= 4;
            fmradio_set(1, fm_in1);
            sleep(1);
            val = s1a0903x01_get(RADIO_TUNED);
            break;

        case RADIO_MUTE:
            fm_in1 = (fm_in1 & 0xfffffffe) | (value?1:0);
            fmradio_set(1, fm_in1);
            break;

        case RADIO_FORCE_MONO:
            fm_in2 = (fm_in2 & 0xfffffffb) | (value?0:4);
            fmradio_set(2, fm_in2);
            break;
        /* NOTE: These were only zeroed when starting the tuner from OFF
                 but the default values already set them to 0. */
#if 0
        case S1A0903X01_IF_MEASUREMENT:
            fm_in1 = (fm_in1 & 0xfffffffb) | (value?4:0);
            fmradio_set(1, fm_in1);
            break;

        case S1A0903X01_SENSITIVITY:
            fm_in2 = (fm_in2 & 0xffff9fff) | ((value & 3) << 13);
            fmradio_set(2, fm_in2);
            break;
#endif
        default:
            val = -1;
    }

    return val;
}

/* tuner abstraction layer: read something from the tuner */
int s1a0903x01_get(int setting)
{
    int val = -1;
    switch(setting)
    {
        case RADIO_PRESENT:
            if (fm_present == -1)
            {
#ifdef HAVE_TUNER_PWR_CTRL
                bool fmstatus = tuner_power(true);
#endif
                /* 5kHz, 7.2MHz crystal, test mode 1 */
                fmradio_set(2, 0x140885);
                fm_present = (fmradio_read(0) == 0x140885);
#ifdef HAVE_TUNER_PWR_CTRL
                if (!fmstatus)
                    tuner_power(false);
#endif
            }

            val = fm_present;
            break;

        case RADIO_TUNED:
            val = fmradio_read(3);
            val = abs(10700 - ((val & 0x7ffff) / 8)) < 50; /* convert to kHz */
            break;

        case RADIO_STEREO:
            val = fmradio_read(3);
            val = ((val & 0x100000) ? true : false);
            break;
    }
    return val;
}
