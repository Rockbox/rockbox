#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "fat.h"
#include "debug.h"
#include "disk.h"
#include "dir.h"
#include "file.h"

extern int ata_init(char*);
extern void ata_read_sectors(int, int, char*);

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf, int len);
void dbg_console(void);

void panicf( char *fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );
    fprintf(stderr,"***PANIC*** ");
    vfprintf(stderr, fmt, ap );
    va_end( ap );
    exit(1);
}

void dbg_dump_sector(int sec)
{
    unsigned char buf[512];

    ata_read_sectors(sec,1,buf);
    DEBUGF("---< Sector %d >-----------------------------------------\n", sec);
    dbg_dump_buffer(buf, 512);
}

void dbg_dump_buffer(unsigned char *buf, int len)
{
    int i, j;
    unsigned char c;
    unsigned char ascii[33];

    for(i = 0;i < len/16;i++)
    {
        DEBUGF("%03x: ", i*16);
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

int dbg_mkfile(char* name, int num)
{
    char text[8192];
    int i;
    int fd;
    int x=0;

    fd = open(name,O_WRONLY);
    if (fd<0) {
        DEBUGF("Failed creating file\n");
        return -1;
    }
    num *= 1024;
    while ( num ) {
        int len = num > sizeof text ? sizeof text : num;

        for (i=0; i<len/CHUNKSIZE; i++ )
            sprintf(text+i*CHUNKSIZE,"%07x,",x++);

        if (write(fd, text, len) < 0) {
            DEBUGF("Failed writing data\n");
            return -1;
        }
        num -= len;
        DEBUGF("wrote %d bytes\n",len);
    }
    
    close(fd);
    return 0;
}

int dbg_chkfile(char* name)
{
    char text[8192];
    int i;
    int x=0;
    int block=0;
    int fd = open(name,O_RDONLY);
    if (fd<0) {
        DEBUGF("Failed opening file\n");
        return -1;
    }
    while (1) {
        int rc = read(fd, text, sizeof text);
        DEBUGF("read %d bytes\n",rc);
        if (rc < 0) {
            DEBUGF("Failed writing data\n");
            return -1;
        }
        else {
            char tmp[CHUNKSIZE+1];
            if (!rc)
                break;
            for (i=0; i<rc/CHUNKSIZE; i++ ) {
                sprintf(tmp,"%07x,",x++);
                if (strncmp(text+i*CHUNKSIZE,tmp,CHUNKSIZE)) {
                    DEBUGF("Mismatch in byte %d (%.4s != %.4s)\n",
                           block*sizeof(text)+i*CHUNKSIZE, tmp,
                           text+i*CHUNKSIZE);
                    dbg_dump_buffer(text+i*CHUNKSIZE - 0x20, 0x40);
                    return -1;
                }
            }
        }
        block++;
    }
    
    close(fd);

    return 0;
}

void dbg_type(char* name)
{
    const int size = SECTOR_SIZE*5;
    unsigned char buf[size+1];
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

void dbg_head(char* name)
{
    unsigned char buf[SECTOR_SIZE*5];
    int fd,rc;

    fd = open(name,O_RDONLY);
    if (fd<0)
        return;
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

    close(fd);
}

char current_directory[256] = "\\";
int last_secnum = 0;

void dbg_prompt(void)
{
    DEBUGF("C:%s> ", current_directory);
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
            DEBUGF("secnum: %d\n", atoi(arg1));
            dbg_dump_sector(atoi(arg1));
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
            dbg_head(arg1);
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
                return dbg_mkfile(arg1,atoi(arg2));
            else
                return dbg_mkfile(arg1,1);
        }
    }

    if (!strcasecmp(cmd, "chkfile"))
    {
        if (arg1)
            return dbg_chkfile(arg1);
    }

    return 0;
}

extern void ata_exit(void);

int main(int argc, char *argv[])
{
    int rc,i;
    struct partinfo* pinfo;

    if(ata_init("disk.img")) {
        DEBUGF("*** Warning! The disk is uninitialized\n");
        return -1;
    }
    pinfo = disk_init();
    if (!pinfo) {
        DEBUGF("*** Failed reading partitions\n");
        return -1;
    }

    for ( i=0; i<4; i++ ) {
        if ( pinfo[i].type == PARTITION_TYPE_FAT32 ) {
            DEBUGF("*** Mounting at block %ld\n",pinfo[i].start);
            rc = fat_mount(pinfo[i].start);
            if(rc) {
                DEBUGF("mount: %d",rc);
                return -1;
            }
            break;
        }
    }
    if ( i==4 ) {
        if(fat_mount(0)) {
            DEBUGF("No FAT32 partition!");
            return -1;
        }
    }

    rc = dbg_cmd(argc, argv);

    ata_exit();

    return rc;
}

