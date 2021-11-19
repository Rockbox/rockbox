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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#include <stdbool.h>
#include <stdlib.h>
#include "config.h"

/* Including the code for fast previews is entirely optional since it
   does add two more mp3entry's - for certain targets it may be less
   beneficial such as flash-only storage */
#if MEMORYSIZE > 2
#define AUDIO_FAST_SKIP_PREVIEW
#endif

#ifdef HAVE_ALBUMART

#include "bmp.h"
#include "metadata.h"
/*
 * Returns the handle id of the buffered albumart for the given slot id
 **/
int playback_current_aa_hid(int slot);

/*
 * Hands out an albumart slot for buffering albumart using the size
 * int the passed dim struct, it copies the data of dim in order to
 * be safe to be reused for other code
 *
 * The slot may be reused if other code calls this with the same dimensions
 * in dim, so if you change dim release and claim a new slot
 *
 * Save to call from other threads */
int playback_claim_aa_slot(struct dim *dim);

/*
 * Releases the albumart slot with given id
 *
 * Save to call from other threads */
void playback_release_aa_slot(int slot);

/*
 * Tells playback to sync buffered album art dimensions
 *
 * Save to call from other threads */
void playback_update_aa_dims(void);

struct bufopen_bitmap_data {
    struct dim *dim;
    struct mp3_albumart *embedded_albumart;
};

#endif /* HAVE_ALBUMART */

/* Functions */
unsigned int audio_track_count(void);
long audio_filebufused(void);
void audio_pre_ff_rewind(void);
void audio_skip(int direction);

void audio_set_cuesheet(bool enable);
#ifdef HAVE_CROSSFADE
void audio_set_crossfade(int enable);
#endif
#ifdef HAVE_PLAY_FREQ
void audio_set_playback_frequency(int setting);
#endif
#ifdef HAVE_ALBUMART
void set_albumart_mode(int setting);
#endif

size_t audio_get_filebuflen(void);

unsigned int playback_status(void);

#endif /* _PLAYBACK_H */
