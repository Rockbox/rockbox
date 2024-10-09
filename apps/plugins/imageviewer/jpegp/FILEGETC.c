#include "rb_glue.h"

static int fd;

extern int GETC(void)
{
    unsigned char x;
    rb->read(fd, &x, 1);
    return x;
}

// multibyte readers: host-endian independent - if evaluated in right order (ie. don't optimize)

extern int GETWbi(void)		// 16-bit big-endian
{
    return ( GETC()<<8 ) | GETC();
}

extern int GETDbi(void)		// 32-bit big-endian
{
    return ( GETC()<<24 ) | ( GETC()<<16 ) | ( GETC()<<8 ) | GETC();
}

extern int GETWli(void)		// 16-bit little-endian
{
    return GETC() | ( GETC()<<8 );
}

extern int GETDli(void)		// 32-bit little-endian
{
    return GETC() | ( GETC()<<8 ) | ( GETC()<<16 ) | ( GETC()<<24 );
}

// seek

extern void SEEK(int d)
{
    rb->lseek(fd, d, SEEK_CUR);
}

extern void POS(int d)
{
    rb->lseek(fd, d, SEEK_SET);
}

extern int TELL(void)
{
    return rb->lseek(fd, 0, SEEK_CUR);
}

// OPEN/CLOSE file

extern void *OPEN(char *f)
{
    printf("Opening %s\n", f);

    fd = rb->open(f,O_RDONLY);

    if (  fd < 0 )
    {
        printf("Error opening %s\n", f);
        return NULL;
    }

    return &fd;
}

extern int CLOSE(void)
{
    return rb->close(fd);
}
