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
#include "bank.h"
#include "file.h"
#include "resource.h"

void bank_create(struct Bank* b, const char *dataDir)
{
    b->_dataDir = dataDir;
}

bool bank_read(struct Bank* b, const struct MemEntry *me, uint8_t *buf) {

    bool ret = false;
    char bankName[10];
    rb->snprintf(bankName, 10, "bank%02x", me->bankId);
    File f;
    file_create(&f, false);
    if (!file_open(&f, bankName, b->_dataDir, "rb"))
        error("bank_read() unable to open '%s' in dir '%s'", bankName, b->_dataDir);

    file_seek(&f, me->bankOffset);

    /* Depending if the resource is packed or not we */
    /* can read directly or unpack it. */
    if (me->packedSize == me->size) {
        file_read(&f, buf, me->packedSize);
        ret = true;
    } else {
        file_read(&f, buf, me->packedSize);
        b->_startBuf = buf;
        b->_iBuf = buf + me->packedSize - 4;
        ret = bank_unpack(b);
    }
    file_close(&f);
    return ret;
}

void bank_decUnk1(struct Bank* b, uint8_t numChunks, uint8_t addCount) {
    uint16_t count = bank_getCode(b, numChunks) + addCount + 1;
    debug(DBG_BANK, "bank_decUnk1(%d, %d) count=%d", numChunks, addCount, count);
    b->_unpCtx.datasize -= count;
    while (count--) {
        assert(b->_oBuf >= b->_iBuf && b->_oBuf >= b->_startBuf);
        *b->_oBuf = (uint8_t)bank_getCode(b, 8);
        --b->_oBuf;
    }
}

/*
  Note from fab: This look like run-length encoding.
*/
void bank_decUnk2(struct Bank* b, uint8_t numChunks) {
    uint16_t i = bank_getCode(b, numChunks);
    uint16_t count = b->_unpCtx.size + 1;
    debug(DBG_BANK, "bank_decUnk2(%d) i=%d count=%d", numChunks, i, count);
    b->_unpCtx.datasize -= count;
    while (count--) {
        assert(b->_oBuf >= b->_iBuf && b->_oBuf >= b->_startBuf);
        *b->_oBuf = *(b->_oBuf + i);
        --b->_oBuf;
    }
}

/*
  Most resource in the banks are compacted.
*/
bool bank_unpack(struct Bank* b) {
    b->_unpCtx.size = 0;
    b->_unpCtx.datasize = READ_BE_UINT32(b->_iBuf);
    b->_iBuf -= 4;
    b->_oBuf = b->_startBuf + b->_unpCtx.datasize - 1;
    b->_unpCtx.crc = READ_BE_UINT32(b->_iBuf);
    b->_iBuf -= 4;
    b->_unpCtx.chk = READ_BE_UINT32(b->_iBuf);
    b->_iBuf -= 4;
    b->_unpCtx.crc ^= b->_unpCtx.chk;
    do {
        if (!bank_nextChunk(b)) {
            b->_unpCtx.size = 1;
            if (!bank_nextChunk(b)) {
                bank_decUnk1(b, 3, 0);
            } else {
                bank_decUnk2(b, 8);
            }
        } else {
            uint16_t c = bank_getCode(b, 2);
            if (c == 3) {
                bank_decUnk1(b, 8, 8);
            } else {
                if (c < 2) {
                    b->_unpCtx.size = c + 2;
                    bank_decUnk2(b, c + 9);
                } else {
                    b->_unpCtx.size = bank_getCode(b, 8);
                    bank_decUnk2(b, 12);
                }
            }
        }
    } while (b->_unpCtx.datasize > 0);
    return (b->_unpCtx.crc == 0);
}

uint16_t bank_getCode(struct Bank* b, uint8_t numChunks) {
    uint16_t c = 0;
    while (numChunks--) {
        c <<= 1;
        if (bank_nextChunk(b)) {
            c |= 1;
        }
    }
    return c;
}

bool bank_nextChunk(struct Bank* b) {
    bool CF = bank_rcr(b, false);
    if (b->_unpCtx.chk == 0) {
        assert(b->_iBuf >= b->_startBuf);
        b->_unpCtx.chk = READ_BE_UINT32(b->_iBuf);
        b->_iBuf -= 4;
        b->_unpCtx.crc ^= b->_unpCtx.chk;
        CF = bank_rcr(b, true);
    }
    return CF;
}

bool bank_rcr(struct Bank* b, bool CF) {
    bool rCF = (b->_unpCtx.chk & 1);
    b->_unpCtx.chk >>= 1;
    if (CF) b->_unpCtx.chk |= 0x80000000;
    return rCF;
}
