/*
 * xrick/data/sounds.h
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza. All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#ifndef _SOUNDS_H
#define _SOUNDS_H

#include "xrick/config.h"

#ifdef ENABLE_SOUND

#include "xrick/system/basic_types.h"

typedef struct {
  char *name;
  U8 *buf;
  U32 len;
  bool dispose;
} sound_t;

enum
{
    /* expected format is 8-bit mono at 22050Hz */
    Wave_SAMPLE_RATE = 22050,
    Wave_AUDIO_FORMAT = 1, /* PCM = 1 (i.e. Linear quantization) */
    Wave_CHANNEL_COUNT = 1,
    Wave_BITS_PER_SAMPLE = 8,
};

typedef struct {
    /* "RIFF" chunk descriptor */
    U8 riffChunkId[4];
    U8 riffChunkSize[4];
    U8 riffType[4];
    /* "fmt" sub-chunk */
    U8 formatChunkId[4];
    U8 formatChunkSize[4];
    U8 audioFormat[2];
    U8 channelCount[2];
    U8 sampleRate[4];
    U8 byteRate[4];
    U8 blockAlign[2];
    U8 bitsPerSample[2];
    /* "data" sub-chunk */
    U8 dataChunkId[4];
    U8 dataChunkSize[4];
} wave_header_t;

/* apparently there are 10 entity sounds in original game (ref. "e_them.c" notes)? However we only have 9 so far... */
enum { SOUNDS_NBR_ENTITIES = 10 };

extern sound_t *soundBombshht;
extern sound_t *soundBonus;
extern sound_t *soundBox;
extern sound_t *soundBullet;
extern sound_t *soundCrawl;
extern sound_t *soundDie;
extern sound_t *soundEntity[SOUNDS_NBR_ENTITIES];
extern sound_t *soundExplode;
extern sound_t *soundGameover;
extern sound_t *soundJump;
extern sound_t *soundPad;
extern sound_t *soundSbonus1;
extern sound_t *soundSbonus2;
extern sound_t *soundStick;
extern sound_t *soundTune0;
extern sound_t *soundTune1;
extern sound_t *soundTune2;
extern sound_t *soundTune3;
extern sound_t *soundTune4;
extern sound_t *soundTune5;
extern sound_t *soundWalk;

#endif /* ENABLE_SOUND */

#endif /* ndef _SOUNDS_H */

/* eof */