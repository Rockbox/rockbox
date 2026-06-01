/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Amaury Pouly
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include "usb_ch9.h"
#include "usb_class_driver.h"

/* NOTE
 *
 * This is USBAudio 1.0. USBAudio 2.0 is notably _not backwards compatible!_
 * USBAudio 1.0 over _USB_ 2.0 is perfectly valid!
 *
 * Relevant specifications are USB 2.0 and USB Audio Class 1.0.
 */

/*
 * usb_audio_get_playing():
 *
 * Returns playing/not playing status of usbaudio.
 */
bool usb_audio_get_playing(void);

/*
 * usb_audio_get_alloc_failed():
 *
 * Return whether the buffer allocation succeeded (0)
 * or failed (1).
 */
bool usb_audio_get_alloc_failed(void);

/*
 * usb_audio_get_playback_sampling_frequency():
 *
 * Return the sample rate currently set.
 */
unsigned long usb_audio_get_playback_sampling_frequency(void);

/*
 * usb_audio_get_main_intf():
 *
 * Return the main usb interface
 */
int usb_audio_get_main_intf(void);

/*
 * usb_audio_get_alt_intf():
 *
 * Return the alternate usb interface
 */
int usb_audio_get_alt_intf(void);

/*
 * usb_audio_get_samplesperframe():
 *
 * Return the samples per frame over the last two feedback cycles
 * This is the samples sent to the mixer.
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_samplesperframe(void);

/*
 * usb_audio_get_samplesperframe():
 *
 * Return the samples per frame over the last two feedback cycles
 * This is the samples received from USB.
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_samples_rx_perframe(void);

/*
 * usb_audio_get_out_ep():
 *
 * Return the out (to device) endpoint
 */
unsigned int usb_audio_get_out_ep(void);

/*
 * usb_audio_get_in_ep():
 *
 * Return the in (to host) endpoint
 */
unsigned int usb_audio_get_in_ep(void);

/*
 * usb_audio_get_prebuffering():
 *
 * Return number of buffers filled ahead of playback
 */
int usb_audio_get_prebuffering(void);

/*
 * usb_audio_get_prebuffering_avg():
 *
 * Return the average number of buffers filled ahead of playback
 * over the last two feedback cycles
 *
 * This is returned in floating point 16.16 type. To convert to float,
 * do ((double)result / (1<<16))
 */
int32_t usb_audio_get_prebuffering_avg(void);

/*
 * usb_audio_get_prebuffering_maxmin():
 *
 * Return the max or min number of buffers filled ahead of playback
 * over the last feedback cycle
 */
int usb_audio_get_prebuffering_maxmin(bool max);

/*
 * usb_audio_get_underflow():
 *
 * Return whether playback is in "underflow" state
 */
bool usb_audio_get_underflow(void);

/*
 * usb_audio_get_overflow():
 *
 * Return whether usb is in "overflow" state
 */
bool usb_audio_get_overflow(void);

/*
 * usb_audio_get_frames_dropped():
 *
 * Return the number of frames which have been dropped during playback
 */
int usb_audio_get_frames_dropped(void);

/*
 * usb_audio_get_cur_volume():
 *
 * Return current audio volume in db
 */
int usb_audio_get_cur_volume(void);

bool usb_audio_get_active(void);

extern struct usb_class_driver usb_cdrv_audio;

#endif
