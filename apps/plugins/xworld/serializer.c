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


/*ser_Serializer(struct Serializer* c, File *stream, Mode mode, uint8_t *ptrBlock, uint16_t saveVer)
	: _stream(stream), _mode(mode), _ptrBlock(ptrBlock), _saveVer(saveVer) {
}*/

void ser_saveOrLoadEntries(struct Serializer* c, Entry *entry) {
	debug(DBG_SER, "ser_saveOrLoadEntries() _mode=%d", _mode);
	c->_bytesCount = 0;
	switch (_mode) {
	case SM_SAVE:
		saveEntries(entry);
		break;
	case SM_LOAD:
		loadEntries(entry);
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
				saveInt(entry->size, entry->data);
				c->_bytesCount += entry->size;
				break;
			case SET_ARRAY:
				if (entry->size == ser_SES_INT8) {
					c->_stream->write(entry->data, entry->n);
					c->_bytesCount += entry->n;
				} else {
					uint8_t *p = (uint8_t *)entry->data;
					for (int i = 0; i < entry->n; ++i) {
						saveInt(entry->size, p);
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
				loadInt(entry->size, entry->data);
				c->_bytesCount += entry->size;
				break;
			case SET_ARRAY:
				if (entry->size == ser_SES_INT8) {
					c->_stream->read(entry->data, entry->n);
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
				*(uint8_t **)(entry->data) = c->_ptrBlock + c->_stream->readUint32BE();
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
		c->_stream->writeByte(*(uint8_t *)p);
		break;
	case 2:
		c->_stream->writeUint16BE(*(uint16_t *)p);
		break;
	case 4:
		c->_stream->writeUint32BE(*(uint32_t *)p);
		break;
	}
}

void ser_loadInt(struct Serializer* c, uint8_t es, void *p) {
	switch (es) {
	case 1:
		*(uint8_t *)p = c->_stream->readByte();
		break;
	case 2:
		*(uint16_t *)p = c->_stream->readUint16BE();
		break;
	case 4:
		*(uint32_t *)p = c->_stream->readUint32BE();
		break;
	}
}
