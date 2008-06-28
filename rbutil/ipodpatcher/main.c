/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ipodpatcher.c 12237 2007-02-08 21:31:38Z dave $
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

#include "ipodpatcher.h"
#include "ipodio.h"

#define VERSION "2.0 with v2.0 bootloaders"

enum {
   NONE,
#ifdef WITH_BOOTOBJS
   INSTALL,
#endif
   INTERACTIVE,
   SHOW_INFO,
   LIST_IMAGES,
   DELETE_BOOTLOADER,
   ADD_BOOTLOADER,
   READ_FIRMWARE,
   WRITE_FIRMWARE,
   READ_AUPD,
   WRITE_AUPD,
   READ_PARTITION,
   WRITE_PARTITION,
   FORMAT_PARTITION,
   CONVERT_TO_FAT32
};

void print_macpod_warning(void)
{
    printf("[INFO] ************************************************************************\n");
    printf("[INFO] *** WARNING FOR ROCKBOX USERS\n");
    printf("[INFO] *** You must convert this ipod to FAT32 format (aka a \"winpod\")\n");
    printf("[INFO] *** if you want to run Rockbox.  Rockbox WILL NOT work on this ipod.\n");
    printf("[INFO] *** See http://www.rockbox.org/twiki/bin/view/Main/IpodConversionToFAT32\n");
    printf("[INFO] ************************************************************************\n");
}

void print_usage(void)
{
    fprintf(stderr,"Usage: ipodpatcher --scan\n");
#ifdef __WIN32__
    fprintf(stderr,"    or ipodpatcher [DISKNO] [action]\n");
#else
    fprintf(stderr,"    or ipodpatcher [device] [action]\n");
#endif
    fprintf(stderr,"\n");
    fprintf(stderr,"Where [action] is one of the following options:\n");
#ifdef WITH_BOOTOBJS
    fprintf(stderr,"        --install\n");
#endif
    fprintf(stderr,"  -l,   --list\n");
    fprintf(stderr,"  -r,   --read-partition     bootpartition.bin\n");
    fprintf(stderr,"  -w,   --write-partition    bootpartition.bin\n");
    fprintf(stderr,"  -rf,  --read-firmware      filename.ipod\n");
    fprintf(stderr,"  -rfb, --read-firmware-bin  filename.bin\n");
    fprintf(stderr,"  -wf,  --write-firmware     filename.ipod\n");
    fprintf(stderr,"  -wfb, --write-firmware-bin filename.bin\n");
#ifdef WITH_BOOTOBJS
    fprintf(stderr,"  -we,  --write-embedded\n");
#endif
    fprintf(stderr,"  -a,   --add-bootloader     filename.ipod\n");
    fprintf(stderr,"  -ab,  --add-bootloader-bin filename.bin\n");
    fprintf(stderr,"  -d,   --delete-bootloader\n");
    fprintf(stderr,"  -f,   --format\n");
    fprintf(stderr,"  -c,   --convert\n");
    fprintf(stderr,"        --read-aupd          filename.bin\n");
    fprintf(stderr,"        --write-aupd         filename.bin\n");
    fprintf(stderr,"\n");

#ifdef __WIN32__
    fprintf(stderr,"DISKNO is the number (e.g. 2) Windows has assigned to your ipod's hard disk.\n");
    fprintf(stderr,"The first hard disk in your computer (i.e. C:\\) will be disk 0, the next disk\n");
    fprintf(stderr,"will be disk 1 etc.  ipodpatcher will refuse to access a disk unless it\n");
    fprintf(stderr,"can identify it as being an ipod.\n");
    fprintf(stderr,"\n");
#else
#if defined(linux) || defined (__linux)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/sda) assigned to your ipod.\n");
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/da1) assigned to your ipod.\n");
#elif defined(__APPLE__) && defined(__MACH__)
    fprintf(stderr,"\"device\" is the device node (e.g. /dev/disk1) assigned to your ipod.\n");
#endif
    fprintf(stderr,"ipodpatcher will refuse to access a disk unless it can identify it as being\n");
    fprintf(stderr,"an ipod.\n");
#endif
}

void display_partinfo(struct ipod_t* ipod)
{
    int i;
    double sectors_per_MB = (1024.0*1024.0)/ipod->sector_size;

    printf("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n");
    for ( i = 0; i < 4; i++ ) {
        if (ipod->pinfo[i].start != 0) {
            printf("[INFO]    %d      %10ld    %10ld  %10.1f   %s (0x%02x)\n",
                   i,
                   (long int)ipod->pinfo[i].start,
                   (long int)ipod->pinfo[i].start+ipod->pinfo[i].size-1,
                   ipod->pinfo[i].size/sectors_per_MB,
                   get_parttype(ipod->pinfo[i].type),
                   (int)ipod->pinfo[i].type);
        }
    }
}


