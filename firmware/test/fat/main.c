#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"
#include "disk.h"
#include "dir.h"
#include "file.h"

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf);
void dbg_print_bpb(struct bpb *bpb);
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

void dbg_print_bpb(struct bpb *bpb)
{
    DEBUGF("bpb_oemname = \"%s\"\n", bpb->bs_oemname);
    DEBUGF("bpb_bytspersec = %d\n", bpb->bpb_bytspersec);
    DEBUGF("bpb_secperclus = %d\n", bpb->bpb_secperclus);
    DEBUGF("bpb_rsvdseccnt = %d\n", bpb->bpb_rsvdseccnt);
    DEBUGF("bpb_numfats = %d\n", bpb->bpb_numfats);
    DEBUGF("bpb_rootentcnt = %d\n", bpb->bpb_rootentcnt);
    DEBUGF("bpb_totsec16 = %d\n", bpb->bpb_totsec16);
    DEBUGF("bpb_media = %02x\n", bpb->bpb_media);
    DEBUGF("bpb_fatsz16 = %d\n", bpb->bpb_fatsz16);
    DEBUGF("bpb_secpertrk = %d\n", bpb->bpb_secpertrk);
    DEBUGF("bpb_numheads = %d\n", bpb->bpb_numheads);
    DEBUGF("bpb_hiddsec = %u\n", bpb->bpb_hiddsec);
    DEBUGF("bpb_totsec32 = %u\n", bpb->bpb_totsec32);

    DEBUGF("bs_drvnum = %d\n", bpb->bs_drvnum);
    DEBUGF("bs_bootsig = %02x\n", bpb->bs_bootsig);
    if(bpb->bs_bootsig == 0x29)
    {
       DEBUGF("bs_volid = %xl\n", bpb->bs_volid);
       DEBUGF("bs_vollab = \"%s\"\n", bpb->bs_vollab);
       DEBUGF("bs_filsystype = \"%s\"\n", bpb->bs_filsystype);
    }

    DEBUGF("bpb_fatsz32 = %u\n", bpb->bpb_fatsz32);
    DEBUGF("last_word = %04x\n", bpb->last_word);

    DEBUGF("fat_type = FAT32\n");
}

void dbg_dir(char* currdir)
{
    DIR* dir;
    struct dirent* entry;

    dir = opendir(currdir);
    if (dir)
    {
        while ( (entry = readdir(dir)) ) {
            DEBUGF("%15s (%d bytes)\n", entry->d_name, entry->size);
        }
    }
    else
    {
        DEBUGF( "Could not open dir %s\n", currdir);
    }
    closedir(dir);
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
    if(ata_init()) {
        DEBUGF("*** Warning! The disk is uninitialized\n");
        return -1;
    }
    if (disk_init()) {
        DEBUGF("*** Failed reading partitions\n");
        return -1;
    }

    if(fat_mount(part[0].start)) {
        DEBUGF("*** Failed mounting fat\n");
    }

    dbg_console();

    return 0;
}

