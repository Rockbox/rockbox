#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "fat.h"
#include "debug.h"
#include "disk.h"
#include "dir.h"
#include "file.h"

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf, int len, int offset);
void dbg_console(void);

void mutex_init(void* l) {}
void mutex_lock(void* l) {}
void mutex_unlock(void* l) {}

void panicf( char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf(stderr,"***PANIC*** ");
    vfprintf(stderr, fmt, ap );
    va_end( ap );
    exit(1);
}

void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf(stderr,"DEBUGF: ");
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "%s:%d ", file, line );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
}

void dbg_dump_sector(int sec)
{
    unsigned char buf[512];

    ata_read_sectors(sec,1,buf);
    DEBUGF("---< Sector %d >-----------------------------------------\n", sec);
    dbg_dump_buffer(buf, 512, 0);
}

void dbg_dump_buffer(unsigned char *buf, int len, int offset)
{
    int i, j;
    unsigned char c;
    unsigned char ascii[33];

    for(i = 0;i < len/16;i++)
    {
        DEBUGF("%03x: ", i*16 + offset);
        for(j = 0;j < 16;j++)
        {
            c = buf[i*16+j];

            DEBUGF("%02x ", c);
            if(c < 32 || c > 127)
            {
                ascii[j] = '.';
            }
            else
            {
                ascii[j] = c;
            }
        }

        ascii[j] = 0;
        DEBUGF("%s\n", ascii);
    }
}

void dbg_dir(char* currdir)
{
    DIR* dir;
    struct dirent* entry;

    dir = opendir(currdir);
    if (dir)
    {
        while ( (entry = readdir(dir)) ) {
            DEBUGF("%15s (%d bytes) %x\n", 
                   entry->d_name, entry->size, entry->startcluster);
        }
        closedir(dir);
    }
    else
    {
        DEBUGF( "Could not open dir %s\n", currdir);
    }
}

#define CHUNKSIZE 8
#define BUFSIZE 8192

int dbg_mkfile(char* name, int num)
{
    char text[BUFSIZE+1];
    int i;
    int fd;
    int x=0;
    bool stop = false;

    fd = creat(name,O_WRONLY);
    if (fd<0) {
        DEBUGF("Failed creating file\n");
        return -1;
    }
    num *= 1024;
    while ( num ) {
        int rc;
        int len = num > BUFSIZE ? BUFSIZE : num;

        for (i=0; i<len/CHUNKSIZE; i++ )
            sprintf(text+i*CHUNKSIZE,"%c%06x,",name[1],x++);

        rc = write(fd, text, len);
        if ( rc < 0 ) {
            DEBUGF("Failed writing data\n");
            return -1;
        }
        else
            if ( rc == 0 ) {
                DEBUGF("No space left\n");
                return -2;
            }
            else
                DEBUGF("wrote %d bytes\n",rc);

        num -= len;

        if ( !num ) {
            if ( stop )
                break;

            /* add a random number of chunks to test byte-copy code */
            num = ((int) rand() % SECTOR_SIZE) & ~7;
            LDEBUGF("Adding random size %d\n",num);
            stop = true;
        }
    }

    return close(fd);
}


int dbg_chkfile(char* name, int size)
{
    char text[81920];
    int i;
    int x=0;
    int pos = 0;
    int block=0;
    int fd = open(name,O_RDONLY);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    DEBUGF("File is %d bytes\n", size);
    /* random start position */
    if ( size )
        pos = ((int)rand() % size) & ~7;
    lseek(fd, pos, SEEK_SET);
    x = pos / CHUNKSIZE;

    LDEBUGF("Check base is %x (%d)\n",x,pos);

    while (1) {
        int rc = read(fd, text, sizeof text);
        DEBUGF("read %d bytes\n",rc);
        if (rc < 0) {
            panicf("Failed reading data\n");
        }
        else {
            char tmp[CHUNKSIZE+1];
            if (!rc)
                break;
            for (i=0; i<rc/CHUNKSIZE; i++ ) {
                sprintf(tmp,"%c%06x,",name[1],x++);
                if (strncmp(text+i*CHUNKSIZE,tmp,CHUNKSIZE)) {
                    int idx = pos + block*sizeof(text) + i*CHUNKSIZE;
                    DEBUGF("Mismatch in byte 0x%x (byte 0x%x of sector 0x%x)."
                           "\nExpected %.8s found %.8s\n",
                           idx, idx % SECTOR_SIZE, idx / SECTOR_SIZE,
                           tmp,
                           text+i*CHUNKSIZE);
                    DEBUGF("i=%x, idx=%x\n",i,idx);
                    dbg_dump_buffer(text+i*CHUNKSIZE - 0x20, 0x40, idx - 0x20);
                    return -1;
                }
            }
        }
        block++;
    }
    
    return close(fd);
}

