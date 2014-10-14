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

#include "sfxplayer.h"
#include "mixer.h"
#include "resource.h"
#include "serializer.h"
#include "sys.h"

void player_create(struct SfxPlayer* sfx, struct Mixer *mix, struct Resource *res, struct System *stub)
{
    sfx->mixer = mix;
    sfx->res = res;
    sfx->sys = stub;
    sfx->_delay = 0;
    sfx->_resNum = 0;
}

void player_init(struct SfxPlayer* sfx) {
    debug(DBG_SND, "sys is 0x%08x", sfx->sys);
    sfx->_mutex = sys_createMutex(sfx->sys);
}

void player_free(struct SfxPlayer* sfx) {
    player_stop(sfx);
    sys_destroyMutex(sfx->sys, sfx->_mutex);
}

void player_setEventsDelay(struct SfxPlayer* sfx, uint16_t delay) {
    debug(DBG_SND, "player_setEventsDelay(%d)", delay);
    struct MutexStack_t ms;
    MutexStack(&ms, sfx->sys, sfx->_mutex);
    sfx->_delay = delay * 60 / 7050;
    MutexStack_destroy(&ms);
}

void player_loadSfxModule(struct SfxPlayer* sfx, uint16_t resNum, uint16_t delay, uint8_t pos) {

    debug(DBG_SND, "player_loadSfxModule(0x%X, %d, %d)", resNum, delay, pos);
    struct MutexStack_t ms;
    MutexStack(&ms, sfx->sys, sfx->_mutex);


    struct MemEntry *me = &sfx->res->_memList[resNum];

    if (me->state == MEMENTRY_STATE_LOADED && me->type == RT_MUSIC) {
        sfx->_resNum = resNum;
        rb->memset(&sfx->_sfxMod, 0, sizeof(struct SfxModule));
        sfx->_sfxMod.curOrder = pos;
        sfx->_sfxMod.numOrder = READ_BE_UINT16(me->bufPtr + 0x3E);
        debug(DBG_SND, "player_loadSfxModule() curOrder = 0x%X numOrder = 0x%X", sfx->_sfxMod.curOrder, sfx->_sfxMod.numOrder);
        for (int i = 0; i < 0x80; ++i) {
            sfx->_sfxMod.orderTable[i] = *(me->bufPtr + 0x40 + i);
        }
        if (delay == 0) {
            sfx->_delay = READ_BE_UINT16(me->bufPtr);
        } else {
            sfx->_delay = delay;
        }
        sfx->_delay = sfx->_delay * 60 / 7050;
        sfx->_sfxMod.data = me->bufPtr + 0xC0;
        debug(DBG_SND, "player_loadSfxModule() eventDelay = %d ms", sfx->_delay);
        player_prepareInstruments(sfx, me->bufPtr + 2);
    } else {
        warning("player_loadSfxModule() ec=0x%X", 0xF8);
    }
    MutexStack_destroy(&ms);
}

void player_prepareInstruments(struct SfxPlayer* sfx, const uint8_t *p) {

    rb->memset(sfx->_sfxMod.samples, 0, sizeof(sfx->_sfxMod.samples));

    for (int i = 0; i < 15; ++i) {
        struct SfxInstrument *ins = &sfx->_sfxMod.samples[i];
        uint16_t resNum = READ_BE_UINT16(p);
        p += 2;
        if (resNum != 0) {
            ins->volume = READ_BE_UINT16(p);
            struct MemEntry *me = &sfx->res->_memList[resNum];
            if (me->state == MEMENTRY_STATE_LOADED && me->type == RT_SOUND) {
                ins->data = me->bufPtr;
                rb->memset(ins->data + 8, 0, 4);
                debug(DBG_SND, "Loaded instrument 0x%X n=%d volume=%d", resNum, i, ins->volume);
            } else {
                error("Error loading instrument 0x%X", resNum);
            }
        }
        p += 2; /* skip volume */
    }
}

void player_start(struct SfxPlayer* sfx) {
    debug(DBG_SND, "player_start()");
    struct MutexStack_t ms;
    MutexStack(&ms, sfx->sys, sfx->_mutex);
    sfx->_sfxMod.curPos = 0;
    sfx->_timerId = sys_addTimer(sfx->sys, sfx->_delay, player_eventsCallback, sfx);
    MutexStack_destroy(&ms);
}

void player_stop(struct SfxPlayer* sfx) {
    debug(DBG_SND, "player_stop()");
    struct MutexStack_t ms;
    MutexStack(&ms, sfx->sys, sfx->_mutex);
    if (sfx->_resNum != 0) {
        sfx->_resNum = 0;
        sys_removeTimer(sfx->sys, sfx->_timerId);
    }
    MutexStack_destroy(&ms);
}

