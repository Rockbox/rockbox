/* Simple buffered version of file reader.
 * JPEG decoding seems to work faster with it.
 * Not fully tested. In case of any issues try FILEGETC (see SOURCES)
 * */
#include "rb_glue.h"

static int fd;
static unsigned char buff[256]; //TODO: Adjust it...
static int length = 0;
static int cur_buff_pos = 0;
static int file_pos = 0;

extern int GETC(void)
{
    if (cur_buff_pos >= length)
    {
        length = rb->read(fd, buff, sizeof(buff));
        file_pos += length;
        cur_buff_pos = 0;
    }

    return buff[cur_buff_pos++];
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
    int newPos = cur_buff_pos + d;
    if (newPos < length && newPos >= 0)
    {
        cur_buff_pos = newPos;
        return;
    }
    file_pos = rb->lseek(fd, (cur_buff_pos - length) + d, SEEK_CUR);
    cur_buff_pos = length = 0;
}

extern void POS(int d)
{
    cur_buff_pos = length = 0;
    file_pos = d;
    rb->lseek(fd, d, SEEK_SET);
}

extern int TELL(void)
{
    return file_pos + cur_buff_pos - length;
}

// OPEN/CLOSE file

extern void *OPEN(char *f)
{
    printf("Opening %s\n", f);
    cur_buff_pos = length = file_pos = 0;
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
    cur_buff_pos = length = file_pos = 0;
    return rb->close(fd);
}
