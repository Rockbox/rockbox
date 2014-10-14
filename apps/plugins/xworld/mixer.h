/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef __MIXER_H__
#define __MIXER_H__

#include "intern.h"

struct MixerChunk {
    const uint8_t *data;
    uint16_t len;
    uint16_t loopPos;
    uint16_t loopLen;
};

struct MixerChannel {
    uint8_t active;
    uint8_t volume;
    struct MixerChunk chunk;
    uint32_t chunkPos;
    uint32_t chunkInc;
};

struct Serializer;
struct System;

#define AUDIO_NUM_CHANNELS 4

struct Mixer {
    void *_mutex;
    struct System *sys;

    /* Since the virtual machine and SDL are running simultaneously in two different threads */
    /* any read or write to an elements of the sound channels MUST be synchronized with a */
    /* mutex. */
    struct MixerChannel _channels[AUDIO_NUM_CHANNELS];
};

void mixer_create(struct Mixer*, struct System *stub);
void mixer_init(struct Mixer*);
void mixer_free(struct Mixer*);

void mixer_playChannel(struct Mixer*, uint8_t channel, const struct MixerChunk *mc, uint16_t freq, uint8_t volume);
void mixer_stopChannel(struct Mixer*, uint8_t channel);
void mixer_setChannelVolume(struct Mixer*, uint8_t channel, uint8_t volume);
void mixer_stopAll(struct Mixer*);

void mixer_saveOrLoad(struct Mixer*, struct Serializer *ser);

#endif
