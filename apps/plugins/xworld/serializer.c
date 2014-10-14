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

#include "serializer.h"
#include "file.h"


void ser_create(struct Serializer* c, File *stream, Mode mode, uint8_t *ptrBlock, uint16_t saveVer)
{
    c->_stream=stream;
    c->_mode=mode;
    c->_ptrBlock=ptrBlock;
    c->_saveVer=saveVer;
}

void ser_saveOrLoadEntries(struct Serializer* c, Entry *entry) {
    debug(DBG_SER, "ser_saveOrLoadEntries() _mode=%d", _mode);
    c->_bytesCount = 0;
    switch (_mode) {
    case SM_SAVE:
        ser_saveEntries(c, entry);
        break;
    case SM_LOAD:
        ser_loadEntries(c, entry);
        break;
    }
    debug(DBG_SER, "ser_saveOrLoadEntries() _bytesCount=%d", _bytesCount);
}

void ser_saveEntries(struct Serializer* c, Entry *entry) {
    debug(DBG_SER, "ser_saveEntries()");
    for (; entry->type != SET_END; ++entry) {
        if (entry->maxVer == CUR_VER) {
            switch (entry->type) {
            case SET_INT:
                ser_saveInt(c, entry->size, entry->data);
                c->_bytesCount += entry->size;
                break;
            case SET_ARRAY:
                if (entry->size == ser_SES_INT8) {
                    file_write(c->stream, entry->data, entry->n);
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
                c->_stream->writeUint32BE(*(uint8_t **)(entry->data) - c->_ptrBlock);
                c->_bytesCount += 4;
                break;
            case SET_END:
                break;
            }
        }
    }
}

void ser_loadEntries(struct Serializer* c, Entry *entry) {
    debug(DBG_SER, "ser_loadEntries()");
    for (; entry->type != SET_END; ++entry) {
        if (c->_saveVer >= entry->minVer && c->_saveVer <= entry->maxVer) {
            switch (entry->type) {
            case SET_INT:
                ser_loadInt(c, entry->size, entry->data);
                c->_bytesCount += entry->size;
                break;
            case SET_ARRAY:
                if (entry->size == ser_SES_INT8) {
                    file_read(c->stream, entry->data, entry->n);
                    c->_bytesCount += entry->n;
                } else {
                    uint8_t *p = (uint8_t *)entry->data;
                    for (int i = 0; i < entry->n; ++i) {
                        loadInt(entry->size, p);
                        p += entry->size;
                        c->_bytesCount += entry->size;
                    }
                }
                break;
            case SET_PTR:
                *(uint8_t **)(entry->data) = c->_ptrBlock + file_readUint32BE(c->stream);
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
        file_writeByte(c->stream, *(uint8_t *)p);
        break;
    case 2:
        file_writeUint16BE(c->stream, *(uint16_t *)p);
        break;
    case 4:
        file_writeUint32BE(c->stream, *(uint32_t *)p);
        break;
    }
}

void ser_loadInt(struct Serializer* c, uint8_t es, void *p) {
    switch (es) {
    case 1:
        *(uint8_t *)p = file_readByte(c->stream);
        break;
    case 2:
        *(uint16_t *)p = file_readUint16BE(c->stream);
        break;
    case 4:
        *(uint32_t *)p = file_readUint32BE(c->stream);
        break;
    }
}
