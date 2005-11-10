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

#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "id3.h"
#include "mp3data.h"

/* Not yet implemented. */
#define CODEC_SET_AUDIOBUF_WATERMARK    4

#define MAX_TRACK 32
struct track_info {
    struct mp3entry id3;     /* TAG metadata */
    char *codecbuf;          /* Pointer to codec buffer */
    long codecsize;        /* Codec length in bytes */
    
    off_t filerem;           /* Remaining bytes of file NOT in buffer */
    off_t filesize;          /* File total length */
    off_t filepos;           /* Read position of file for next buffer fill */
    off_t start_pos;         /* Position to first bytes of file in buffer */
    volatile int available;  /* Available bytes to read from buffer */
    bool taginfo_ready;      /* Is metadata read */
    int playlist_offset;     /* File location in playlist */
    bool event_sent;         /* Has event callback functions been called? */
};

/* Functions */
void audio_set_track_changed_event(void (*handler)(struct mp3entry *id3));
void audio_set_track_buffer_event(void (*handler)(struct mp3entry *id3,
                                                  bool last_track));
void audio_set_track_unbuffer_event(void (*handler)(struct mp3entry *id3,
                                                   bool last_track));
void audio_invalidate_tracks(void);
void voice_init(void);

extern void audio_next_dir(void);
extern void audio_prev_dir(void);

#endif