int dbg_wrtest(char* name)
{
    char text[81920];
    int i;
    int x=0;
    int pos = 0;
    int block=0;
    int size, fd, rc;
    char tmp[CHUNKSIZE+1];

    fd = open(name,O_RDWR);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    DEBUGF("File is %d bytes\n", size);
    /* random start position */
    if ( size )
        pos = ((int)rand() % size) & ~7;
    rc = lseek(fd, pos, SEEK_SET);
    if ( rc < 0 )
        panicf("Failed seeking\n");
    x = pos / CHUNKSIZE;
    LDEBUGF("Check base is %x (%d)\n",x,pos);

    sprintf(tmp,"%c%06x,",name[1],x++);
    rc = write(fd, tmp, 8);
    if ( rc < 0 )
        panicf("Failed writing data\n");

    if ( size )
        pos = ((int)rand() % size) & ~7;
    rc = lseek(fd, pos, SEEK_SET);
    if ( rc < 0 )
        panicf("Failed seeking\n");
    x = pos / CHUNKSIZE;
    LDEBUGF("Check base 2 is %x (%d)\n",x,pos);

    while (1) {
        rc = read(fd, text, sizeof text);
        DEBUGF("read %d bytes\n",rc);
        if (rc < 0) {
            panicf("Failed reading data\n");
        }
        else {
            if (!rc)
                break;
            for (i=0; i<rc/CHUNKSIZE; i++ ) {
                sprintf(tmp,"%c%06x,",name[1],x++);
                if (strncmp(text+i*CHUNKSIZE,tmp,CHUNKSIZE)) {
                    int idx = pos + block*sizeof(text) + i*CHUNKSIZE;
                    DEBUGF("Mismatch in byte 0x%x (byte 0x%x of sector 0x%x)."
                           "\nExpected %.8s found %.8s\n",
                           idx, idx % SECTOR_SIZE, idx / SECTOR_SIZE,
                           tmp,
                           text+i*CHUNKSIZE);
                    DEBUGF("i=%x, idx=%x\n",i,idx);
                    dbg_dump_buffer(text+i*CHUNKSIZE - 0x20, 0x40, idx - 0x20);
                    return -1;
                }
            }
        }
        block++;
    }
    
    return close(fd);
}

void dbg_type(char* name)
{
    const int size = SECTOR_SIZE*5;
    unsigned char buf[SECTOR_SIZE*5+1];
    int fd,rc;

    fd = open(name,O_RDONLY);
    if (fd<0)
        return;
    DEBUGF("Got file descriptor %d\n",fd);
    
    while ( 1 ) {
        rc = read(fd, buf, size);
        if( rc > 0 )
        {
            buf[size] = 0;
            printf("%d: %.*s\n", rc, rc, buf);
        }
        else if ( rc == 0 ) {
            DEBUGF("EOF\n");
            break;
        }
        else
        {
            DEBUGF("Failed reading file: %d\n",rc);
            break;
        }
    }
    close(fd);
}

int dbg_append(char* name)
{
    int x=0;
    int size, fd, rc;
    char tmp[CHUNKSIZE+1];

    fd = open(name,O_RDONLY);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    DEBUGF("File is %d bytes\n", size);
    x = size / CHUNKSIZE;
    LDEBUGF("Check base is %x (%d)\n",x,size);

    if (close(fd) < 0)
        return -1;

    fd = open(name,O_RDWR|O_APPEND);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }

    sprintf(tmp,"%c%06x,",name[1],x++);
    rc = write(fd, tmp, 8);
    if ( rc < 0 )
        panicf("Failed writing data\n");

    return close(fd);
}

