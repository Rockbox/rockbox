/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef RADIO_H
#define RADIO_H

#ifndef FMRADIO_H
#include "fmradio.h"
#endif

#if CONFIG_TUNER
void radio_load_presets(char *filename);
void radio_init(void);
bool radio_screen(void);
void radio_start(void);
void radio_pause(void);
void radio_stop(void);
bool radio_hardware_present(void);
bool in_radio_screen(void);

#define MAX_FMPRESET_LEN 27

struct fmstation
{
    int frequency; /* In Hz */
    char name[MAX_FMPRESET_LEN+1];
};

struct fm_region_setting
{
    int lang;
    int freq_min;
    int freq_max;
    int freq_step;
#if (CONFIG_TUNER & TEA5767)
    int deemphasis; /* 0: 50us, 1: 75us */
    int band; /* 0: europe, 1: japan (BL in TEA spec)*/
#endif
};

#endif

#endif
