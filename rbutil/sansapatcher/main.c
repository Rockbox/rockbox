/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sansapatcher.h"
#include "sansaio.h"
#include "parttypes.h"

#define VERSION "0.6 with v4.0 bootloaders"

enum {
   NONE,
   INSTALL,
   INTERACTIVE,
   SHOW_INFO,
   LIST_IMAGES,
   DELETE_BOOTLOADER,
   ADD_BOOTLOADER,
   READ_FIRMWARE,
   WRITE_FIRMWARE,
   READ_PARTITION,
   WRITE_PARTITION,
   UPDATE_OF,
   UPDATE_PPBL
};

static void print_usage(void)
{
    fprintf(stderr,"Usage: sansapatcher --scan\n");
#ifdef __WIN32__
    fprintf(stderr,"    or sansapatcher [DISKNO] [action]\n");
#else
    fprintf(stderr,"    or sansapatcher [device] [action]\n");
#endif
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
    fprintf(stderr,"        --install\n");
    fprintf(stderr,"  -l,   --list\n");
    fprintf(stderr,"  -rf,  --read-firmware             filename.mi4\n");
    fprintf(stderr,"  -a,   --add-bootloader            filename.mi4\n");
    fprintf(stderr,"  -d,   --delete-bootloader\n");
    fprintf(stderr,"  -of   --update-original-firmware  filename.mi4\n");
    fprintf(stderr,"  -bl   --update-ppbl               filename.bin\n");
    fprintf(stderr,"\n");

#ifdef __WIN32__
    fprintf(stderr,"DISKNO is the number (e.g. 2) Windows has assigned to your sansa's hard disk.\n");
    fprintf(stderr,"The first hard disk in your computer (i.e. C:\\) will be disk 0, the next disk\n");
    fprintf(stderr,"will be disk 1 etc.  sansapatcher will refuse to access a disk unless it\n");
    fprintf(stderr,"can identify it as being an E200 or C200.\n");
    fprintf(stderr,"\n");
#else
#if defined(linux) || defined (__linux)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/sda) assigned to your sansa.\n");
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/da1) assigned to your sansa.\n");
#elif defined(__APPLE__) && defined(__MACH__)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/disk1) assigned to your sansa.\n");
#endif
    fprintf(stderr,"sansapatcher will refuse to access a disk unless it can identify it as being\n");
    fprintf(stderr,"an E200 or C200.\n");
#endif
}

static const char* get_parttype(int pt)
{
    int i;
    static const char unknown[]="Unknown";

    if (pt == -1) {
        return "HFS/HFS+";
    }

    i=0;
    while (parttypes[i].name != NULL) {
        if (parttypes[i].type == pt) {
            return (parttypes[i].name);
        }
        i++;
    }

    return unknown;
}

static void display_partinfo(struct sansa_t* sansa)
{
    int i;
    double sectors_per_MB = (1024.0*1024.0)/sansa->sector_size;

    printf("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n");
    for ( i = 0; i < 4; i++ ) {
        if (sansa->pinfo[i].start != 0) {
            printf("[INFO]    %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n",
                   i,
                   sansa->pinfo[i].start,
                   sansa->pinfo[i].start+sansa->pinfo[i].size-1,
                   sansa->pinfo[i].size/sectors_per_MB,
                   get_parttype(sansa->pinfo[i].type),
                   sansa->pinfo[i].type);
        }
    }
}


