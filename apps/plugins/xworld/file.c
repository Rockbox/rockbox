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

#include "plugin.h"
#include "file.h"
//#include "miniz.c"
/*
  struct File_impl {
  bool _ioErr;
  File_impl() : _ioErr(false) {}
  virtual bool open(const char *path, const char *mode) = 0;
  virtual void close() = 0;
  virtual void seek(int32_t off) = 0;
  virtual void read(void *ptr, uint32_t size) = 0;
  virtual void write(void *ptr, uint32_t size) = 0;
  };

  struct stdFile : File_impl {
  FILE *_fp;
  stdFile() : _fp(0) {}
  bool open(const char *path, const char *mode) {
  _ioErr = false;
  _fp = fopen(path, mode);
  return (_fp != NULL);
  }
  void close() {
  if (_fp) {
  fclose(_fp);
  _fp = 0;
  }
  }
  void seek(int32_t off) {
  if (_fp) {
  fseek(_fp, off, SEEK_SET);
  }
  }
  void read(void *ptr, uint32_t size) {
  if (_fp) {
  uint32_t r = fread(ptr, 1, size, _fp);
  if (r != size) {
  _ioErr = true;
  }
  }
  }
  void write(void *ptr, uint32_t size) {
  if (_fp) {
  uint32_t r = fwrite(ptr, 1, size, _fp);
  if (r != size) {
  _ioErr = true;
  }
  }
  }
  };

  struct zlibFile : File_impl {
  gzFile _fp;
  zlibFile() : _fp(0) {}
  bool open(const char *path, const char *mode) {
  _ioErr = false;
  _fp = gzopen(path, mode);
  return (_fp != NULL);
  }
  void close() {
  if (_fp) {
  gzclose(_fp);
  _fp = 0;
  }
  }
  void seek(int32_t off) {
  if (_fp) {
  gzseek(_fp, off, SEEK_SET);
  }
  }
  void read(void *ptr, uint32_t size) {
  if (_fp) {
  uint32_t r = gzread(_fp, ptr, size);
  if (r != size) {
  _ioErr = true;
  }
  }
  }
  void write(void *ptr, uint32_t size) {
  if (_fp) {
  uint32_t r = gzwrite(_fp, ptr, size);
  if (r != size) {
  _ioErr = true;
  }
  }
  }
  };
*/

void file_create(struct File* f, bool gzipped) {
    f->gzipped=gzipped;
    f->fd=-1;
    f->ioErr=false;
}

bool file_open(struct File* f, const char *filename, const char *directory, const char *mode) {
    char buf[512];
    rb->snprintf(buf, 512, "%s/%s", directory, filename);
    char *p = buf + rb->strlen(directory)+1;
    string_lower(p);

    int flags=0;
    for(int i=0;mode[i];++i)
    {
        switch(mode[i])
        {
        case 'w':
            flags|=O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case 'r':
            flags|=O_RDONLY;
            break;
        default:
            break;
        }
    }
    f->fd=-1;
    debug(DBG_FILE, "trying %s first", buf);
    f->fd = rb->open(buf, flags, 0666);
    if (f->fd < 0) { // let's try uppercase
        string_upper(p);
        debug(DBG_FILE, "now trying %s uppercase", buf);
        f->fd = rb->open(buf, flags, 0666);
    }
    if(f->fd > 0)
        return true;
    else
        return false;
}

void file_close(struct File* f) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->close(f->fd);
    }
}

bool file_ioErr(struct File* f) {
    return f->ioErr;
}

void file_seek(struct File* f, int32_t off) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->lseek(f->fd, off, SEEK_SET);
    }
}
void file_read(struct File* f, void *ptr, uint32_t size) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->read(f->fd, ptr, size);
    }
}
uint8_t file_readByte(struct File* f) {
    uint8_t b;
    if(f->gzipped)
    {
    }
    else
    {
        if(rb->read(f->fd, &b, 1)!=1)
        {
            f->ioErr=true;
            warning("File read failed");
        }
    }
    return b;
}

uint16_t file_readUint16BE(struct File* f) {
    uint8_t hi = file_readByte(f);
    uint8_t lo = file_readByte(f);
    return (hi << 8) | lo;
}

uint32_t file_readUint32BE(struct File* f) {
    uint16_t hi = file_readUint16BE(f);
    uint16_t lo = file_readUint16BE(f);
    return (hi << 16) | lo;
}

void file_write(struct File* f, void *ptr, uint32_t size) {
    if(f->gzipped)
    {
    }
    else
    {
        rb->write(f->fd, ptr, size);
    }
}

void file_writeByte(struct File* f, uint8_t b) {
    file_write(f, &b, 1);
}

void file_writeUint16BE(struct File* f, uint16_t n) {
    file_writeByte(f, n >> 8);
    file_writeByte(f, n & 0xFF);
}

void file_writeUint32BE(struct File* f, uint32_t n) {
    file_writeUint16BE(f, n >> 16);
    file_writeUint16BE(f, n & 0xFFFF);
}

void file_remove(const char* filename, const char* directory)
{
    char buf[512];
    rb->snprintf(buf, 512, "%s/%s", directory, filename);
    char *p = buf + rb->strlen(directory)+1;
    string_lower(p);
    if(rb->file_exists(buf))
    {
        rb->remove(buf);
    }
    else
    {
        string_upper(p);
        rb->remove(buf);
    }
}
