#include "plugin.h"
#include "stdio_compat.h"

static _FILE_ __file__[MAX_STDIO_FILES] = {
    {-1,-1},{-1,-1},{-1,-1},{-1,-1},
    {-1,-1},{-1,-1},{-1,-1},{-1,-1}
};

_FILE_ *_fopen_(const char *path, const char *mode)
{
    _FILE_ *f = __file__;
    int flags = O_RDONLY;
    int fd = -1;
    int i;

    /* look for free slot */
    for (i=0; i<MAX_OPEN_FILES; i++, f++)
        if (f->fd == -1)
            break;

    /* no empty slots */
    if (i == MAX_STDIO_FILES)
        return NULL;

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


    fd = rb->open(path, flags);

    if (fd == -1)
        return NULL;

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
    /* equivalent of fread() */
    return rb->read(stream->fd, (char *)ptr, size*nmemb);
}

size_t _fwrite_(const void *ptr, size_t size, size_t nmemb, _FILE_ *stream)
{
    return rb->write(stream->fd, ptr, size*nmemb);
}

int _fseek_(_FILE_ *stream, long offset, int whence)
{
    return rb->lseek(stream->fd, offset, whence);
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