int main(int argc, char* argv[])
{
    char yesno[4];
    int i;
    int n;
    char* filename;
    int action = SHOW_INFO;
    int type;
    struct sansa_t sansa;
    int res = 0;

    fprintf(stderr,"sansapatcher v" VERSION " - (C) Dave Chapman 2006-2007\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if ((argc > 1) && ((strcmp(argv[1],"-h")==0) || (strcmp(argv[1],"--help")==0))) {
        print_usage();
        return 1;
    }

    if (sansa_alloc_buffer(&sansa_sectorbuf,BUFFER_SIZE) < 0) {
        fprintf(stderr,"Failed to allocate memory buffer\n");
    }

    if ((argc > 1) && (strcmp(argv[1],"--scan")==0)) {
        if (sansa_scan(&sansa) == 0)
            fprintf(stderr,"[ERR]  No E200s or C200s found.\n");
        return 0;
    }

    /* If the first parameter doesn't start with -, then we interpret it as a device */
    if ((argc > 1) && (argv[1][0] != '-')) {
        sansa.diskname[0]=0;
#ifdef __WIN32__
        snprintf(sansa.diskname,sizeof(sansa.diskname),"\\\\.\\PhysicalDrive%s",argv[1]);
#else
        strncpy(sansa.diskname,argv[1],sizeof(sansa.diskname));
#endif
        i = 2;
    } else {
        /* Autoscan for C200/E200s */
        n = sansa_scan(&sansa);
        if (n==0) {
            fprintf(stderr,"[ERR]  No E200s or C200s found, aborting\n");
            fprintf(stderr,"[ERR]  Please connect your sansa and ensure it is in UMS mode\n");
#if defined(__APPLE__) && defined(__MACH__)
            fprintf(stderr,"[ERR]  Also ensure that your Sansa's main partition is not mounted.\n");
#elif !defined(__WIN32__)
            if (geteuid()!=0) {
                fprintf(stderr,"[ERR]  You may also need to run sansapatcher as root.\n");
            }
#endif
            fprintf(stderr,"[ERR]  Please refer to the Rockbox manual if you continue to have problems.\n");
        } else if (n > 1) {
            fprintf(stderr,"[ERR]  %d Sansas found, aborting\n",n);
            fprintf(stderr,"[ERR]  Please connect only one Sansa and re-run sansapatcher.\n");
        }

        if (n != 1) {
            if (argc==1) {
                printf("\nPress ENTER to exit sansapatcher :");
                fgets(yesno,4,stdin);
            }
            return 0;
        }

        i = 1;
    }

    action = INTERACTIVE;

    while (i < argc) {
        if ((strcmp(argv[i],"-l")==0) || (strcmp(argv[i],"--list")==0)) {
            action = LIST_IMAGES;
            i++;
        } else if (strcmp(argv[i],"--install")==0) {
            action = INSTALL;
            i++;
        } else if ((strcmp(argv[i],"-d")==0) || 
                   (strcmp(argv[i],"--delete-bootloader")==0)) {
            action = DELETE_BOOTLOADER;
            i++;
        } else if ((strcmp(argv[i],"-a")==0) || 
                   (strcmp(argv[i],"--add-bootloader")==0)) {
            action = ADD_BOOTLOADER;
            type = FILETYPE_MI4;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-of")==0) || 
                   (strcmp(argv[i],"--update-original-firmware")==0)) {
            action = UPDATE_OF;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-bl")==0) || 
                   (strcmp(argv[i],"--update-ppbl")==0)) {
            action = UPDATE_PPBL;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-rf")==0) || 
                   (strcmp(argv[i],"--read-firmware")==0)) {
            action = READ_FIRMWARE;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        }
    }

    if (sansa.diskname[0]==0) {
        print_usage();
        return 1;
    }

    if (sansa_open(&sansa, 0) < 0) {
        return 1;
    }

    fprintf(stderr,"[INFO] Reading partition table from %s\n",sansa.diskname);
    fprintf(stderr,"[INFO] Sector size is %d bytes\n",sansa.sector_size);

    if (sansa_read_partinfo(&sansa,0) < 0) {
        return 2;
    }

    display_partinfo(&sansa);

    i = is_sansa(&sansa);
    if (i < 0) {
        fprintf(stderr,"[ERR]  Disk is not an E200 or C200 (%d), aborting.\n",i);
        return 3;
    }

    if (sansa.hasoldbootloader) {
        printf("[ERR]  ************************************************************************\n");
        printf("[ERR]  *** OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n");
        printf("[ERR]  *** You must reinstall the original Sansa firmware before running\n");
        printf("[ERR]  *** sansapatcher for the first time.\n");
        printf("[ERR]  *** See http://www.rockbox.org/twiki/bin/view/Main/SansaE200Install\n");
        printf("[ERR]  ************************************************************************\n");
        res = 4;
    } else {
        if (action==LIST_IMAGES) {
            sansa_list_images(&sansa);
        } else if (action==INTERACTIVE) {

            printf("Enter i to install the Rockbox bootloader, u to uninstall\n or c to cancel and do nothing (i/u/c) :");

            if (fgets(yesno,4,stdin)) {
                if (yesno[0]=='i') {
                    if (sansa_reopen_rw(&sansa) < 0) {
                        res = 5;
                    }

                    if (sansa_add_bootloader(&sansa, NULL, FILETYPE_INTERNAL)==0) {
                        fprintf(stderr,"[INFO] Bootloader installed successfully.\n");
                    } else {
                        fprintf(stderr,"[ERR]  --install failed.\n");
                        res = 6;
                    }
                } else if (yesno[0]=='u') {
                    if (sansa_reopen_rw(&sansa) < 0) {
                        res = 5;
                    }

                    if (sansa_delete_bootloader(&sansa)==0) {
                        fprintf(stderr,"[INFO] Bootloader removed.\n");
                    } else {
                        fprintf(stderr,"[ERR]  Bootloader removal failed.\n");
                        res = 7;
                    }
                }
            }
        } else if (action==READ_FIRMWARE) {
            if (sansa_read_firmware(&sansa, filename)==0) {
                fprintf(stderr,"[INFO] Firmware read to file %s.\n",filename);
            } else {
                fprintf(stderr,"[ERR]  --read-firmware failed.\n");
            }
        } else if (action==INSTALL) {
            if (sansa_reopen_rw(&sansa) < 0) {
                return 5;
            }

            if (sansa_add_bootloader(&sansa, NULL, FILETYPE_INTERNAL)==0) {
                fprintf(stderr,"[INFO] Bootloader installed successfully.\n");
            } else {
                fprintf(stderr,"[ERR]  --install failed.\n");
            }
        } else if (action==ADD_BOOTLOADER) {
            if (sansa_reopen_rw(&sansa) < 0) {
                return 5;
            }

            if (sansa_add_bootloader(&sansa, filename, type)==0) {
                fprintf(stderr,"[INFO] Bootloader %s written to device.\n",filename);
            } else {
                fprintf(stderr,"[ERR]  --add-bootloader failed.\n");
            }
        } else if (action==DELETE_BOOTLOADER) {
            if (sansa_reopen_rw(&sansa) < 0) {
                return 5;
            }

            if (sansa_delete_bootloader(&sansa)==0) {
                fprintf(stderr,"[INFO] Bootloader removed successfully.\n");
            } else {
                fprintf(stderr,"[ERR]  --delete-bootloader failed.\n");
            }
        } else if (action==UPDATE_OF) {
            if (sansa_reopen_rw(&sansa) < 0) {
                return 5;
            }

            if (sansa_update_of(&sansa, filename)==0) {
                fprintf(stderr,"[INFO] OF updated successfully.\n");
            } else {
                fprintf(stderr,"[ERR]  --update-original-firmware failed.\n");
            }
        } else if (action==UPDATE_PPBL) {
            printf("[WARN] PPBL installation will overwrite your bootloader. This will lead to a\n");
            printf("       Sansa that won't boot if the bootloader file is invalid. Only continue if\n");
            printf("       you're sure you know what you're doing.\n");
            printf("       Continue (y/n)? ");

            if (fgets(yesno,4,stdin)) {
                if (yesno[0]=='y') {
                    if (sansa_reopen_rw(&sansa) < 0) {
                        return 5;
                    }
        
                    if (sansa_update_ppbl(&sansa, filename)==0) {
                        fprintf(stderr,"[INFO] PPBL updated successfully.\n");
                    } else {
                        fprintf(stderr,"[ERR]  --update-ppbl failed.\n");
                    }
                }
            }
        }
    }

    sansa_close(&sansa);

    if (action==INTERACTIVE) {
        printf("Press ENTER to exit sansapatcher :");
        fgets(yesno,4,stdin);
    }

    return res;
}
