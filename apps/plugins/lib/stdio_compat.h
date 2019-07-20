/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 by Marcin Bukat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stddef.h>

#define MAX_STDIO_FILES 11

#undef FILE
#define FILE _FILE_

#define fopen  _fopen_
#define fclose _fclose_
#define fflush _fflush_
#define fread  _fread_
#define fwrite _fwrite_
#define fseek  _fseek_
#define fseeko _fseek_
#define ftell  _ftell_
#define ftello _ftell_
#define fgetc  _fgetc_
#define ungetc _ungetc_
#define fputc  _fputc_
#define fgets  _fgets_
#undef clearerr
#define clearerr _clearerr_
#undef ferror
#define ferror _ferror_
#undef feof
#define feof   _feof_
#define fprintf _fprintf_
#undef stdout
#define stdout _stdout_
#undef stderr
#define stderr _stderr_
#undef getc
#define getc fgetc

typedef struct {
    int fd;
    int unget_char;
    int error;
} _FILE_;

extern _FILE_ *_stdout_, *_stderr_;

_FILE_ *_fopen_(const char *path, const char *mode);
int _fclose_(_FILE_ *stream);
int _fflush_(_FILE_ *stream);
size_t _fread_(void *ptr, size_t size, size_t nmemb, _FILE_ *stream);
size_t _fwrite_(const void *ptr, size_t size, size_t nmemb, _FILE_ *stream);
int _fseek_(_FILE_ *stream, long offset, int whence);
long _ftell_(_FILE_ *stream);
int _fgetc_(_FILE_ *stream);
int _ungetc_(int c, _FILE_ *stream);
int _fputc_(int c, _FILE_ *stream);
char *_fgets_(char *s, int size, _FILE_ *stream);
int _unlink_(const char *pathname);
void _clearerr_(_FILE_ *stream);
int _ferror_(_FILE_ *stream);
int _feof_(_FILE_ *stream);
int _fprintf_(_FILE_ *stream, const char *format, ...);
