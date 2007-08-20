#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LANGUAGE_SIZE 20000

static char language_buffer[MAX_LANGUAGE_SIZE];

int lang_load(const char *filename)
{
    int fsize;
    int fd = open(filename, O_RDONLY);
    int retcode=0;
    unsigned char lang_header[3];
    if(fd == -1)
        return 1;
    if(3 == read(fd, lang_header, 3)) {
        unsigned char *ptr = language_buffer;
        int id;
        printf("%02x %02x %02x\n",
               lang_header[0], lang_header[1], lang_header[2]);

        fsize = read(fd, language_buffer, MAX_LANGUAGE_SIZE);
        
        while(fsize>3) {
            id = (ptr[0]<<8) | ptr[1];  /* get two-byte id */
            ptr+=2;                     /* pass the id */
            if(id < 2000) {
                printf("%03d %s\n", id, ptr);
            }
            while(*ptr) {               /* pass the string */
                fsize--;
                ptr++;
            }
            fsize-=3; /* the id and the terminating zero */
            ptr++;       /* pass the terminating zero-byte */
        }
    }
    close(fd);
    return retcode;
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        printf("Usage: lngdump <lng file>\n");
        return 2;
    }
    lang_load(argv[1]);
}
