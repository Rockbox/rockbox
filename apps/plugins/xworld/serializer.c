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

#include "serializer.h"
#include "file.h"


void ser_create(struct Serializer* c, File *stream, enum Mode mode, uint8_t *ptrBlock, uint16_t saveVer)
{
    c->_stream = stream;
    c->_mode = mode;
    c->_ptrBlock = ptrBlock;
    c->_saveVer = saveVer;
}

void ser_saveOrLoadEntries(struct Serializer* c, struct Entry *entry) {
    debug(DBG_SER, "ser_saveOrLoadEntries() _mode=%d", c->_mode);
    c->_bytesCount = 0;
    switch (c->_mode) {
    case SM_SAVE:
        ser_saveEntries(c, entry);
        break;
    case SM_LOAD:
        ser_loadEntries(c, entry);
        break;
    }
    debug(DBG_SER, "ser_saveOrLoadEntries() _bytesCount=%d", c->_bytesCount);
}

void ser_saveEntries(struct Serializer* c, struct Entry *entry) {
    debug(DBG_SER, "ser_saveEntries()");
    for (; entry->type != SET_END; ++entry) {
        if (entry->maxVer == CUR_VER) {
            switch (entry->type) {
            case SET_INT:
                ser_saveInt(c, entry->size, entry->data);
                c->_bytesCount += entry->size;
                break;
            case SET_ARRAY:
                if (entry->size == SES_INT8) {
                    file_write(c->_stream, entry->data, entry->n);
                    c->_bytesCount += entry->n;
                } else {
                    uint8_t *p = (uint8_t *)entry->data;
                    for (int i = 0; i < entry->n; ++i) {
                        ser_saveInt(c, entry->size, p);
                        p += entry->size;
                        c->_bytesCount += entry->size;
                    }
                }
                break;
            case SET_PTR:
                file_writeUint32BE(c->_stream, *(uint8_t **)(entry->data) - c->_ptrBlock);
                c->_bytesCount += 4;
                break;
            case SET_END:
                break;
            }
        }
    }
}

void ser_loadEntries(struct Serializer* c, struct Entry *entry) {
    debug(DBG_SER, "ser_loadEntries()");
    for (; entry->type != SET_END; ++entry) {
        if (c->_saveVer >= entry->minVer && c->_saveVer <= entry->maxVer) {
            switch (entry->type) {
            case SET_INT:
                ser_loadInt(c, entry->size, entry->data);
                c->_bytesCount += entry->size;
                break;
            case SET_ARRAY:
                if (entry->size == SES_INT8) {
                    file_read(c->_stream, entry->data, entry->n);
                    c->_bytesCount += entry->n;
                } else {
                    uint8_t *p = (uint8_t *)entry->data;
                    for (int i = 0; i < entry->n; ++i) {
                        ser_loadInt(c, entry->size, p);
                        p += entry->size;
                        c->_bytesCount += entry->size;
                    }
                }
                break;
            case SET_PTR:
                *(uint8_t **)(entry->data) = c->_ptrBlock + file_readUint32BE(c->_stream);
                c->_bytesCount += 4;
                break;
            case SET_END:
                break;
            }
        }
    }
}

void ser_saveInt(struct Serializer* c, uint8_t es, void *p) {
    switch (es) {
    case 1:
        file_writeByte(c->_stream, *(uint8_t *)p);
        break;
    case 2:
        file_writeUint16BE(c->_stream, *(uint16_t *)p);
        break;
    case 4:
        file_writeUint32BE(c->_stream, *(uint32_t *)p);
        break;
    }
}

void ser_loadInt(struct Serializer* c, uint8_t es, void *p) {
    switch (es) {
    case 1:
        *(uint8_t *)p = file_readByte(c->_stream);
        break;
    case 2:
        *(uint16_t *)p = file_readUint16BE(c->_stream);
        break;
    case 4:
        *(uint32_t *)p = file_readUint32BE(c->_stream);
        break;
    }
}
