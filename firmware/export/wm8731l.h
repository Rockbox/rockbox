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

#ifndef _WM8731L_H
#define _WM8731L_H

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

/* Register settings for the supported samplerates: */
#define WM8731L_8000HZ      0x4d
#define WM8731L_32000HZ     0x59
#define WM8731L_44100HZ     0x63
#define WM8731L_48000HZ     0x41
#define WM8731L_88200HZ     0x7f
#define WM8731L_96000HZ     0x5d

#endif /* _WM8975_H */