int main(int argc, char* argv[])
{
    char yesno[4];
    int i;
    int n;
    int infile, outfile;
    unsigned int inputsize;
    char* filename;
    int action = SHOW_INFO;
    int type;
    struct ipod_t ipod;

    fprintf(stderr,"ipodpatcher v" VERSION " - (C) Dave Chapman 2006-2007\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    if ((argc > 1) && ((strcmp(argv[1],"-h")==0) || (strcmp(argv[1],"--help")==0))) {
        print_usage();
        return 1;
    }

    if (ipod_alloc_buffer(&ipod_sectorbuf,BUFFER_SIZE) < 0) {
        fprintf(stderr,"Failed to allocate memory buffer\n");
    }

    if ((argc > 1) && (strcmp(argv[1],"--scan")==0)) {
        if (ipod_scan(&ipod) == 0)
            fprintf(stderr,"[ERR]  No ipods found.\n");
        return 0;
    }

    /* If the first parameter doesn't start with -, then we interpret it as a device */
    if ((argc > 1) && (argv[1][0] != '-')) {
        ipod.diskname[0]=0;
#ifdef __WIN32__
        snprintf(ipod.diskname,sizeof(ipod.diskname),"\\\\.\\PhysicalDrive%s",argv[1]);
#else
        strncpy(ipod.diskname,argv[1],sizeof(ipod.diskname));
#endif
        i = 2;
    } else {
        /* Autoscan for ipods */
        n = ipod_scan(&ipod);
        if (n==0) {
            fprintf(stderr,"[ERR]  No ipods found, aborting\n");
            fprintf(stderr,"[ERR]  Please connect your ipod and ensure it is in disk mode\n");
#if defined(__APPLE__) && defined(__MACH__)
            fprintf(stderr,"[ERR]  Also ensure that itunes is closed, and that your ipod is not mounted.\n");
#elif !defined(__WIN32__)
            if (geteuid()!=0) {
                fprintf(stderr,"[ERR]  You may also need to run ipodpatcher as root.\n");
            }
#endif
            fprintf(stderr,"[ERR]  Please refer to the Rockbox manual if you continue to have problems.\n");
        } else if (n > 1) {
            fprintf(stderr,"[ERR]  %d ipods found, aborting\n",n);
            fprintf(stderr,"[ERR]  Please connect only one ipod and re-run ipodpatcher.\n");
        }

        if (n != 1) {
#ifdef WITH_BOOTOBJS
            if (argc==1) {
                printf("\nPress ENTER to exit ipodpatcher :");
                fgets(yesno,4,stdin);
            }
#endif
            return 0;
        }

        i = 1;
    }

#ifdef WITH_BOOTOBJS
    action = INTERACTIVE;
#else
    action = NONE;
#endif

    while (i < argc) {
        if ((strcmp(argv[i],"-l")==0) || (strcmp(argv[i],"--list")==0)) {
            action = LIST_IMAGES;
            i++;
#ifdef WITH_BOOTOBJS
        } else if (strcmp(argv[i],"--install")==0) {
            action = INSTALL;
            i++;
#endif
        } else if ((strcmp(argv[i],"-d")==0) || 
                   (strcmp(argv[i],"--delete-bootloader")==0)) {
            action = DELETE_BOOTLOADER;
            i++;
        } else if ((strcmp(argv[i],"-a")==0) || 
                   (strcmp(argv[i],"--add-bootloader")==0)) {
            action = ADD_BOOTLOADER;
            type = FILETYPE_DOT_IPOD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-ab")==0) || 
                   (strcmp(argv[i],"--add-bootloader-bin")==0)) {
            action = ADD_BOOTLOADER;
            type = FILETYPE_DOT_BIN;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-rf")==0) || 
                   (strcmp(argv[i],"--read-firmware")==0)) {
            action = READ_FIRMWARE;
            type = FILETYPE_DOT_IPOD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-rfb")==0) || 
                   (strcmp(argv[i],"--read-firmware-bin")==0)) {
            action = READ_FIRMWARE;
            type = FILETYPE_DOT_BIN;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
#ifdef WITH_BOOTOBJS
        } else if ((strcmp(argv[i],"-we")==0) || 
                   (strcmp(argv[i],"--write-embedded")==0)) {
            action = WRITE_FIRMWARE;
            type = FILETYPE_INTERNAL;
            filename="[embedded bootloader]";  /* Only displayed for user */
            i++;
#endif
        } else if ((strcmp(argv[i],"-wf")==0) || 
                   (strcmp(argv[i],"--write-firmware")==0)) {
            action = WRITE_FIRMWARE;
            type = FILETYPE_DOT_IPOD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-wfb")==0) || 
                   (strcmp(argv[i],"--write-firmware-bin")==0)) {
            action = WRITE_FIRMWARE;
            type = FILETYPE_DOT_BIN;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-r")==0) || 
                   (strcmp(argv[i],"--read-partition")==0)) {
            action = READ_PARTITION;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-w")==0) || 
                   (strcmp(argv[i],"--write-partition")==0)) {
            action = WRITE_PARTITION;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-v")==0) || 
                   (strcmp(argv[i],"--verbose")==0)) {
            ipod_verbose++;
            i++;
        } else if ((strcmp(argv[i],"-f")==0) || 
                   (strcmp(argv[i],"--format")==0)) {
            action = FORMAT_PARTITION;
            i++;
        } else if (strcmp(argv[i],"--read-aupd")==0) {
            action = READ_AUPD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if (strcmp(argv[i],"--write-aupd")==0) {
            action = WRITE_AUPD;
            i++;
            if (i == argc) { print_usage(); return 1; }
            filename=argv[i];
            i++;
        } else if ((strcmp(argv[i],"-c")==0) || 
                   (strcmp(argv[i],"--convert")==0)) {
            action = CONVERT_TO_FAT32;
            i++;
        } else {
            print_usage(); return 1;
        }
    }

    if (ipod.diskname[0]==0) {
        print_usage();
        return 1;
    }

    if (ipod_open(&ipod, 0) < 0) {
        return 1;
    }

    fprintf(stderr,"[INFO] Reading partition table from %s\n",ipod.diskname);
    fprintf(stderr,"[INFO] Sector size is %d bytes\n",ipod.sector_size);

    if (read_partinfo(&ipod,0) < 0) {
        return 2;
    }

    display_partinfo(&ipod);

    if (ipod.pinfo[0].start==0) {
        fprintf(stderr,"[ERR]  No partition 0 on disk:\n");
        display_partinfo(&ipod);
        return 3;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0) {
        fprintf(stderr,"[ERR]  Failed to read firmware directory - nimages=%d\n",ipod.nimages);
        return 1;
    }

    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0) {
        fprintf(stderr,"[ERR] Unknown version number in firmware (%08x)\n",
                       ipod.ipod_directory[0].vers);
        return -1;
    }

    printf("[INFO] Ipod model: %s (\"%s\")\n",ipod.modelstr,
           ipod.macpod ? "macpod" : "winpod");

    if (ipod.ipod_directory[0].vers == 0x10000) {
        fprintf(stderr,"[ERR]  *** ipodpatcher does not support the 2nd Generation Nano! ***\n");
#ifdef WITH_BOOTOBJS
        printf("Press ENTER to exit ipodpatcher :");
        fgets(yesno,4,stdin);
#endif
        return 0;
    }

    if (ipod.macpod) {
        print_macpod_warning();
    }
  
    if (action==LIST_IMAGES) {
        list_images(&ipod);
#ifdef WITH_BOOTOBJS
    } else if (action==INTERACTIVE) {

        printf("Enter i to install the Rockbox bootloader, u to uninstall\n or c to cancel and do nothing (i/u/c) :");

        if (fgets(yesno,4,stdin)) {
            if (yesno[0]=='i') {
                if (ipod_reopen_rw(&ipod) < 0) {
                    return 5;
                }

                if (add_bootloader(&ipod, NULL, FILETYPE_INTERNAL)==0) {
                    fprintf(stderr,"[INFO] Bootloader installed successfully.\n");
                } else {
                    fprintf(stderr,"[ERR]  --install failed.\n");
                }
            } else if (yesno[0]=='u') {
                if (ipod_reopen_rw(&ipod) < 0) {
                    return 5;
                }

                if (delete_bootloader(&ipod)==0) {
                    fprintf(stderr,"[INFO] Bootloader removed.\n");
                } else {
                    fprintf(stderr,"[ERR]  Bootloader removal failed.\n");
                }
            }
        }
#endif
    } else if (action==DELETE_BOOTLOADER) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        if (ipod.ipod_directory[0].entryOffset==0) {
            fprintf(stderr,"[ERR]  No bootloader detected.\n");
        } else {
            if (delete_bootloader(&ipod)==0) {
                fprintf(stderr,"[INFO] Bootloader removed.\n");
            } else {
                fprintf(stderr,"[ERR]  --delete-bootloader failed.\n");
            }
        }
    } else if (action==ADD_BOOTLOADER) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        if (add_bootloader(&ipod, filename, type)==0) {
            fprintf(stderr,"[INFO] Bootloader %s written to device.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --add-bootloader failed.\n");
        }
