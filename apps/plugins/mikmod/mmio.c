/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mmio.c,v 1.3 2005/03/30 19:10:58 realtech Exp $

  Portable file I/O routines

==============================================================================*/

/*

	The way this module works:

	- _mm_fopen will call the errorhandler [see mmerror.c] in addition to
	  setting _mm_errno on exit.
	- _mm_iobase is for internal use.  It is used by Player_LoadFP to
	  ensure that it works properly with wad files.
	- _mm_read_I_* and _mm_read_M_* differ : the first is for reading data
	  written by a little endian (intel) machine, and the second is for reading
	  big endian (Mac, RISC, Alpha) machine data.
	- _mm_write functions work the same as the _mm_read functions.
	- _mm_read_string is for reading binary strings.  It is basically the same
	  as an fread of bytes.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef SUNOS
extern int fclose(FILE *);
extern int fgetc(FILE *);
extern int fputc(int, FILE *);
extern size_t fread(void *, size_t, size_t, FILE *);
extern int fseek(FILE *, long, int);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
#endif

#define COPY_BUFSIZE  1024

/* some prototypes */
static int _mm_MemReader_Eof(MREADER* reader);
static int _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size);
static int _mm_MemReader_Get(MREADER* reader);
static int _mm_MemReader_Seek(MREADER* reader,long offset,int whence);
static long _mm_MemReader_Tell(MREADER* reader);

//static long _mm_iobase=0,temp_iobase=0;

