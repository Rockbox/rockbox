#include "plugin.h"
#include "ficl.h"
#include <tlsf.h>
#include "console.h"

void *ficlMalloc(size_t size)
{
    return tlsf_malloc(size);
}

void *ficlRealloc(void *p, size_t size)
{
    return tlsf_realloc(p, size);
}

void ficlFree(void *p)
{
    tlsf_free(p);
}

void  ficlCallbackDefaultTextOut(ficlCallback *callback, char *message)
{
    FICL_IGNORE(callback);
    if (message != NULL)
    {
        console_puts(message);
    }
}


/* not supported under strict ANSI C */
int ficlFileStatus(char *filename, int *status)
{
    (void)filename;
   
    *status = -1;
    return -1;
}


/* gotta do it the hard way under strict ANSI C */
long ficlFileSize(ficlFile *ff)
{
    long currentOffset;
    long size;

    if (ff == NULL)
        return -1;

    currentOffset = ftell(ff->f);
    fseek(ff->f, 0, SEEK_END);
    size = ftell(ff->f);
    fseek(ff->f, currentOffset, SEEK_SET);

    return size;
}
