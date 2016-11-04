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

#include "plugin.h"
#include "resource.h"
#include "bank.h"
#include "file.h"
#include "serializer.h"
#include "video.h"
#include "util.h"
#include "parts.h"
#include "vm.h"
#include "sys.h"

void res_create(struct Resource* res, struct Video* vid, struct System* sys, const char* dataDir)
{
    res->video = vid;
    res->sys = sys;
    res->_dataDir = dataDir;
    res->currentPartId = 0;
    res->requestedNextPart = 0;
}

void res_readBank(struct Resource* res, const MemEntry *me, uint8_t *dstBuf) {
    uint16_t n = me - res->_memList;
    debug(DBG_BANK, "res_readBank(%d)", n);

    struct Bank bk;
    bank_create(&bk, res->_dataDir);
    if (!bank_read(&bk, me, dstBuf)) {
        error("res_readBank() unable to unpack entry %d\n", n);
    }
}

#ifdef XWORLD_DEBUG
static const char *resTypeToString(struct Resource* res, unsigned int type)
{
    (void) res;
    static const char* resTypes[] =
    {
        "RT_SOUND",
        "RT_MUSIC",
        "RT_POLY_ANIM",
        "RT_PALETTE",
        "RT_BYTECODE",
        "RT_POLY_CINEMATIC"
    };
    if (type >= (sizeof(resTypes) / sizeof(const char *)))
        return "RT_UNKNOWN";
    return resTypes[type];
}
#endif

#define RES_SIZE 0
#define RES_COMPRESSED 1
int resourceSizeStats[7][2];
#define STATS_TOTAL_SIZE 6
int resourceUnitStats[7][2];

/*
  Read all entries from memlist.bin. Do not load anything in memory,
  this is just a fast way to access the data later based on their id.
*/
void res_readEntries(struct Resource* res) {
    File f;
    file_create(&f, false);

    int resourceCounter = 0;

    if (!file_open(&f, "memlist.bin", res->_dataDir, "rb")) {
        error("Could not open 'MEMLIST.BIN', data files missing");
        /* error() will exit() no need to return or do anything else. */
    }

    /* Prepare stats array */
    rb->memset(resourceSizeStats, 0, sizeof(resourceSizeStats));
    rb->memset(resourceUnitStats, 0, sizeof(resourceUnitStats));

    res->_numMemList = 0;
    struct MemEntry *memEntry = res->_memList;
    while (1) {
        assert(res->_numMemList < ARRAYLEN(res->_memList));
        memEntry->state = file_readByte(&f);
        memEntry->type = file_readByte(&f);
        memEntry->bufPtr = 0;
        file_readUint16BE(&f);
        memEntry->unk4 = file_readUint16BE(&f);
        memEntry->rankNum = file_readByte(&f);
        memEntry->bankId = file_readByte(&f);
        memEntry->bankOffset = file_readUint32BE(&f);
        memEntry->unkC = file_readUint16BE(&f);
        memEntry->packedSize = file_readUint16BE(&f);
        memEntry->unk10 = file_readUint16BE(&f);
        memEntry->size = file_readUint16BE(&f);

        debug(DBG_RES, "mementry state is %d", memEntry->state);
        if (memEntry->state == MEMENTRY_STATE_END_OF_MEMLIST) {
            break;
        }

        /* Memory tracking */
        if (memEntry->packedSize == memEntry->size)
        {
            resourceUnitStats[memEntry->type][RES_SIZE] ++;
            resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE] ++;
        }
        else
        {
            resourceUnitStats[memEntry->type][RES_COMPRESSED] ++;
            resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED] ++;
        }

        resourceSizeStats[memEntry->type][RES_SIZE] += memEntry->size;
        resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] += memEntry->size;
        resourceSizeStats[memEntry->type][RES_COMPRESSED] += memEntry->packedSize;
        resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED] += memEntry->packedSize;

        debug(DBG_RES, "R:0x%02X, %-17s size=%5d (compacted gain=%2.0f%%)",
              resourceCounter,
              resTypeToString(res, memEntry->type),
              memEntry->size,
              memEntry->size ? (memEntry->size - memEntry->packedSize) / (float)memEntry->size * 100.0f : 0.0f);

        resourceCounter++;

        res->_numMemList++;
        memEntry++;
    }

    debug(DBG_RES, "\n");
    debug(DBG_RES, "Total # resources: %d", resourceCounter);
    debug(DBG_RES, "Compressed       : %d", resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
    debug(DBG_RES, "Uncompressed     : %d", resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE]);
    debug(DBG_RES, "Note: %2.0f%% of resources are compressed.", 100 * resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED] / (float)resourceCounter);
    debug(DBG_RES, "\n");
    debug(DBG_RES, "Total size (uncompressed) : %7d bytes.", resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE]);
    debug(DBG_RES, "Total size (compressed)   : %7d bytes.", resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
    debug(DBG_RES, "Note: Overall compression gain is : %2.0f%%.",
          (resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] - resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED]) / (float)resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] * 100);

    debug(DBG_RES, "\n");
    for(int i = 0 ; i < 6 ; i++)
        debug(DBG_RES, "Total %-17s unpacked size: %7d (%2.0f%% of total unpacked size) packedSize %7d (%2.0f%% of floppy space) gain:(%2.0f%%)",
              resTypeToString(res, i),
              resourceSizeStats[i][RES_SIZE],
              resourceSizeStats[i][RES_SIZE] / (float)resourceSizeStats[STATS_TOTAL_SIZE][RES_SIZE] * 100.0f,
              resourceSizeStats[i][RES_COMPRESSED],
              resourceSizeStats[i][RES_COMPRESSED] / (float)resourceSizeStats[STATS_TOTAL_SIZE][RES_COMPRESSED] * 100.0f,
              (resourceSizeStats[i][RES_SIZE] - resourceSizeStats[i][RES_COMPRESSED]) / (float)resourceSizeStats[i][RES_SIZE] * 100.0f);

    debug(DBG_RES, "Note: Damn you sound compression rate!");

    debug(DBG_RES, "\nTotal bank files:              %d", resourceUnitStats[STATS_TOTAL_SIZE][RES_SIZE] + resourceUnitStats[STATS_TOTAL_SIZE][RES_COMPRESSED]);
    for(int i = 0 ; i < 6 ; i++)
        debug(DBG_RES, "Total %-17s files: %3d", resTypeToString(res, i), resourceUnitStats[i][RES_SIZE] + resourceUnitStats[i][RES_COMPRESSED]);

    file_close(&f);
}

