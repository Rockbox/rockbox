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

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "intern.h"


#define MEMENTRY_STATE_END_OF_MEMLIST 0xFF
#define MEMENTRY_STATE_NOT_NEEDED 0
#define MEMENTRY_STATE_LOADED 1
#define MEMENTRY_STATE_LOAD_ME 2

/*
  This is a directory entry. When the game starts, it loads memlist.bin and
  populate and array of MemEntry
*/
typedef struct MemEntry {
    uint8_t state;         /* 0x0 */
    uint8_t type;          /* 0x1, Resource::ResType */
    uint8_t *bufPtr;       /* 0x2 */
    uint16_t unk4;         /* 0x4, unused */
    uint8_t rankNum;       /* 0x6 */
    uint8_t bankId;        /* 0x7 */
    uint32_t bankOffset;   /* 0x8 0xA */
    uint16_t unkC;         /* 0xC, unused */
    uint16_t packedSize;   /* 0xE */
    /* All ressources are packed (for a gain of 28% according to Chahi) */

    uint16_t unk10;        /* 0x10, unused */
    uint16_t size;         /* 0x12 */
} __attribute__((packed)) MemEntry;

/*
  Note: state is not a boolean, it can have value 0, 1, 2 or 255, respectively meaning:
  0:NOT_NEEDED
  1:LOADED
  2:LOAD_ME
  255:END_OF_MEMLIST

  See MEMENTRY_STATE_* #defines above.
*/

struct Serializer;
struct Video;

#define MEM_BLOCK_SIZE  (600 * 1024)
#define RT_SOUND 0
#define RT_MUSIC 1
#define RT_POLY_ANIM 2
#define RT_PALETTE 3
#define RT_BYTECODE 4
#define RT_POLY_CINEMATIC 5

struct Resource {
    struct Video *video;
    struct System *sys;
    const char *_dataDir;
    struct MemEntry _memList[150];
    uint16_t _numMemList;
    uint16_t currentPartId, requestedNextPart;
    uint8_t *_memPtrStart, *_scriptBakPtr, *_scriptCurPtr, *_vidBakPtr, *_vidCurPtr;
    bool _useSegVideo2;

    uint8_t *segPalettes;
    uint8_t *segBytecode;
    uint8_t *segCinematic;
    uint8_t *_segVideo2;
};


void res_create(struct Resource*, struct Video*, struct System*, const char* dataDir);

void res_readBank(struct Resource*, const MemEntry *me, uint8_t *dstBuf);
void res_readEntries(struct Resource*);
void res_loadMarkedAsNeeded(struct Resource*);
void res_invalidateAll(struct Resource*);
void res_invalidateRes(struct Resource*);
void res_loadPartsOrMemoryEntry(struct Resource*, uint16_t num);
void res_setupPart(struct Resource*, uint16_t ptrId);
void res_allocMemBlock(struct Resource*);
void res_freeMemBlock(struct Resource*);

void res_saveOrLoad(struct Resource*, struct Serializer *ser);

const char* res_getDataDir(struct Resource*);
#endif
