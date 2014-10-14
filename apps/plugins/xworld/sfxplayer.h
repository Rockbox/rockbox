/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __SFXPLAYER_H__
#define __SFXPLAYER_H__

#include "intern.h"

struct SfxInstrument {
    uint8_t *data;
    uint16_t volume;
};

struct SfxModule {
    const uint8_t *data;
    uint16_t curPos;
    uint8_t curOrder;
    uint8_t numOrder;
    uint8_t orderTable[0x80];
    struct SfxInstrument samples[15];
};

struct SfxPattern {
    uint16_t note_1;
    uint16_t note_2;
    uint16_t sampleStart;
    uint8_t *sampleBuffer;
    uint16_t sampleLen;
    uint16_t loopPos;
    uint8_t *loopData;
    uint16_t loopLen;
    uint16_t sampleVolume;
};

struct Mixer;
struct Resource;
struct Serializer;
struct System;

struct SfxPlayer {
    struct Mixer *mixer;
    struct Resource *res;
    struct System *sys;

    void *_mutex;
    void *_timerId;
    uint16_t _delay;
    uint16_t _resNum;
    struct SfxModule _sfxMod;
    int16_t *_markVar;
};

void player_create(struct SfxPlayer*, struct Mixer *mix, struct Resource *res, struct System *stub);
void player_init(struct SfxPlayer*);
void player_free(struct SfxPlayer*);

void player_setEventsDelay(struct SfxPlayer*, uint16_t delay);
void player_loadSfxModule(struct SfxPlayer*, uint16_t resNum, uint16_t delay, uint8_t pos);
void player_prepareInstruments(struct SfxPlayer*, const uint8_t *p);
void player_start(struct SfxPlayer*);
void player_stop(struct SfxPlayer*);
void player_handleEvents(struct SfxPlayer*);
void player_handlePattern(struct SfxPlayer*, uint8_t channel, const uint8_t *patternData);

uint32_t player_eventsCallback(uint32_t interval, void *param);

void player_saveOrLoad(struct SfxPlayer*, struct Serializer *ser);

#endif
