#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "debug.h"
#include "disk.h"
#include "dir.h"
#include "file.h"

extern int ata_init(char*);
extern void ata_read_sectors(int, int, char*);

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf);
void dbg_console(void);

void dbg_dump_sector(int sec)
{
    unsigned char buf[512];

    ata_read_sectors(sec,1,buf);
    DEBUGF("---< Sector %d >-----------------------------------------\n", sec);
    dbg_dump_buffer(buf);
}

void dbg_dump_buffer(unsigned char *buf)
{
    int i, j;
    unsigned char c;
    unsigned char ascii[33];

    for(i = 0;i < 512/16;i++)
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

void dbg_mkfile(char* name)
{
    char* text = "Detta är en dummy-text\n";
    int fd = open(name,O_WRONLY);
    if (fd<0) {
        DEBUGF("Failed creating file\n");
        return;
    }
    if (write(fd, text, strlen(text)) < 0)
        DEBUGF("Failed writing data\n");

    close(fd);
}

void dbg_type(char* name)
{
    unsigned char buf[SECTOR_SIZE*5];
    int i,fd,rc;

    fd = open(name,O_RDONLY);
    if (fd<0)
        return;
    DEBUGF("Got file descriptor %d\n",fd);
    
    for (i=0;i<5;i++) {
        rc = read(fd, buf, SECTOR_SIZE*2/3);
        if( rc > 0 )
        {
            buf[rc]=0;
            printf("%d: %s\n", i, buf);
        }
        else if ( rc == 0 ) {
            DEBUGF("EOF\n");
            break;
        }
        else
        {
            DEBUGF("Failed reading file: %d\n",rc);
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
    
    rc = lseek(fd,512,SEEK_SET);
    if ( rc >= 0 ) {
        rc = read(fd, buf, SECTOR_SIZE);
        if( rc > 0 )
        {
            buf[rc]=0;
            printf("%d: %s\n", strlen(buf), buf);
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

    rc = lseek(fd,-100,SEEK_CUR);
    if ( rc >= 0 ) {
        rc = read(fd, buf, SECTOR_SIZE);
        if( rc > 0 )
        {
            buf[rc]=0;
            printf("%d: %s\n", strlen(buf), buf);
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

char current_directory[256] = "\\";
int last_secnum = 0;

void dbg_prompt(void)
{
    DEBUGF("C:%s> ", current_directory);
}

void dbg_console(void)
{
    char cmd[32] = "";
    char last_cmd[32] = "";
    int quit = 0;
    char *s;

    while(!quit)
    {
        dbg_prompt();
        if(fgets(cmd, sizeof(cmd) - 1, stdin))
        {
            if(strlen(cmd) == 1) /* empty command? */
            {
                strcpy(cmd, last_cmd);
            }

            /* Get the first token */
	    s = strtok(cmd, " \n");
            if(s)
            {
                if(!strcasecmp(s, "dir"))
                {
                    s = strtok(NULL, " \n");
                    if (!s)
                        s = "/";
                    dbg_dir(s);
                    continue;
                }

                if(!strcasecmp(s, "ds"))
                {
                    /* Remember the command */
                    strcpy(last_cmd, s);
                    
                    if((s = strtok(NULL, " \n")))
                    {
                        last_secnum = atoi(s);
                    }
                    else
                    {
                        last_secnum++;
                    }
                    DEBUGF("secnum: %d\n", last_secnum);
                    dbg_dump_sector(last_secnum);
                    continue;
                }

                if(!strcasecmp(s, "type"))
                {
                    s = strtok(NULL, " \n");
                    if (!s)
                        continue;
                    dbg_type(s);
                    continue;
                }
            
                if(!strcasecmp(s, "exit") ||
                   !strcasecmp(s, "x"))
                {
                    quit = 1;
                }
            }
        }
        else
            quit = 1;
    }
}

int main(int argc, char *argv[])
{
    int rc,i;
    struct partinfo* pinfo;

    if(ata_init(argv[1])) {
        DEBUGF("*** Warning! The disk is uninitialized\n");
        return -1;
    }
    pinfo = disk_init();
    if (!pinfo) {
        DEBUGF("*** Failed reading partitions\n");
        return -1;
    }

    if ( argc > 2 ) {
        dbg_dump_sector(atoi(argv[2]));
        return 0;
    }

    for ( i=0; i<4; i++ ) {
        if ( pinfo[i].type == PARTITION_TYPE_FAT32 ) {
            DEBUGF("*** Mounting at block %ld\n",pinfo[i].start);
            rc = fat_mount(pinfo[i].start);
            if(rc) {
                DEBUGF("mount: %d",rc);
                return 0;
            }
            break;
        }
    }
    if ( i==4 ) {
        if(fat_mount(0)) {
            DEBUGF("No FAT32 partition!");
            return 0;
        }
    }

    //dbg_console();
    //dbg_tail("/fat.h");
    //dbg_dir("/");
    dbg_mkfile("/apa.txt");
    dbg_dir("/");

    return 0;
}

