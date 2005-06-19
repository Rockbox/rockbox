/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Function names are taken from apps/recorder/recording.c to
 * make the integration later easier..
 * 
 */

#ifndef PCM_RECORD_H
#define PCM_RECORD_H

unsigned long pcm_status(void);

void pcm_init_recording(void);

void pcm_open_recording(void);
void pcm_close_recording(void);


void pcm_set_recording_options(int source, bool enable_waveform);
void pcm_set_recording_gain(int gain, int volume);
                                
void pcm_record(const char *filename);
void pcm_stop_recording(void);

//void pcm_new_file(const char *filename);


unsigned long pcm_recorded_time(void);
unsigned long pcm_num_recorded_bytes(void);
void pcm_pause_recording(void);
void pcm_resume_recording(void);

#endif