/*
  Go over every resource and check if they are marked at "MEMENTRY_STATE_LOAD_ME".
  Load them in memory and mark them are MEMENTRY_STATE_LOADED
*/
void res_loadMarkedAsNeeded(struct Resource* res) {

    while (1) {
        struct MemEntry *me = NULL;

        /* get resource with max rankNum */
        uint8_t maxNum = 0;
        uint16_t i = res->_numMemList;
        struct MemEntry *it = res->_memList;
        while (i--) {
            if (it->state == MEMENTRY_STATE_LOAD_ME && maxNum <= it->rankNum) {
                maxNum = it->rankNum;
                me = it;
            }
            it++;
        }

        if (me == NULL) {
            break; // no entry found
        }


        /* At this point the resource descriptor should be pointed to "me" */

        uint8_t *loadDestination = NULL;
        if (me->type == RT_POLY_ANIM) {
            loadDestination = res->_vidCurPtr;
        } else {
            loadDestination = res->_scriptCurPtr;
            if (me->size > res->_vidBakPtr - res->_scriptCurPtr) {
                warning("res_load() not enough memory");
                me->state = MEMENTRY_STATE_NOT_NEEDED;
                continue;
            }
        }


        if (me->bankId == 0) {
            warning("res_load() ec=0x%X (me->bankId == 0)", 0xF00);
            me->state = MEMENTRY_STATE_NOT_NEEDED;
        } else {
            debug(DBG_BANK, "res_load() bufPos=%X size=%X type=%X pos=%X bankId=%X", loadDestination - res->_memPtrStart, me->packedSize, me->type, me->bankOffset, me->bankId);
            res_readBank(res, me, loadDestination);
            if(me->type == RT_POLY_ANIM) {
                video_copyPagePtr(res->video, res->_vidCurPtr);
                me->state = MEMENTRY_STATE_NOT_NEEDED;
            } else {
                me->bufPtr = loadDestination;
                me->state = MEMENTRY_STATE_LOADED;
                res->_scriptCurPtr += me->size;
            }
        }
    }
}

