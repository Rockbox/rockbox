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

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

extern int tenthdb2master(int db);
extern int tenthdb2mixer(int db);

extern void audiohw_reset(void);
extern void audiohw_enable_output(bool enable);
extern int audiohw_set_master_vol(int vol_l, int vol_r);
extern int audiohw_set_mixer_vol(int channel1, int channel2);
extern void audiohw_set_bass(int value);
extern void audiohw_set_treble(int value);
extern void audiohw_set_nsorder(int order);
extern void audiohw_set_sample_rate(int sampling_control);

extern void audiohw_enable_recording(bool source_mic);
extern void audiohw_disable_recording(void);
extern void audiohw_set_recvol(int left, int right, int type);
extern void audiohw_set_monitor(int enable);

/* Register addresses */
#define LINVOL        0x00
#define RINVOL        0x01
#define LOUTVOL       0x02
#define ROUTVOL       0x03
#define AAPCTRL       0x04  /* Analog audio path control */
#define DACCTRL       0x05
#define PWRMGMT       0x06
#define AINTFCE       0x07
#define SAMPCTRL      0x08
#define ACTIVECTRL    0x09
#define RESET         0x0f

/* Register settings for the supported samplerates: */
#define WM8731L_8000HZ      0x4d
#define WM8731L_32000HZ     0x59
#define WM8731L_44100HZ     0x63
#define WM8731L_48000HZ     0x41
#define WM8731L_88200HZ     0x7f
#define WM8731L_96000HZ     0x5d

#endif /* _WM8975_H */
