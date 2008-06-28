/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#ifndef _WM8758_H
#define _WM8758_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -570
#define VOLUME_MAX  60

#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | BASS_CUTOFF_CAP | TREBLE_CUTOFF_CAP)

extern int tenthdb2master(int db);
extern int tenthdb2mixer(int db);

extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_set_mixer_vol(int channel1, int channel2);
extern void audiohw_set_nsorder(int order);
extern void audiohw_set_sample_rate(int sampling_control);

#define RESET      0x00
#define PWRMGMT1   0x01
#define PWRMGMT2   0x02
#define PWRMGMT3   0x03
#define AINTFCE    0x04
#define CLKCTRL    0x06
#define SRATECTRL  0x07
#define DACCTRL    0x0a
#define INCTRL     0x2c
#define LINPGAVOL  0x2d
#define RINPGAVOL  0x2e
#define LADCBOOST  0x2f
#define RADCBOOST  0x30
#define OUTCTRL    0x31
#define LOUTMIX    0x32
#define ROUTMIX    0x33

#define LOUT1VOL   0x34
#define ROUT1VOL   0x35
#define LOUT2VOL   0x36
#define ROUT2VOL   0x37

#define PLLN       0x24
#define PLLK1      0x25
#define PLLK2      0x26
#define PLLK3      0x27

#define EQ1        0x12
#define EQ2        0x13
#define EQ3        0x14
#define EQ4        0x15
#define EQ5        0x16
#define EQ_GAIN_MASK       0x001f
#define EQ_CUTOFF_MASK     0x0060
#define EQ_GAIN_VALUE(x)   (((-x) + 12) & 0x1f)
#define EQ_CUTOFF_VALUE(x) ((((x) - 1) & 0x03) << 5)

/* Register settings for the supported samplerates: */
#define WM8758_8000HZ      0x4d
#define WM8758_12000HZ     0x61
#define WM8758_16000HZ     0x55
#define WM8758_22050HZ     0x77
#define WM8758_24000HZ     0x79
#define WM8758_32000HZ     0x59
#define WM8758_44100HZ     0x63
#define WM8758_48000HZ     0x41
#define WM8758_88200HZ     0x7f
#define WM8758_96000HZ     0x5d

#endif /* _WM8758_H */
