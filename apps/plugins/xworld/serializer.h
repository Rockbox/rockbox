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

#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include "intern.h"

#define CUR_VER 2

#define VER(x) x

enum EntryType {
    SET_INT,
    SET_ARRAY,
    SET_PTR,
    SET_END
};

#define SE_INT(i,sz,ver)     { SET_INT, sz, 1, i, ver, CUR_VER }
#define SE_ARRAY(a,n,sz,ver) { SET_ARRAY, sz, n, a, ver, CUR_VER }
#define SE_PTR(p,ver)        { SET_PTR, 0, 0, p, ver, CUR_VER }
#define SE_END()             { SET_END, 0, 0, 0, 0, 0 }

struct File;

enum {
    SES_BOOL  = 1,
    SES_INT8  = 1,
    SES_INT16 = 2,
    SES_INT32 = 4
};

enum Mode {
    SM_SAVE,
    SM_LOAD
};

struct Entry {
    enum EntryType type;
    uint8_t size;
    uint16_t n;
    void *data;
    uint16_t minVer;
    uint16_t maxVer;
};

struct Serializer {
    File *_stream;
    enum Mode _mode;
    uint8_t *_ptrBlock;
    uint16_t _saveVer;
    uint32_t _bytesCount;
};

void ser_create(struct Serializer*, File *stream, enum Mode mode, uint8_t *ptrBlock, uint16_t saveVer);

void ser_saveOrLoadEntries(struct Serializer*, struct Entry *entry);

void ser_saveEntries(struct Serializer*, struct Entry *entry);
void ser_loadEntries(struct Serializer*, struct Entry *entry);

void ser_saveInt(struct Serializer*, uint8_t es, void *p);
void ser_loadInt(struct Serializer*, uint8_t es, void *p);
#endif