void res_invalidateRes(struct Resource* res) {
    struct MemEntry *me = res->_memList;
    uint16_t i = res->_numMemList;
    while (i--) {
        if (me->type <= RT_POLY_ANIM || me->type > 6) {  /* 6 WTF ?!?! ResType goes up to 5 !! */
            me->state = MEMENTRY_STATE_NOT_NEEDED;
        }
        ++me;
    }
    res->_scriptCurPtr = res->_scriptBakPtr;
}

void res_invalidateAll(struct Resource* res) {
    struct MemEntry *me = res->_memList;
    uint16_t i = res->_numMemList;
    while (i--) {
        me->state = MEMENTRY_STATE_NOT_NEEDED;
        ++me;
    }
    res->_scriptCurPtr = res->_memPtrStart;
}

/* This method serves two purpose:
   - Load parts in memory segments (palette,code,video1,video2)
   or
   - Load a resource in memory

   This is decided based on the resourceId. If it does not match a mementry id it is supposed to
   be a part id. */
void res_loadPartsOrMemoryEntry(struct Resource* res, uint16_t resourceId) {

    if (resourceId > res->_numMemList) {

        res->requestedNextPart = resourceId;

    } else {

        struct MemEntry *me = &res->_memList[resourceId];

        if (me->state == MEMENTRY_STATE_NOT_NEEDED) {
            me->state = MEMENTRY_STATE_LOAD_ME;
            res_loadMarkedAsNeeded(res);
        }
    }

}

/* Protection screen and cinematic don't need the player and enemies polygon data
   so _memList[video2Index] is never loaded for those parts of the game. When
   needed (for action phrases) _memList[video2Index] is always loaded with 0x11
   (as seen in memListParts). */
void res_setupPart(struct Resource* res, uint16_t partId) {

    if (partId == res->currentPartId)
        return;

    if (partId < GAME_PART_FIRST || partId > GAME_PART_LAST)
        error("res_setupPart() ec=0x%X invalid partId", partId);

    uint16_t memListPartIndex = partId - GAME_PART_FIRST;

    uint8_t paletteIndex = memListParts[memListPartIndex][MEMLIST_PART_PALETTE];
    uint8_t codeIndex    = memListParts[memListPartIndex][MEMLIST_PART_CODE];
    uint8_t videoCinematicIndex  = memListParts[memListPartIndex][MEMLIST_PART_POLY_CINEMATIC];
    uint8_t video2Index  = memListParts[memListPartIndex][MEMLIST_PART_VIDEO2];

    /* Mark all resources as located on harddrive. */
    res_invalidateAll(res);

    res->_memList[paletteIndex].state = MEMENTRY_STATE_LOAD_ME;
    res->_memList[codeIndex].state = MEMENTRY_STATE_LOAD_ME;
    res->_memList[videoCinematicIndex].state = MEMENTRY_STATE_LOAD_ME;

    /* This is probably a cinematic or a non interactive part of the game. */
    /* Player and enemy polygons are not needed. */
    if (video2Index != MEMLIST_PART_NONE)
        res->_memList[video2Index].state = MEMENTRY_STATE_LOAD_ME;


    res_loadMarkedAsNeeded(res);

    res->segPalettes = res->_memList[paletteIndex].bufPtr;
    debug(DBG_RES, "paletteIndex is 0x%08x", res->segPalettes);
    res->segBytecode     = res->_memList[codeIndex].bufPtr;
    res->segCinematic   = res->_memList[videoCinematicIndex].bufPtr;



    /* This is probably a cinematic or a non interactive part of the game. */
    /* Player and enemy polygons are not needed. */
    if (video2Index != MEMLIST_PART_NONE)
        res->_segVideo2 = res->_memList[video2Index].bufPtr;

    debug(DBG_RES, "");
    debug(DBG_RES, "setupPart(%d)", partId - GAME_PART_FIRST);
    debug(DBG_RES, "Loaded resource %d (%s) in segPalettes.", paletteIndex, resTypeToString(res, res->_memList[paletteIndex].type));
    debug(DBG_RES, "Loaded resource %d (%s) in segBytecode.", codeIndex, resTypeToString(res, res->_memList[codeIndex].type));
    debug(DBG_RES, "Loaded resource %d (%s) in segCinematic.", videoCinematicIndex, resTypeToString(res, res->_memList[videoCinematicIndex].type));


    /* prevent warnings: */
#ifdef XWORLD_DEBUG
    if (video2Index != MEMLIST_PART_NONE)
        debug(DBG_RES, "Loaded resource %d (%s) in _segVideo2.", video2Index, resTypeToString(res, res->_memList[video2Index].type));
#endif


    res->currentPartId = partId;


    /* _scriptCurPtr is changed in res->load(); */
    res->_scriptBakPtr = res->_scriptCurPtr;
}

