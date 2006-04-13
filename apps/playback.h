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

#define CODEC_IDX_AUDIO  0
#define CODEC_IDX_VOICE  1

/* Not yet implemented. */
#define CODEC_SET_AUDIOBUF_WATERMARK    4

#define MAX_TRACK 32
struct track_info {
    struct mp3entry id3;       /* TAG metadata */
    char *codecbuf;            /* Pointer to codec buffer */
    size_t codecsize;          /* Codec length in bytes */
    bool has_codec;            /* Does this track have a codec on the buffer */

    size_t buf_idx;            /* Pointer to the track's buffer */
    size_t filerem;            /* Remaining bytes of file NOT in buffer */
    size_t filesize;           /* File total length */
    size_t start_pos;          /* Position to first bytes of file in buffer */
    volatile size_t available; /* Available bytes to read from buffer */

    bool taginfo_ready;        /* Is metadata read */

    bool event_sent;           /* Was this track's buffered event sent */
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

void audio_preinit(void);

#endif


