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

#include "plugin.h"
#include "stdio_compat.h"
#include "errno.h"

static _FILE_ __file__[MAX_STDIO_FILES] = {
    {-1,-1,0},{-1,-1,0},{-1,-1,0},{-1,-1,0},
    {-1,-1,0},{-1,-1,0},{-1,-1,0},{-1,-1,0},
    {-1,-1,0},{-1,-1,0},{-1,-1,0}
};

_FILE_ *_stdout_ = NULL, *_stderr_ = NULL;

_FILE_ *_fopen_(const char *path, const char *mode)
{
    _FILE_ *f = __file__;
    int flags = O_RDONLY;
    int fd = -1;
    int i;

    /* look for free slot */
    for (i=0; i<MAX_STDIO_FILES; i++, f++)
        if (f->fd == -1)
            break;

    /* no empty slots */
    if (i == MAX_STDIO_FILES)
    {
        rb->splash(HZ, "no open slots");
        return NULL;
    }

    if (*mode != 'r')
    {
        /* Not read */
        flags = (O_WRONLY | O_CREAT | O_TRUNC);
        if (*mode != 'w')
        {
            /* Not write (create of append) */
            flags = (O_WRONLY | O_CREAT | O_APPEND);
            if (*mode != 'a')
            {
                /* Illegal */
                return NULL;
            }
        }
    }

    if (mode[1] == 'b')
    {
        /* Ignore */
        mode++;
    }

    if (mode[1] == '+')
    {
        /* Read and Write. */
        flags |= (O_RDONLY | O_WRONLY);
        flags += (O_RDWR - (O_RDONLY | O_WRONLY));
    }


    fd = rb->open(path, flags, 0666);

    if (fd < 0)
    {
        return NULL;
    }

    /* initialize */
    f->fd = fd;
    f->unget_char = -1;

    return f;
}

int _fflush_(_FILE_ *stream)
{
    (void)stream;
    /* no buffering so we are always in sync */
    return 0;
}

size_t _fread_(void *ptr, size_t size, size_t nmemb, _FILE_ *stream)
{
    ssize_t ret = rb->read(stream->fd, (char *)ptr, size*nmemb);

    if (ret < (ssize_t)(size*nmemb))
        stream->error = -1;
    return ret / size;
}

size_t _fwrite_(const void *ptr, size_t size, size_t nmemb, _FILE_ *stream)
{
    if(stream)
    {
        ssize_t ret = rb->write(stream->fd, ptr, size*nmemb);

        if (ret < (ssize_t)(size*nmemb))
            stream->error = -1;

        return ret / size;
    }
    /* stderr, stdout (disabled) */
    else
    {
        return size * nmemb;
    }
}

int _fseek_(_FILE_ *stream, long offset, int whence)
{
    if(rb->lseek(stream->fd, offset, whence) >= 0)
        return 0;
    else
    {
        rb->splashf(HZ, "lseek() failed: %d fd:%d", errno, stream->fd);
        return -1;
    }
}

long _ftell_(_FILE_ *stream)
{
    return rb->lseek(stream->fd, 0, SEEK_CUR);
}

int _fclose_(_FILE_ *stream)
{
    if (stream->fd == -1)
        return EOF;

    if (rb->close(stream->fd) == -1)
        return EOF;

    /* free the resource */
    stream->fd = -1;

    return 0;
}

int _fgetc_(_FILE_ *stream)
{
    unsigned char c;

    if (stream->unget_char != -1)
    {
        c = stream->unget_char;
        stream->unget_char = -1;
        return c;
    }

    if ( rb->read(stream->fd, &c, 1) != 1)
        return EOF;

    return c;
}

int _ungetc_(int c, _FILE_ *stream)
{
    stream->unget_char = c;

    return c;
}

int _fputc_(int c, _FILE_ *stream)
{
    unsigned char cb = c;

    if (rb->write(stream->fd, &cb, 1) != 1)
        return EOF;

    return cb;
}

char *_fgets_(char *s, int size, _FILE_ *stream)
{
    char *p = s;
    char c = '\0';

    for (int i=0; i<size; i++)
    {
        if ( rb->read(stream->fd, &c, 1) != 1)
            break;

        *p++ = c;

        if (c == '\n')
            break;
    }

    if (p == s)
        return NULL;

    *p = '\0';

    return s;
}

void _clearerr_(_FILE_ *stream)
{
    if (stream)
        stream->error = 0;
}

int _ferror_(_FILE_ *stream)
{
    return (stream) ? stream->error : -1;
}

int _feof_(_FILE_ *stream)
{
    int c = _fgetc_(stream);

    if (c != EOF)
    {
        _ungetc_(c, stream);
        return 0;
    }

    return 0;
}

int _fprintf_(_FILE_ *stream, const char *format, ...)
{
    int len;
    va_list ap;
    char buf[256];

    va_start(ap, format);
    len = rb->vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    return _fwrite_(buf, 1, len, stream);
}
