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

#ifndef __BANK_H__
#define __BANK_H__

#include "intern.h"

struct MemEntry;

struct UnpackContext {
    uint16_t size;
    uint32_t crc;
    uint32_t chk;
    int32_t datasize;
};

struct Bank
{
    struct UnpackContext _unpCtx;
    const char *_dataDir;
    uint8_t *_iBuf, *_oBuf, *_startBuf;
};

/* needs allocated memory */
void bank_create(struct Bank*, const char *dataDir);

bool bank_read(struct Bank*, const struct MemEntry *me, uint8_t *buf);
void bank_decUnk1(struct Bank*, uint8_t numChunks, uint8_t addCount);
void bank_decUnk2(struct Bank*, uint8_t numChunks);
bool bank_unpack(struct Bank*);
uint16_t bank_getCode(struct Bank*, uint8_t numChunks);
bool bank_nextChunk(struct Bank*);
bool bank_rcr(struct Bank*, bool CF);

#endif
