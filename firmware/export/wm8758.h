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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _WM8758_H
#define _WM8758_H

extern void wmcodec_reset(void);
extern int wmcodec_init(void);
extern void wmcodec_enable_output(bool enable);
extern int wmcodec_set_master_vol(int vol_l, int vol_r);
extern int wmcodec_set_mixer_vol(int channel1, int channel2);
extern void wmcodec_set_bass(int value);
extern void wmcodec_set_treble(int value);
extern int wmcodec_mute(int mute);
extern void wmcodec_close(void);
extern void wmcodec_set_nsorder(int order);
extern void wmcodec_set_sample_rate(int sampling_control);

extern void wmcodec_enable_recording(bool source_mic);
extern void wmcodec_disable_recording(void);
extern void wmcodec_set_recvol(int left, int right, int type);
extern void wmcodec_set_monitor(int enable);

#define RESET      0x00
#define PWRMGMT1   0x01
#define PWRMGMT2   0x02
#define PWRMGMT3   0x03
#define AINTFCE    0x04
#define CLKCTRL    0x06
#define SRATECTRL  0x07
#define DACCTRL    0x0a
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