#ifdef WITH_BOOTOBJS
    } else if (action==INSTALL) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        if (add_bootloader(&ipod, NULL, FILETYPE_INTERNAL)==0) {
            fprintf(stderr,"[INFO] Bootloader installed successfully.\n");
        } else {
            fprintf(stderr,"[ERR]  --install failed.\n");
        }
#endif
    } else if (action==WRITE_FIRMWARE) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        if (write_firmware(&ipod, filename,type)==0) {
            fprintf(stderr,"[INFO] Firmware %s written to device.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --write-firmware failed.\n");
        }
    } else if (action==READ_FIRMWARE) {
        if (read_firmware(&ipod, filename, type)==0) {
            fprintf(stderr,"[INFO] Firmware read to file %s.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --read-firmware failed.\n");
        }
    } else if (action==READ_AUPD) {
        if (read_aupd(&ipod, filename)==0) {
            fprintf(stderr,"[INFO] AUPD image read to file %s.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --read-aupd failed.\n");
        }
    } else if (action==WRITE_AUPD) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        if (write_aupd(&ipod, filename)==0) {
            fprintf(stderr,"[INFO] AUPD image %s written to device.\n",filename);
        } else {
            fprintf(stderr,"[ERR]  --write-aupd failed.\n");
        }
    } else if (action==READ_PARTITION) {
        outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,S_IREAD|S_IWRITE);
        if (outfile < 0) {
           perror(filename);
           return 4;
        }

        if (read_partition(&ipod, outfile) < 0) {
            fprintf(stderr,"[ERR]  --read-partition failed.\n");
        } else {
            fprintf(stderr,"[INFO] Partition extracted to %s.\n",filename);
        }
        close(outfile);
    } else if (action==WRITE_PARTITION) {
        if (ipod_reopen_rw(&ipod) < 0) {
            return 5;
        }

        infile = open(filename,O_RDONLY|O_BINARY);
        if (infile < 0) {
            perror(filename);
            return 2;
        }

        /* Check filesize is <= partition size */
        inputsize=filesize(infile);
        if (inputsize > 0) {
            if (inputsize <= (ipod.pinfo[0].size*ipod.sector_size)) {
                fprintf(stderr,"[INFO] Input file is %u bytes\n",inputsize);
                if (write_partition(&ipod,infile) < 0) {
                    fprintf(stderr,"[ERR]  --write-partition failed.\n");
                } else {
                    fprintf(stderr,"[INFO] %s restored to partition\n",filename);
                }
            } else {
                fprintf(stderr,"[ERR]  File is too large for firmware partition, aborting.\n");
            }
        }

        close(infile);
    } else if (action==FORMAT_PARTITION) {
        printf("WARNING!!! YOU ARE ABOUT TO USE AN EXPERIMENTAL FEATURE.\n");
        printf("ALL DATA ON YOUR IPOD WILL BE ERASED.\n");
        printf("Are you sure you want to format your ipod? (y/n):");
        
        if (fgets(yesno,4,stdin)) {
            if (yesno[0]=='y') {
                if (ipod_reopen_rw(&ipod) < 0) {
                    return 5;
                }

                if (format_partition(&ipod,1) < 0) {
                    fprintf(stderr,"[ERR]  Format failed.\n");
                }
            } else {
                fprintf(stderr,"[INFO] Format cancelled.\n");
            }
        }
    } else if (action==CONVERT_TO_FAT32) {
        if (!ipod.macpod) {
            printf("[ERR]  Ipod is already FAT32, aborting\n");
        } else {
            printf("WARNING!!! YOU ARE ABOUT TO USE AN EXPERIMENTAL FEATURE.\n");
            printf("ALL DATA ON YOUR IPOD WILL BE ERASED.\n");
            printf("Are you sure you want to convert your ipod to FAT32? (y/n):");
        
            if (fgets(yesno,4,stdin)) {
                if (yesno[0]=='y') {
                    if (ipod_reopen_rw(&ipod) < 0) {
                        return 5;
                    }

                    if (write_dos_partition_table(&ipod) < 0) {
                        fprintf(stderr,"[ERR]  Partition conversion failed.\n");
                    }

                    if (format_partition(&ipod,1) < 0) {
                        fprintf(stderr,"[ERR]  Format failed.\n");
                    }
                } else {
                    fprintf(stderr,"[INFO] Format cancelled.\n");
                }
            }
        }
    }

    ipod_close(&ipod);

#ifdef WITH_BOOTOBJS
    if (action==INTERACTIVE) {
        printf("Press ENTER to exit ipodpatcher :");
        fgets(yesno,4,stdin);
    }
#endif


    return 0;
}
