#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"
#include "disk.h"
#include "dir.h"

void dbg_dump_sector(int sec);
void dbg_dump_buffer(unsigned char *buf);
void dbg_print_bpb(struct bpb *bpb);
void dbg_console(void);

void dbg_dump_sector(int sec)
{
    unsigned char buf[512];

    ata_read_sectors(sec,1,buf);
    printf("---< Sector %d >-----------------------------------------\n", sec);
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

            printf("%02x ", c);
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
        printf("%s\n", ascii);
    }
}

void dbg_print_bpb(struct bpb *bpb)
{
    printf("bpb_oemname = \"%s\"\n", bpb->bs_oemname);
    printf("bpb_bytspersec = %d\n", bpb->bpb_bytspersec);
    printf("bpb_secperclus = %d\n", bpb->bpb_secperclus);
    printf("bpb_rsvdseccnt = %d\n", bpb->bpb_rsvdseccnt);
    printf("bpb_numfats = %d\n", bpb->bpb_numfats);
    printf("bpb_rootentcnt = %d\n", bpb->bpb_rootentcnt);
    printf("bpb_totsec16 = %d\n", bpb->bpb_totsec16);
    printf("bpb_media = %02x\n", bpb->bpb_media);
    printf("bpb_fatsz16 = %d\n", bpb->bpb_fatsz16);
    printf("bpb_secpertrk = %d\n", bpb->bpb_secpertrk);
    printf("bpb_numheads = %d\n", bpb->bpb_numheads);
    printf("bpb_hiddsec = %u\n", bpb->bpb_hiddsec);
    printf("bpb_totsec32 = %u\n", bpb->bpb_totsec32);

    printf("bs_drvnum = %d\n", bpb->bs_drvnum);
    printf("bs_bootsig = %02x\n", bpb->bs_bootsig);
    if(bpb->bs_bootsig == 0x29)
    {
       printf("bs_volid = %xl\n", bpb->bs_volid);
       printf("bs_vollab = \"%s\"\n", bpb->bs_vollab);
       printf("bs_filsystype = \"%s\"\n", bpb->bs_filsystype);
    }

    printf("bpb_fatsz32 = %u\n", bpb->bpb_fatsz32);
    printf("last_word = %04x\n", bpb->last_word);

    printf("fat_type = FAT32\n");
}

void dbg_dir(char* currdir)
{
    DIR* dir;
    struct dirent* entry;

    dir = opendir(currdir);
    if (dir)
    {
        for ( entry = readdir(dir); 
              entry; 
              entry = readdir(dir) )
        {
            printf("%s (%08x)\n", entry->d_name, entry->size);
        }
    }
    else
    {
        fprintf(stderr, "Could not open dir %s\n", currdir);
    }
    closedir(dir);
}

void dbg_type(int cluster)
{
    unsigned char buf[SECTOR_SIZE*5];
    struct fat_file ent;
    int i;

    fat_open(cluster,&ent);
    
    for (i=0;i<5;i++)
        if(fat_read(&ent, 1, buf) >= 0)
        {
            buf[SECTOR_SIZE]=0;
            printf("%s\n", buf);
        }
        else
        {
            fprintf(stderr, "Could not read file on cluster %d\n", cluster);
        }
}

char current_directory[256] = "\\";
int last_secnum = 0;

void dbg_prompt(void)
{
    printf("C:%s> ", current_directory);
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
                    printf("secnum: %d\n", last_secnum);
                    dbg_dump_sector(last_secnum);
                    continue;
                }

                if(!strcasecmp(s, "type"))
                {
                    int cluster = 0;
                    if((s = strtok(NULL, " \n")))
                    {
                        cluster = atoi(s);
                    }
                    dbg_type(cluster);
                    continue;
                }
            
                if(!strcasecmp(s, "exit") ||
                   !strcasecmp(s, "x"))
                {
                    quit = 1;
                }
            }
        }
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