void player_handleEvents(struct SfxPlayer* sfx) {
    struct MutexStack_t ms;
    MutexStack(&ms, sfx->sys, sfx->_mutex);
    uint8_t order = sfx->_sfxMod.orderTable[sfx->_sfxMod.curOrder];
    const uint8_t *patternData = sfx->_sfxMod.data + sfx->_sfxMod.curPos + order * 1024;
    for (uint8_t ch = 0; ch < 4; ++ch) {
        player_handlePattern(sfx, ch, patternData);
        patternData += 4;
    }
    sfx->_sfxMod.curPos += 4 * 4;
    debug(DBG_SND, "player_handleEvents() order = 0x%X curPos = 0x%X", order, sfx->_sfxMod.curPos);
    if (sfx->_sfxMod.curPos >= 1024) {
        sfx->_sfxMod.curPos = 0;
        order = sfx->_sfxMod.curOrder + 1;
        if (order == sfx->_sfxMod.numOrder) {
            sfx->_resNum = 0;
            sys_removeTimer(sfx->sys, sfx->_timerId);
            mixer_stopAll(sfx->mixer);
        }
        sfx->_sfxMod.curOrder = order;
    }
    MutexStack_destroy(&ms);
}

void player_handlePattern(struct SfxPlayer* sfx, uint8_t channel, const uint8_t *data) {
    struct SfxPattern pat;
    rb->memset(&pat, 0, sizeof(struct SfxPattern));
    pat.note_1 = READ_BE_UINT16(data + 0);
    pat.note_2 = READ_BE_UINT16(data + 2);
    if (pat.note_1 != 0xFFFD) {
        uint16_t sample = (pat.note_2 & 0xF000) >> 12;
        if (sample != 0) {
            uint8_t *ptr = sfx->_sfxMod.samples[sample - 1].data;
            if (ptr != 0) {
                debug(DBG_SND, "player_handlePattern() preparing sample %d", sample);
                pat.sampleVolume = sfx->_sfxMod.samples[sample - 1].volume;
                pat.sampleStart = 8;
                pat.sampleBuffer = ptr;
                pat.sampleLen = READ_BE_UINT16(ptr) * 2;
                uint16_t loopLen = READ_BE_UINT16(ptr + 2) * 2;
                if (loopLen != 0) {
                    pat.loopPos = pat.sampleLen;
                    pat.loopData = ptr;
                    pat.loopLen = loopLen;
                } else {
                    pat.loopPos = 0;
                    pat.loopData = 0;
                    pat.loopLen = 0;
                }
                int16_t m = pat.sampleVolume;
                uint8_t effect = (pat.note_2 & 0x0F00) >> 8;
                if (effect == 5) { /* volume up */
                    uint8_t volume = (pat.note_2 & 0xFF);
                    m += volume;
                    if (m > 0x3F) {
                        m = 0x3F;
                    }
                } else if (effect == 6) { /* volume down */
                    uint8_t volume = (pat.note_2 & 0xFF);
                    m -= volume;
                    if (m < 0) {
                        m = 0;
                    }
                }
                mixer_setChannelVolume(sfx->mixer, channel, m);
                pat.sampleVolume = m;
            }
        }
    }
    if (pat.note_1 == 0xFFFD) {
        debug(DBG_SND, "player_handlePattern() _scriptVars[0xF4] = 0x%X", pat.note_2);
        *sfx->_markVar = pat.note_2;
    } else if (pat.note_1 != 0) {
        if (pat.note_1 == 0xFFFE) {
            mixer_stopChannel(sfx->mixer, channel);
        } else if (pat.sampleBuffer != 0) {
            struct MixerChunk mc;
            rb->memset(&mc, 0, sizeof(mc));
            mc.data = pat.sampleBuffer + pat.sampleStart;
            mc.len = pat.sampleLen;
            mc.loopPos = pat.loopPos;
            mc.loopLen = pat.loopLen;
            /* convert amiga period value to hz */
            uint16_t freq = 7159092 / (pat.note_1 * 2);
            debug(DBG_SND, "player_handlePattern() adding sample freq = 0x%X", freq);
            mixer_playChannel(sfx->mixer, channel, &mc, freq, pat.sampleVolume);
        }
    }
}

uint32_t player_eventsCallback(uint32_t interval, void *param) {
    (void) interval;
    debug(DBG_SND, "player_eventsCallback with interval %d ms and param 0x%08x", interval, param);
    struct SfxPlayer *p = (struct SfxPlayer *)param;
    player_handleEvents(p);
    return p->_delay;
}

void player_saveOrLoad(struct SfxPlayer* sfx, struct Serializer *ser) {
    sys_lockMutex(sfx->sys, sfx->_mutex);
    struct Entry entries[] = {
        SE_INT(&sfx->_delay, SES_INT8, VER(2)),
        SE_INT(&sfx->_resNum, SES_INT16, VER(2)),
        SE_INT(&sfx->_sfxMod.curPos, SES_INT16, VER(2)),
        SE_INT(&sfx->_sfxMod.curOrder, SES_INT8, VER(2)),
        SE_END()
    };
    ser_saveOrLoadEntries(ser, entries);
    sys_unlockMutex(sfx->sys, sfx->_mutex);
    if (ser->_mode == SM_LOAD && sfx->_resNum != 0) {
        uint16_t delay = sfx->_delay;
        player_loadSfxModule(sfx, sfx->_resNum, 0, sfx->_sfxMod.curOrder);
        sfx->_delay = delay;
        sfx->_timerId = sys_addTimer(sfx->sys, sfx->_delay, player_eventsCallback, sfx);
    }
}
