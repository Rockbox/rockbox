#include "rocklibc.h"

int PREFIX(getc)(int fd)
{
    unsigned char c;
    if (rb->read(fd, &c, 1))
        return c;
    else
        return EOF;
}