void res_allocMemBlock(struct Resource* res) {
    if(rb->audio_status())
        rb->audio_stop();
    /* steal the audio buffer */
    size_t sz;
    /* memory usage is first statically allocated, then the remainder is used dynamically:
     * static:
     *  [VM memory - 600K]
     *  [Framebuffers - 128K]
     *  [Temporary framebuffer - 192K]
     * dynamic:
     *  [String table buffer]
     */
    res->_memPtrStart = rb->plugin_get_audio_buffer(&sz);
    if(sz < MEM_BLOCK_SIZE + (4 * VID_PAGE_SIZE) + 320 * 200 * sizeof(fb_data))
    {
        warning("res_allocMemBlock: can't allocate enough memory!");
    }

    res->sys->membuf = res->_memPtrStart + ( MEM_BLOCK_SIZE + (4 * VID_PAGE_SIZE) + 320 * 200 * sizeof(fb_data));
    res->sys->bytes_left = sz - (MEM_BLOCK_SIZE + (4 * VID_PAGE_SIZE) + 320 * 200 * sizeof(fb_data));

    debug(DBG_RES, "audiobuf is %d bytes in size", sz);

    res->_scriptBakPtr = res->_scriptCurPtr = res->_memPtrStart;
    res->_vidBakPtr = res->_vidCurPtr = res->_memPtrStart + MEM_BLOCK_SIZE - 0x800 * 16; //0x800 = 2048, so we have 32KB free for vidBack and vidCur
    res->_useSegVideo2 = false;
}

void res_freeMemBlock(struct Resource* res) {
    (void) res;
    /* there's no need to do anything to free the audio buffer */
    return;
}

void res_saveOrLoad(struct Resource* res, struct Serializer *ser) {
    uint8_t loadedList[64];
    if (ser->_mode == SM_SAVE) {
        rb->memset(loadedList, 0, sizeof(loadedList));
        uint8_t *p = loadedList;
        uint8_t *q = res->_memPtrStart;
        while (1) {
            struct MemEntry *it = res->_memList;
            struct MemEntry *me = 0;
            uint16_t num = res->_numMemList;
            while (num--) {
                if (it->state == MEMENTRY_STATE_LOADED && it->bufPtr == q) {
                    me = it;
                }
                ++it;
            }
            if (me == 0) {
                break;
            } else {
                assert(p < loadedList + 64);
                *p++ = me - res->_memList;
                q += me->size;
            }
        }
    }

    struct Entry entries[] = {
        SE_ARRAY(loadedList, 64, SES_INT8, VER(1)),
        SE_INT(&res->currentPartId, SES_INT16, VER(1)),
        SE_PTR(&res->_scriptBakPtr, VER(1)),
        SE_PTR(&res->_scriptCurPtr, VER(1)),
        SE_PTR(&res->_vidBakPtr, VER(1)),
        SE_PTR(&res->_vidCurPtr, VER(1)),
        SE_INT(&res->_useSegVideo2, SES_BOOL, VER(1)),
        SE_PTR(&res->segPalettes, VER(1)),
        SE_PTR(&res->segBytecode, VER(1)),
        SE_PTR(&res->segCinematic, VER(1)),
        SE_PTR(&res->_segVideo2, VER(1)),
        SE_END()
    };

    ser_saveOrLoadEntries(ser, entries);
    if (ser->_mode == SM_LOAD) {
        uint8_t *p = loadedList;
        uint8_t *q = res->_memPtrStart;
        while (*p) {
            struct MemEntry *me = &res->_memList[*p++];
            res_readBank(res, me, q);
            me->bufPtr = q;
            me->state = MEMENTRY_STATE_LOADED;
            q += me->size;
        }
    }
}

const char* res_getDataDir(struct Resource* res)
{
    return res->_dataDir;
}
