/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#include <stdbool.h>

#include "id3.h"
#include "mp3data.h"

#define CODEC_IDX_AUDIO     0
#define CODEC_IDX_VOICE     1

/* Not yet implemented. */
#define CODEC_SET_AUDIOBUF_WATERMARK    4

#if MEM > 1
#define MAX_TRACK       128
#else
#define MAX_TRACK       32
#endif

#define MAX_TRACK_MASK  (MAX_TRACK-1)

#define PLAYBACK_MAX_EVENTS  4
enum PLAYBACK_EVENT_TYPE {
    PLAYBACK_EVENT_TRACK_BUFFER,
    PLAYBACK_EVENT_TRACK_FINISH,
    PLAYBACK_EVENT_TRACK_CHANGE,
};

/* Functions */
const char * get_codec_filename(int cod_spec);
void playback_add_event(enum PLAYBACK_EVENT_TYPE type, void (*handler));
void playback_remove_event(enum PLAYBACK_EVENT_TYPE type, void (*handler));
void voice_wait(void);

#if CONFIG_CODEC == SWCODEC /* This #ifdef is better here than gui/gwps.c */
extern void audio_next_dir(void);
extern void audio_prev_dir(void);
#else
#define audio_next_dir() 
#define audio_prev_dir()
#endif

#endif