int _mm_fopen(CHAR* fname,CHAR* attrib)
{
	int fp;

	//if(!(fp=fopen(fname,attrib))) {
	//	_mm_errno = MMERR_OPENING_FILE;
	//	if(_mm_errorhandler) _mm_errorhandler();
	//}
	fp = open(fname, O_RDONLY);
	if( fp < 0 ) {
		_mm_errno = MMERR_OPENING_FILE;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return fp;
}

int _mm_FileExists(CHAR* fname)
{
	int fp;

	//if(!(fp=fopen(fname,"r"))) return 0;
	//fclose(fp);
	fp = open(fname, O_RDONLY);
	if ( fp < 0 ) return 0;
	close(fp);
    
	return 1;
}

int _mm_fclose(int fp)
{
	//return fclose(fp);
	return close(fp);
}

/* Sets the current file-position as the new iobase */
void _mm_iobase_setcur(MREADER* reader)
{
	reader->prev_iobase=reader->iobase;  /* store old value in case of revert */
	reader->iobase=reader->Tell(reader);
}

/* Reverts to the last known iobase value. */
void _mm_iobase_revert(MREADER* reader)
{
	reader->iobase=reader->prev_iobase;
}

/*========== File Reader */

typedef struct MFILEREADER {
	MREADER core;
	int file;
} MFILEREADER;

static int _mm_FileReader_Eof(MREADER* reader)
{
    //return feof(((MFILEREADER*)reader)->file);
	int size   = filesize(((MFILEREADER*)reader)->file);
	int offset = lseek(((MFILEREADER*)reader)->file, 0, SEEK_CUR);
	return offset < 0;
	return (size <= 0 || offset < 0 || offset >= size) ? 1 : 0;
}

static int _mm_FileReader_Read(MREADER* reader,void* ptr,size_t size)
{
	//return !!fread(ptr,size,1,((MFILEREADER*)reader)->file);
	return read(((MFILEREADER*)reader)->file, ptr, size);
}

static int _mm_FileReader_Get(MREADER* reader)
{
	//return fgetc(((MFILEREADER*)reader)->file);
	unsigned char c;
	if ( read(((MFILEREADER*)reader)->file, &c, 1) )
		return c;
	else
		return EOF;
}

static int _mm_FileReader_Seek(MREADER* reader,long offset,int whence)
{
	//return fseek(((MFILEREADER*)reader)->file,
	//			 (whence==SEEK_SET)?offset+reader->iobase:offset,whence);
	return lseek(((MFILEREADER*)reader)->file,
				 (whence==SEEK_SET)?offset+reader->iobase:offset,whence);
}

static long _mm_FileReader_Tell(MREADER* reader)
{
	//return ftell(((MFILEREADER*)reader)->file)-reader->iobase;
	return lseek( (((MFILEREADER*)reader)->file)-reader->iobase, 0, SEEK_CUR );
}

MREADER *_mm_new_file_reader(int fp)
{
	MFILEREADER* reader=(MFILEREADER*)MikMod_malloc(sizeof(MFILEREADER));
	if (reader) {
		reader->core.Eof =&_mm_FileReader_Eof;
		reader->core.Read=&_mm_FileReader_Read;
		reader->core.Get =&_mm_FileReader_Get;
		reader->core.Seek=&_mm_FileReader_Seek;
		reader->core.Tell=&_mm_FileReader_Tell;
		reader->file=fp;
	}
	return (MREADER*)reader;
}

void _mm_delete_file_reader (MREADER* reader)
{
	if(reader) MikMod_free(reader);
}

/*========== File Writer */

typedef struct MFILEWRITER {
	MWRITER core;
	int file;
} MFILEWRITER;

static int _mm_FileWriter_Seek(MWRITER* writer,long offset,int whence)
{
	//return fseek(((MFILEWRITER*)writer)->file,offset,whence);
	return lseek(((MFILEREADER*)writer)->file,offset,whence);
}

static long _mm_FileWriter_Tell(MWRITER* writer)
{
	//return ftell(((MFILEWRITER*)writer)->file);
	return lseek(((MFILEWRITER*)writer)->file, 0, SEEK_CUR);
}

static int _mm_FileWriter_Write(MWRITER* writer,void* ptr,size_t size)
{
	//return (fwrite(ptr,size,1,((MFILEWRITER*)writer)->file)==size);
	return (write(ptr,size,((MFILEWRITER*)writer)->file)==size);
}

static int _mm_FileWriter_Put(MWRITER* writer,int value)
{
	//return fputc(value,((MFILEWRITER*)writer)->file);
    return 1; // TODO
}

MWRITER *_mm_new_file_writer(int fp)
{
	MFILEWRITER* writer=(MFILEWRITER*)MikMod_malloc(sizeof(MFILEWRITER));
	if (writer) {
		writer->core.Seek =&_mm_FileWriter_Seek;
		writer->core.Tell =&_mm_FileWriter_Tell;
		writer->core.Write=&_mm_FileWriter_Write;
		writer->core.Put  =&_mm_FileWriter_Put;
		writer->file=fp;
	}
	return (MWRITER*) writer;
}

void _mm_delete_file_writer (MWRITER* writer)
{
	if(writer) MikMod_free (writer);
}

/*========== Memory Reader */


typedef struct MMEMREADER {
	MREADER core;
	const void *buffer;
	long len;
	long pos;
} MMEMREADER;

void _mm_delete_mem_reader(MREADER* reader)
{
	if (reader) { MikMod_free(reader); }
}

MREADER *_mm_new_mem_reader(const void *buffer, int len)
{
	MMEMREADER* reader=(MMEMREADER*)MikMod_malloc(sizeof(MMEMREADER));
	if (reader)
	{
		reader->core.Eof =&_mm_MemReader_Eof;
		reader->core.Read=&_mm_MemReader_Read;
		reader->core.Get =&_mm_MemReader_Get;
		reader->core.Seek=&_mm_MemReader_Seek;
		reader->core.Tell=&_mm_MemReader_Tell;
		reader->buffer = buffer;
		reader->len = len;
		reader->pos = 0;
	}
	return (MREADER*)reader;
}

static int _mm_MemReader_Eof(MREADER* reader)
{
	if (!reader) { return 1; }
	if ( ((MMEMREADER*)reader)->pos > ((MMEMREADER*)reader)->len ) { 
		return 1; 
	}
	return 0;
}

static int _mm_MemReader_Read(MREADER* reader,void* ptr,size_t size)
{
	unsigned char *d=ptr;
	const unsigned char *s;
	
	if (!reader) { return 0; }

	if (reader->Eof(reader)) { return 0; }

	s = ((MMEMREADER*)reader)->buffer;
	s += ((MMEMREADER*)reader)->pos;

	if ( ((MMEMREADER*)reader)->pos + size > ((MMEMREADER*)reader)->len) 
	{
		((MMEMREADER*)reader)->pos = ((MMEMREADER*)reader)->len;
		return 0; /* not enough remaining bytes */
	}

	((MMEMREADER*)reader)->pos += (long)size;

	while (size--)
	{
		*d = *s;
		s++;
		d++;	
	}
	
	return 1;
}

static int _mm_MemReader_Get(MREADER* reader)
{
	int pos;

	if (reader->Eof(reader)) { return 0; }
	
	pos = ((MMEMREADER*)reader)->pos;
	((MMEMREADER*)reader)->pos++;

	return ((unsigned char*)(((MMEMREADER*)reader)->buffer))[pos];
}

static int _mm_MemReader_Seek(MREADER* reader,long offset,int whence)
{
	if (!reader) { return -1; }
	
	switch(whence)
	{
		case SEEK_CUR:
			((MMEMREADER*)reader)->pos += offset;
			break;
		case SEEK_SET:
			((MMEMREADER*)reader)->pos = offset;
			break;
		case SEEK_END:
			((MMEMREADER*)reader)->pos = ((MMEMREADER*)reader)->len - offset - 1;
			break;
	}
	if ( ((MMEMREADER*)reader)->pos < 0) { ((MMEMREADER*)reader)->pos = 0; }
	if ( ((MMEMREADER*)reader)->pos > ((MMEMREADER*)reader)->len ) { 
		((MMEMREADER*)reader)->pos = ((MMEMREADER*)reader)->len;
	}
	return 0;
}

static long _mm_MemReader_Tell(MREADER* reader)
{
	if (reader) {
		return ((MMEMREADER*)reader)->pos;
	}
	return 0;
}

/*========== Write functions */

void _mm_write_string(CHAR* data,MWRITER* writer)
{
	if(data)
		_mm_write_UBYTES(data,strlen(data),writer);
}

void _mm_write_M_UWORD(UWORD data,MWRITER* writer)
{
	_mm_write_UBYTE(data>>8,writer);
	_mm_write_UBYTE(data&0xff,writer);
}

void _mm_write_I_UWORD(UWORD data,MWRITER* writer)
{
	_mm_write_UBYTE(data&0xff,writer);
	_mm_write_UBYTE(data>>8,writer);
}

void _mm_write_M_ULONG(ULONG data,MWRITER* writer)
{
	_mm_write_M_UWORD(data>>16,writer);
	_mm_write_M_UWORD(data&0xffff,writer);
}

void _mm_write_I_ULONG(ULONG data,MWRITER* writer)
{
	_mm_write_I_UWORD(data&0xffff,writer);
	_mm_write_I_UWORD(data>>16,writer);
}

void _mm_write_M_SWORD(SWORD data,MWRITER* writer)
{
	_mm_write_M_UWORD((UWORD)data,writer);
}

void _mm_write_I_SWORD(SWORD data,MWRITER* writer)
{
	_mm_write_I_UWORD((UWORD)data,writer);
}

void _mm_write_M_SLONG(SLONG data,MWRITER* writer)
{
	_mm_write_M_ULONG((ULONG)data,writer);
}

void _mm_write_I_SLONG(SLONG data,MWRITER* writer)
{
	_mm_write_I_ULONG((ULONG)data,writer);
}

#if defined __STDC__ || defined _MSC_VER || defined MPW_C
#define DEFINE_MULTIPLE_WRITE_FUNCTION(type_name,type)						\
void _mm_write_##type_name##S (type *buffer,int number,MWRITER* writer)		\
{																			\
	while(number-->0)														\
		_mm_write_##type_name(*(buffer++),writer);							\
}
#else
#define DEFINE_MULTIPLE_WRITE_FUNCTION(type_name,type)						\
void _mm_write_/**/type_name/**/S (type *buffer,int number,MWRITER* writer)	\
{																			\
	while(number-->0)														\
		_mm_write_/**/type_name(*(buffer++),writer);						\
}
#endif

DEFINE_MULTIPLE_WRITE_FUNCTION(M_SWORD,SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(M_UWORD,UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_SWORD,SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_UWORD,UWORD)

DEFINE_MULTIPLE_WRITE_FUNCTION(M_SLONG,SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(M_ULONG,ULONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_SLONG,SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_ULONG,ULONG)

/*========== Read functions */

int _mm_read_string(CHAR* buffer,int number,MREADER* reader)
{
	return reader->Read(reader,buffer,number);
}

UWORD _mm_read_M_UWORD(MREADER* reader)
{
	UWORD result=((UWORD)_mm_read_UBYTE(reader))<<8;
	result|=_mm_read_UBYTE(reader);
	return result;
}

UWORD _mm_read_I_UWORD(MREADER* reader)
{
	UWORD result=_mm_read_UBYTE(reader);
	result|=((UWORD)_mm_read_UBYTE(reader))<<8;
	return result;
}

ULONG _mm_read_M_ULONG(MREADER* reader)
{
	ULONG result=((ULONG)_mm_read_M_UWORD(reader))<<16;
	result|=_mm_read_M_UWORD(reader);
	return result;
}

ULONG _mm_read_I_ULONG(MREADER* reader)
{
	ULONG result=_mm_read_I_UWORD(reader);
	result|=((ULONG)_mm_read_I_UWORD(reader))<<16;
	return result;
}

SWORD _mm_read_M_SWORD(MREADER* reader)
{
	return((SWORD)_mm_read_M_UWORD(reader));
}

SWORD _mm_read_I_SWORD(MREADER* reader)
{
	return((SWORD)_mm_read_I_UWORD(reader));
}

SLONG _mm_read_M_SLONG(MREADER* reader)
{
	return((SLONG)_mm_read_M_ULONG(reader));
}

SLONG _mm_read_I_SLONG(MREADER* reader)
{
	return((SLONG)_mm_read_I_ULONG(reader));
}

#if defined __STDC__ || defined _MSC_VER || defined MPW_C
#define DEFINE_MULTIPLE_READ_FUNCTION(type_name,type)						\
int _mm_read_##type_name##S (type *buffer,int number,MREADER* reader)		\
{																			\
	while(number-->0)														\
		*(buffer++)=_mm_read_##type_name(reader);							\
	return !reader->Eof(reader);											\
}
#else
#define DEFINE_MULTIPLE_READ_FUNCTION(type_name,type)						\
int _mm_read_/**/type_name/**/S (type *buffer,int number,MREADER* reader)	\
{																			\
	while(number-->0)														\
		*(buffer++)=_mm_read_/**/type_name(reader);							\
	return !reader->Eof(reader);											\
}
#endif

DEFINE_MULTIPLE_READ_FUNCTION(M_SWORD,SWORD)
DEFINE_MULTIPLE_READ_FUNCTION(M_UWORD,UWORD)
DEFINE_MULTIPLE_READ_FUNCTION(I_SWORD,SWORD)
DEFINE_MULTIPLE_READ_FUNCTION(I_UWORD,UWORD)

DEFINE_MULTIPLE_READ_FUNCTION(M_SLONG,SLONG)
DEFINE_MULTIPLE_READ_FUNCTION(M_ULONG,ULONG)
DEFINE_MULTIPLE_READ_FUNCTION(I_SLONG,SLONG)
DEFINE_MULTIPLE_READ_FUNCTION(I_ULONG,ULONG)

/* ex:set ts=4: */