int dbg_test(char* name)
{
    int x=0;
    int j;
    int fd;
    char text[BUFSIZE+1];

    for (j=0; j<5; j++) {
        int num = 40960;

        fd = open(name,O_WRONLY|O_CREAT|O_APPEND);
        if (fd<0) {
            DEBUGF("Failed opening file\n");
            return -1;
        }

        while ( num ) {
            int rc, i;
            int len = num > BUFSIZE ? BUFSIZE : num;

            for (i=0; i<len/CHUNKSIZE; i++ )
                sprintf(text+i*CHUNKSIZE,"%c%06x,",name[1],x++);

            rc = write(fd, text, len);
            if ( rc < 0 ) {
                DEBUGF("Failed writing data\n");
                return -1;
            }
            else
                if ( rc == 0 ) {
                    DEBUGF("No space left\n");
                    return -2;
                }
                else
                    DEBUGF("wrote %d bytes\n",rc);

            num -= len;
        }

        if (close(fd) < 0)
            return -1;
    }    

    return 0;
}

int dbg_dump(char* name, int offset)
{
    char buf[SECTOR_SIZE];

    int rc;
    int fd = open(name,O_RDONLY);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }
    lseek(fd, offset, SEEK_SET);
    rc = read(fd, buf, sizeof buf);

    if ( rc < 0 )
        panicf("Error reading data\n");

    if (close(fd) < 0)
        return -1;

    dbg_dump_buffer(buf, rc, offset);

    return 0;
}

void dbg_tail(char* name)
{
    unsigned char buf[SECTOR_SIZE*5];
    int fd,rc;

    fd = open(name,O_RDONLY);
    if (fd<0)
        return;
    DEBUGF("Got file descriptor %d\n",fd);
    
    rc = lseek(fd,-512,SEEK_END);
    if ( rc >= 0 ) {
        rc = read(fd, buf, SECTOR_SIZE);
        if( rc > 0 )
        {
            buf[rc]=0;
            printf("%d:\n%s\n", strlen(buf), buf);
        }
        else if ( rc == 0 ) {
            DEBUGF("EOF\n");
        }
        else
        {
            DEBUGF("Failed reading file: %d\n",rc);
        }
    }
    else {
        perror("lseek");
    }

    close(fd);
}

int dbg_head(char* name)
{
    unsigned char buf[SECTOR_SIZE*5];
    int fd,rc;

    fd = open(name,O_RDONLY);
    if (fd<0)
        return -1;
    DEBUGF("Got file descriptor %d\n",fd);
    
    rc = read(fd, buf, SECTOR_SIZE*3);
    if( rc > 0 )
    {
        buf[rc]=0;
        printf("%d:\n%s\n", strlen(buf), buf);
    }
    else if ( rc == 0 ) {
        DEBUGF("EOF\n");
    }
    else
    {
        DEBUGF("Failed reading file: %d\n",rc);
    }

    return close(fd);
}

int dbg_trunc(char* name, int size)
{
    int fd,rc;

#if 1
    fd = open(name,O_RDWR);
    if (fd<0)
        return -1;

    rc = ftruncate(fd, size);
    if (rc<0) {
        DEBUGF("ftruncate(%d) failed\n", size);
        return -2;
    }

#else
    fd = open(name,O_RDWR|O_TRUNC);
    if (fd<0)
        return -1;

    rc = lseek(fd, size, SEEK_SET);
    if (fd<0)
        return -2;
#endif

    return close(fd);
}

int dbg_mkdir(char* name)
{
    int fd;

    fd = mkdir(name, 0);
    if (fd<0) {
        DEBUGF("Failed creating directory\n");
        return -1;
    }
    return 0;
}

