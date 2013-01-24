#include "ficl.h"




void *ficlMalloc(size_t size)
{
    return malloc(size);
}

void *ficlRealloc(void *p, size_t size)
{
    return realloc(p, size);
}

void ficlFree(void *p)
{
    free(p);
}

void  ficlCallbackDefaultTextOut(ficlCallback *callback, char *message)
{
    FICL_IGNORE(callback);
    if (message != NULL)
        fputs(message, stdout);
    else
        fflush(stdout);
    return;
}


/* not supported under strict ANSI C */
int ficlFileStatus(char *filename, int *status)
{
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



void ficlSystemCompilePlatform(ficlSystem *system)
{
    return;
}


