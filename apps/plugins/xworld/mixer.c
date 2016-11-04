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

#include "mixer.h"
#include "serializer.h"
#include "sys.h"

static int8_t addclamp(int a, int b) {
    int add = a + b;
    if (add < -128) {
        add = -128;
    }
    else if (add > 127) {
        add = 127;
    }
    return (int8_t)add;
}

void mixer_create(struct Mixer* mx, struct System *stub)
{
    mx->sys = stub;
}

static void mixer_mixCallback(void *param, uint8_t *buf, int len);

void mixer_init(struct Mixer* mx) {
    rb->memset(mx->_channels, 0, sizeof(mx->_channels));
    if(!mx->sys)
    {
        error("in mixer sys is NULL");
    }
    mx->_mutex = sys_createMutex(mx->sys);
    sys_startAudio(mx->sys, mixer_mixCallback, mx);
}

void mixer_free(struct Mixer* mx) {
    mixer_stopAll(mx);
    sys_stopAudio(mx->sys);
    sys_destroyMutex(mx->sys, mx->_mutex);
}

void mixer_playChannel(struct Mixer* mx, uint8_t channel, const struct MixerChunk *mc, uint16_t freq, uint8_t volume) {
    debug(DBG_SND, "mixer_playChannel(%d, %d, %d)", channel, freq, volume);
    assert(channel < AUDIO_NUM_CHANNELS);

    /* FW: the mutex code was converted 1:1 from C++ to C, leading to the ugly calls */
    /*     to constructors/destructors as seen here */

    struct MutexStack_t ms;
    MutexStack(&ms, mx->sys, mx->_mutex);

    struct MixerChannel *ch = &mx->_channels[channel];
    ch->active = true;
    ch->volume = volume;
    ch->chunk = *mc;
    ch->chunkPos = 0;
    ch->chunkInc = (freq << 8) / sys_getOutputSampleRate(mx->sys);

    MutexStack_destroy(&ms);
}

void mixer_stopChannel(struct Mixer* mx, uint8_t channel) {
    debug(DBG_SND, "mixer_stopChannel(%d)", channel);
    assert(channel < AUDIO_NUM_CHANNELS);

    struct MutexStack_t ms;
    MutexStack(&ms, mx->sys, mx->_mutex);

    mx->_channels[channel].active = false;

    MutexStack_destroy(&ms);
}

void mixer_setChannelVolume(struct Mixer* mx, uint8_t channel, uint8_t volume) {
    debug(DBG_SND, "mixer_setChannelVolume(%d, %d)", channel, volume);
    assert(channel < AUDIO_NUM_CHANNELS);

    struct MutexStack_t ms;
    MutexStack(&ms, mx->sys, mx->_mutex);

    mx->_channels[channel].volume = volume;

    MutexStack_destroy(&ms);
}

void mixer_stopAll(struct Mixer* mx) {
    debug(DBG_SND, "mixer_stopAll()");

    struct MutexStack_t ms;
    MutexStack(&ms, mx->sys, mx->_mutex);

    for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
        mx->_channels[i].active = false;
    }

    MutexStack_destroy(&ms);
}

/* Mx is SDL callback. Called in order to populate the buf with len bytes. */
/* The mixer iterates through all active channels and combine all sounds. */

/* Since there is no way to know when SDL will ask for a buffer fill, we need */
/* to synchronize with a mutex so the channels remain stable during the execution */
/* of this method. */
static void mixer_mix(struct Mixer* mx, int8_t *buf, int len) {
    int8_t *pBuf;

    /* disabled because this will be called in IRQ */
    /*sys_lockMutex(mx->sys, mx->_mutex);*/

    /* Clear the buffer since nothing guarantees we are receiving clean memory. */
    rb->memset(buf, 0, len);

    for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
        struct MixerChannel *ch = &mx->_channels[i];
        if (!ch->active)
            continue;

        pBuf = buf;
        for (int j = 0; j < len; ++j, ++pBuf) {

            uint16_t p1, p2;
            uint16_t ilc = (ch->chunkPos & 0xFF);
            p1 = ch->chunkPos >> 8;
            ch->chunkPos += ch->chunkInc;

            if (ch->chunk.loopLen != 0) {
                if (p1 == ch->chunk.loopPos + ch->chunk.loopLen - 1) {
                    debug(DBG_SND, "Looping sample on channel %d", i);
                    ch->chunkPos = p2 = ch->chunk.loopPos;
                } else {
                    p2 = p1 + 1;
                }
            } else {
                if (p1 == ch->chunk.len - 1) {
                    debug(DBG_SND, "Stopping sample on channel %d", i);
                    ch->active = false;
                    break;
                } else {
                    p2 = p1 + 1;
                }
            }
            /* interpolate */
            int8_t b1 = *(int8_t *)(ch->chunk.data + p1);
            int8_t b2 = *(int8_t *)(ch->chunk.data + p2);
            int8_t b = (int8_t)((b1 * (0xFF - ilc) + b2 * ilc) >> 8);

            /* set volume and clamp */
            *pBuf = addclamp(*pBuf, (int)b * ch->volume / 0x40);  /* 0x40=64 */
        }

    }

    /*sys_unlockMutex(mx->sys, mx->_mutex);*/
}

static void mixer_mixCallback(void *param, uint8_t *buf, int len) {
    mixer_mix((struct Mixer*)param, (int8_t *)buf, len);
}

void mixer_saveOrLoad(struct Mixer* mx, struct Serializer *ser) {
    sys_lockMutex(mx->sys, mx->_mutex);
    for (int i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
        struct MixerChannel *ch = &mx->_channels[i];
        struct Entry entries[] = {
            SE_INT(&ch->active, SES_BOOL, VER(2)),
            SE_INT(&ch->volume, SES_INT8, VER(2)),
            SE_INT(&ch->chunkPos, SES_INT32, VER(2)),
            SE_INT(&ch->chunkInc, SES_INT32, VER(2)),
            SE_PTR(&ch->chunk.data, VER(2)),
            SE_INT(&ch->chunk.len, SES_INT16, VER(2)),
            SE_INT(&ch->chunk.loopPos, SES_INT16, VER(2)),
            SE_INT(&ch->chunk.loopLen, SES_INT16, VER(2)),
            SE_END()
        };
        ser_saveOrLoadEntries(ser, entries);
    }
    sys_unlockMutex(mx->sys, mx->_mutex);
};
