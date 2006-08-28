/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef RECORDING_H
#define RECORDING_H

bool recording_screen(bool no_source);
char *rec_create_filename(char *buf);
int rec_create_directory(void);

#if CONFIG_CODEC == SWCODEC
/* selects an audio source for recording or playback */
#define SRCF_PLAYBACK         0x0000    /* default */
#define SRCF_RECORDING        0x1000
#ifdef CONFIG_TUNER
/* for AUDIO_SRC_FMRADIO */
#define SRCF_FMRADIO_PLAYING  0x0000    /* default */
#define SRCF_FMRADIO_PAUSED   0x2000
#endif
void rec_set_source(int source, int flags);
#endif /* CONFIG_CODEC == SW_CODEC */

/* steals mp3 buffer, sets source and then options */
/* SRCF_RECORDING is implied */
void rec_set_recording_options(int frequency, int quality,
                               int source, int source_flags,
                               int channel_mode, bool editable,
                               int prerecord_time);

/* steals mp3 buffer, creates unique filename and starts recording */
void rec_record(void);

/* creates unique filename and starts recording */
void rec_new_file(void);

#endif
