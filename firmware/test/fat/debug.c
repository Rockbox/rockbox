#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "ata.h"
#include "debug.h"

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

    for(i = 0;i < 512/32;i++)
    {
        for(j = 0;j < 32;j++)
        {
            c = buf[i*32+j];

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

    switch(bpb->fat_type)
    {
    case FATTYPE_FAT12:
       printf("fat_type = FAT12\n");
       break;
    case FATTYPE_FAT16:
       printf("fat_type = FAT16\n");
       break;
    case FATTYPE_FAT32:
       printf("fat_type = FAT32\n");
       break;
    default:
       printf("fat_type = UNKNOWN (%d)\n", bpb->fat_type);
       break;
    }
}

void dbg_dir(struct bpb *bpb, int currdir)
{
    struct fat_dirent dent;
    struct fat_direntry de;

    if(fat_opendir(bpb, &dent, currdir) >= 0)
    {
        while(fat_getnext(bpb, &dent, &de) >= 0)
        {
            printf("%s\n", de.name);
        }
    }
    else
    {
        fprintf(stderr, "Could not read dir on cluster %d\n", currdir);
    }
}

extern char current_directory[];
int last_secnum = 0;

void dbg_prompt(void)
{
    printf("C:%s> ", current_directory);
}

void dbg_console(struct bpb* bpb)
{
    char cmd[32] = "";
    char last_cmd[32] = "";
    int quit = 0;
    char *s;
    int secnum;

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
                    secnum = 0;
                    if((s = strtok(NULL, " \n")))
                    {
                        secnum = atoi(s);
                    }
                    dbg_dir(bpb, secnum);
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
            
                if(!strcasecmp(s, "exit") ||
                   !strcasecmp(s, "x"))
                {
                    quit = 1;
                }
            }
        }
    }
}