int dbg_cmd(int argc, char *argv[])
{
    char* cmd = NULL;
    char* arg1 = NULL;
    char* arg2 = NULL;

    if (argc > 1) {
        cmd = argv[1];
        if ( argc > 2 ) {
            arg1 = argv[2];
            if ( argc > 3 ) {
                arg2 = argv[3];
            }
        }
    }
    else {
        DEBUGF("usage: fat command [options]\n"
               "commands:\n"
               " dir <dir>\n"
               " ds <sector> - display sector\n"
               " type <file>\n"
               " head <file>\n"
               " tail <file>\n"
               " mkfile <file> <size (KB)>\n"
               " chkfile <file>\n"
               " del <file>\n"
               " rmdir <dir>\n"
               " dump <file> <offset>\n"
               " mkdir <dir>\n"
               " trunc <file> <size>\n"
               " wrtest <file>\n"
               " append <file>\n"
               " test <file>\n"
               " ren <file> <newname>\n"
            );
        return -1;
    }

    if (!strcasecmp(cmd, "dir"))
    {
        if ( arg1 )
            dbg_dir(arg1);
        else
            dbg_dir("/");
    }

    if (!strcasecmp(cmd, "ds"))
    {                    
        if ( arg1 ) {
            DEBUGF("secnum: %d\n", strtol(arg1, NULL, 0));
            dbg_dump_sector(strtol(arg1, NULL, 0));
        }
    }

    if (!strcasecmp(cmd, "type"))
    {
        if (arg1)
            dbg_type(arg1);
    }

    if (!strcasecmp(cmd, "head"))
    {
        if (arg1)
            return dbg_head(arg1);
    }
            
    if (!strcasecmp(cmd, "tail"))
    {
        if (arg1)
            dbg_tail(arg1);
    }

    if (!strcasecmp(cmd, "mkfile"))
    {
        if (arg1) {
            if (arg2)
                return dbg_mkfile(arg1,strtol(arg2, NULL, 0));
            else
                return dbg_mkfile(arg1,1);
        }
    }

    if (!strcasecmp(cmd, "chkfile"))
    {
        if (arg1) {
            if (arg2)
                return dbg_chkfile(arg1, strtol(arg2, NULL, 0));
            else
                return dbg_chkfile(arg1, 0);
        }
    }

    if (!strcasecmp(cmd, "mkdir"))
    {
        if (arg1) {
            return dbg_mkdir(arg1);
        }
    }

    if (!strcasecmp(cmd, "del"))
    {
        if (arg1)
            return remove(arg1);
    }

    if (!strcasecmp(cmd, "rmdir"))
    {
        if (arg1)
            return rmdir(arg1);
    }

    if (!strcasecmp(cmd, "dump"))
    {
        if (arg1) {
            if (arg2)
                return dbg_dump(arg1, strtol(arg2, NULL, 0));
            else
                return dbg_dump(arg1, 0);
        }
    }

    if (!strcasecmp(cmd, "wrtest"))
    {
        if (arg1)
            return dbg_wrtest(arg1);
    }

    if (!strcasecmp(cmd, "append"))
    {
        if (arg1)
            return dbg_append(arg1);
    }

    if (!strcasecmp(cmd, "test"))
    {
        if (arg1)
            return dbg_test(arg1);
    }

    if (!strcasecmp(cmd, "trunc"))
    {
        if (arg1 && arg2)
            return dbg_trunc(arg1, strtol(arg2, NULL, 0));
    }

    if (!strcasecmp(cmd, "ren"))
    {
        if (arg1 && arg2)
            return rename(arg1, arg2);
    }

    return 0;
}

extern void ata_exit(void);

int main(int argc, char *argv[])
{
    int rc,i;
    struct partinfo* pinfo;

    srand(clock());

    if(ata_init()) {
        DEBUGF("*** Warning! The disk is uninitialized\n");
        return -1;
    }
    pinfo = disk_init();
    if (!pinfo) {
        DEBUGF("*** Failed reading partitions\n");
        return -1;
    }

    for ( i=0; i<4; i++ ) {
        if ( pinfo[i].type == PARTITION_TYPE_FAT32 
#ifdef HAVE_FAT16SUPPORT
          || pinfo[i].type == PARTITION_TYPE_FAT16
#endif
        ) {
            DEBUGF("*** Mounting at block %ld\n",pinfo[i].start);
            rc = fat_mount(IF_MV2(0,) IF_MV2(0,) pinfo[i].start);
            if(rc) {
                DEBUGF("mount: %d",rc);
                return -1;
            }
            break;
        }
    }
    if ( i==4 ) {
        if(fat_mount(IF_MV2(0,) IF_MV2(0,) 0)) {
            DEBUGF("No FAT32 partition!");
            return -1;
        }
    }

    rc = dbg_cmd(argc, argv);

    ata_exit();

    return rc;
}

