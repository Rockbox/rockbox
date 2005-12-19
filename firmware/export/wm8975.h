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

#ifndef _WM8975_H
#define _WM8975_H

extern void wm8975_reset(void);
extern int wm8975_init(void);
extern void wm8975_enable_output(bool enable);
extern int wm8975_set_master_vol(int vol_l, int vol_r);
extern void wm8975_get_master_vol(int* vol_l, int* vol_r);
extern int wm8975_set_mixer_vol(int channel1, int channel2);
extern void wm8975_set_bass(int value);
extern void wm8975_set_treble(int value);
extern int wm8975_mute(int mute);
extern void wm8975_close(void);
extern void wm8975_set_nsorder(int order);
extern void wm8975_set_sample_rate(int sampling_control);

extern void wm8975_enable_recording(bool source_mic);
extern void wm8975_disable_recording(void);
extern void wm8975_set_recvol(int left, int right, int type);
extern void wm8975_set_monitor(int enable);

/* Register settings for the supported samplerates: */
#define WM8975_8000HZ      0x4d
#define WM8975_12000HZ     0x61
#define WM8975_16000HZ     0x55
#define WM8975_22050HZ     0x77
#define WM8975_24000HZ     0x79
#define WM8975_32000HZ     0x59
#define WM8975_44100HZ     0x63
#define WM8975_48000HZ     0x41
#define WM8975_88200HZ     0x7f
#define WM8975_96000HZ     0x5d

#endif /* _WM8975_H */
