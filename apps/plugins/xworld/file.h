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

#ifndef __FILE_H__
#define __FILE_H__

#include "intern.h"

struct File_impl;

struct File {
    int fd;
};

void file_create(struct File*, bool gzipped);

bool file_openc(struct File*, onst char *filename, const char *directory, const char *mode="rb");
void file_close(struct File*);
bool file_ioErr(struct File*);
void file_seek(struct File*, int32_t off);
void file_read(struct File*, void *ptr, uint32_t size);
uint8_t file_readByte(struct File*);
uint16_t file_readUint16BE(struct File*);
uint32_t file_readUint32BE(struct File*);
void file_write(struct File*, void *ptr, uint32_t size);
void file_writeByte(struct File*, uint8_t b);
void file_writeUint16BE(struct File*, uint16_t n);
void file_writeUint32BE(struct File*, uint32_t n);

#endif
