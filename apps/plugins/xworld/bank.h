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

/* NEEDS MEMORY ALREADY ALLOCATED!!! */
void bank_create(struct Bank*, const char *dataDir);

bool bank_read(struct Bank*, const MemEntry *me, uint8_t *buf);
void bank_decUnk1(struct Bank*, uint8_t numChunks, uint8_t addCount);
void bank_decUnk2(struct Bank*, uint8_t numChunks);
bool bank_unpack(struct Bank*);
uint16_t bank_getCode(struct Bank*, uint8_t numChunks);
bool bank_nextChunk(struct Bank*);
bool bank_rcr(struct Bank*, bool CF);

#endif
