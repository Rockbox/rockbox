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
#include "audio.h"

bool in_recording_screen(void);
bool recording_screen(bool no_source);
char *rec_create_filename(char *buf);
int rec_create_directory(void);

struct timer
{
    bool countdown;
    bool timer_display;
    unsigned int days;
    unsigned int hrs;
    unsigned int mins;
    unsigned int secs;
    unsigned int days_rpt;
    unsigned int hrs_rpt;
    unsigned int mins_rpt;
    bool repeater;
};

struct timer *get_timerstat(void);

/* If true, start recording automatically when recording_sreen() is entered */
extern bool recording_start_automatic;

#if CONFIG_CODEC == SWCODEC
/* handles device powerup and sets audio source */
void rec_set_source(int source, unsigned flags);
#endif /* CONFIG_CODEC == SW_CODEC */

struct audio_recording_options;
/* Initializes a recording_options structure with global settings.
   pass returned data to audio_set_recording_options or 
   rec_set_recording_options */
void rec_init_recording_options(struct audio_recording_options *options);
/* steals mp3 buffer, sets source and then options */
/* SRCF_RECORDING is implied for SWCODEC */
void rec_set_recording_options(struct audio_recording_options *options);

/* steals mp3 buffer, creates unique filename and starts recording */
void rec_record(void);

/* creates unique filename and starts recording */
void rec_new_file(void);

#endif
