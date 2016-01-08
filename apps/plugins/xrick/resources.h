/*
 * xrick/resources.h
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

#ifndef _RESOURCES_H
#define _RESOURCES_H

#include "xrick/config.h"
#include "xrick/screens.h"
#include "xrick/system/basic_types.h"

/*
 * All data is assumed to be Little Endian
 */
typedef struct
{
    U8 magic[4];
    U8 version[2];
    U8 resourceId[2];
} resource_header_t;

typedef struct 
{
    U8 w;
    U8 h;
    U8 spr[2];
    U8 sni[2];
    U8 trig_w;
    U8 trig_h;
    U8 snd;
} resource_entdata_t;

typedef struct 
{
    U8 x[2];
    U8 y[2];
    U8 row[2];
    U8 submap[2];
    U8 tuneId[2];
} resource_map_t;

typedef struct 
{
    U8 page[2];
    U8 bnum[2];
    U8 connect[2];
    U8 mark[2];
} resource_submap_t;

typedef struct {
    U8 count[2];
    U8 dx[2];
    U8 dy[2];
    U8 base[2];
} resource_imapsteps_t;

typedef struct {
    U8 width[2];
    U8 height[2];
    U8 xPos[2];
    U8 yPos[2];
} resource_pic_t;

typedef struct {
  U8 score[4];
  U8 name[HISCORE_NAME_SIZE];
} resource_hiscore_t;

#ifdef GFXPC
typedef struct {
    U8 mask[2];
    U8 pict[2];
} resource_spriteX_t;
#endif /* GFXPC */

extern const U8 resource_magic[4];

enum
{
    DATA_VERSION = 3,

    /* "bootstrap" file */
    Resource_FILELIST = 0,

    /* graphics, misc, texts */
    Resource_PALETTE,
    Resource_ENTDATA,
    Resource_SPRSEQ,
    Resource_MVSTEP,
    Resource_MAPS,
    Resource_SUBMAPS,
    Resource_CONNECT,
    Resource_BNUMS,
    Resource_BLOCKS,
    Resource_MARKS,
    Resource_EFLGC,
    Resource_IMAPSL,
    Resource_IMAPSTEPS,
    Resource_IMAPSOFS,
    Resource_IMAPTEXT,
    Resource_GAMEOVERTXT,
    Resource_PAUSEDTXT,
    Resource_SPRITESDATA,
    Resource_TILESDATA,
    Resource_HIGHSCORES,
    Resource_IMGSPLASH,
    Resource_PICHAF,  /* ST version only */
    Resource_PICCONGRATS,  /* ST version only */
    Resource_PICSPLASH,  /* ST version only */
    Resource_IMAINHOFT, /* PC version only */
    Resource_IMAINRDT, /* PC version only */
    Resource_IMAINCDC, /* PC version only */
    Resource_SCREENCONGRATS, /* PC version only */

    /* sounds */
    Resource_SOUNDBOMBSHHT,
    Resource_SOUNDBONUS,
    Resource_SOUNDBOX,
    Resource_SOUNDBULLET,
    Resource_SOUNDCRAWL,
    Resource_SOUNDDIE,
    Resource_SOUNDENTITY0,
    Resource_SOUNDENTITY1,
    Resource_SOUNDENTITY2,
    Resource_SOUNDENTITY3,
    Resource_SOUNDENTITY4,
    Resource_SOUNDENTITY5,
    Resource_SOUNDENTITY6,
    Resource_SOUNDENTITY7,
    Resource_SOUNDENTITY8,
    Resource_SOUNDEXPLODE,
    Resource_SOUNDGAMEOVER,
    Resource_SOUNDJUMP,
    Resource_SOUNDPAD,
    Resource_SOUNDSBONUS1,
    Resource_SOUNDSBONUS2,
    Resource_SOUNDSTICK,
    Resource_SOUNDTUNE0,
    Resource_SOUNDTUNE1,
    Resource_SOUNDTUNE2,
    Resource_SOUNDTUNE3,
    Resource_SOUNDTUNE4,
    Resource_SOUNDTUNE5,
    Resource_SOUNDWALK,

    Resource_MAX_COUNT,
};

#define BOOTSTRAP_RESOURCE_NAME "filelist.dat"

bool resources_load(void);
void resources_unload(void);

#endif /* ndef _RESOURCES_H */

/* eof */
