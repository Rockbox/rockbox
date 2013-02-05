/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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
#ifndef _EQ_H
#define _EQ_H

/* => support from 3 to 32 bands */
#define EQ_NUM_BANDS 10

struct eq_band_setting
{
    int cutoff; /* Hz */
    int q;
    int gain;   /* +/- dB */
};

/** DSP interface **/
void dsp_set_eq_precut(int precut);
void dsp_set_eq_coefs(int band, const struct eq_band_setting *setting);
void dsp_eq_enable(bool enable);

#endif /* _EQ_H */
