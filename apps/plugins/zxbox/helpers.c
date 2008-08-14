#include "zxconfig.h"
#include "helpers.h"

int my_getc(int fd){
    unsigned char c;
    if ( rb->read(fd, &c, 1) )
        return c;
    else
        return EOF;
}

off_t my_ftell(int fd){
    return rb->lseek(fd, 0, SEEK_CUR);
}

int my_putc(char c , int fd){
    return rb->write(fd,&c,1);
}

void *my_malloc(size_t size)
{
    static char *offset = NULL;
    static ssize_t totalSize = 0;
    char *ret;

    int remainder = size % 4;

    size = size + 4-remainder;

    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer((size_t *)&totalSize);
    }

    if (size + 4 > abs(totalSize) )
    {
        /* We've made our point. */
        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;

}
