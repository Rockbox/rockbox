/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by William Poetra Yoga Hadisoeseno 
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

void usage()
{
    fprintf(stderr, "usage: IHFSsplit <ihfs_img> <output_dir>\n");
    exit(1);
}

typedef struct {
    uint32_t signature;
    uint32_t fslen;
    uint32_t unknown1;
    uint32_t unknown2;
    char timestamp[12];
    uint32_t numfiles;
    char zeros[476];
    uint32_t marker;
} ihfs_header_t;

#define MAX_FILES 2048
#define MAX_IHFS_PATH 56

typedef struct {
    struct {
        char fullpath[MAX_IHFS_PATH];
        uint32_t sector;
        uint32_t length;
    } files[MAX_FILES];
} ihfs_file_table_t;

#define SECTOR_SIZE 512

int ihfs_sanity(const int ihfs_img)
{
    struct stat statbuf;
    ihfs_header_t ihfs_hdr;

    printf("starting sanity check for IHFS image...\n");

    lseek(ihfs_img, 0, SEEK_SET);
    read(ihfs_img, &ihfs_hdr, sizeof (ihfs_hdr));

    printf("  checking for IHFS signature...\n");
    if (ihfs_hdr.signature != 0x49484653)
        return 1;

    printf("  checking for FS length...\n");
    fstat(ihfs_img, &statbuf);
    if (ihfs_hdr.fslen * SECTOR_SIZE != statbuf.st_size)
        return 1;

    printf("  checking for unknown value 1...\n");
    if (ihfs_hdr.unknown1 != 0x00000004)
        return 1;

    printf("  checking for unknown value 2...\n");
    if (ihfs_hdr.unknown2 != 0xfffff000)
        return 1;

    printf("  checking for number of files...\n");
    if (ihfs_hdr.numfiles > MAX_FILES)
        return 1;

    printf("  checking for marker...\n");
    if (ihfs_hdr.marker != 0x55aa55aa)
        return 1;

    return 0;
}

void mkdir_p(const char *path)
{
    char *dir;

    dir = dirname(strdup(path));
    if (strchr(dir, '/'))
        mkdir_p(dir);

    mkdir(dir, 0755);
}

#define BUF_SIZE 4096

void outputfile(const char *outpath, const int ihfs_img, const int offset, const int length)
{
    int outfd;
    int i, rem;
    char buf[BUF_SIZE];

    lseek(ihfs_img, offset, SEEK_SET);

    outfd = creat(outpath, 0644);

    for (i = 0; i < length / BUF_SIZE; ++i) {
        read(ihfs_img, buf, BUF_SIZE);
        write(outfd, buf, BUF_SIZE);
    }
    rem = length - i * BUF_SIZE;
    if (rem > 0) {
        read(ihfs_img, buf, rem);
        write(outfd, buf, rem);
    }

    close(outfd);
}

int main(int argc, char **argv)
{
    struct stat statbuf;
    int ihfs_img;
    ihfs_header_t ihfs_hdr;
    ihfs_file_table_t ihfs_ftbl;
    int i, j;
    char *outpath, *base_path, ihfs_path[MAX_IHFS_PATH+1];

    /* check the arguments */

    if (argc != 3)
        usage();

    stat(argv[1], &statbuf);
    if (!S_ISREG(statbuf.st_mode))
        usage();

    stat(argv[2], &statbuf);
    if (!S_ISDIR(statbuf.st_mode))
        usage();

    /* check the file, then split */

    ihfs_img = open(argv[1], O_RDONLY);

    if (ihfs_sanity(ihfs_img)) {
        printf("Non-IHFS format!\n");
        return 1;
    } else
        printf("sanity check OK\n");

    lseek(ihfs_img, 0, SEEK_SET);
    read(ihfs_img, &ihfs_hdr, sizeof (ihfs_hdr));
    lseek(ihfs_img, 4 * SECTOR_SIZE, SEEK_SET);
    read(ihfs_img, &ihfs_ftbl, sizeof (ihfs_ftbl));

    base_path = strdup(argv[2]);
    outpath = malloc(strlen(base_path) + 1 + MAX_IHFS_PATH + 1);
    for (i = 0; i < ihfs_hdr.numfiles; ++i) {
        printf("\n");
        printf("pathname: %s\n", ihfs_ftbl.files[i].fullpath);
        printf("starts at sector %d, length is %d bytes\n", ihfs_ftbl.files[i].sector, ihfs_ftbl.files[i].length);

        strncpy(ihfs_path, ihfs_ftbl.files[i].fullpath, MAX_IHFS_PATH);
        ihfs_path[MAX_IHFS_PATH] = '\0';
        for (j = 0; j < strlen(ihfs_path); ++j)
            if (ihfs_path[j] == '\\')
                ihfs_path[j] = '/';

        sprintf(outpath, "%s/%s", base_path, ihfs_path);
        mkdir_p(outpath);
        outputfile(outpath, ihfs_img, ihfs_ftbl.files[i].sector * SECTOR_SIZE, ihfs_ftbl.files[i].length);
    }
    free(outpath);

    close(ihfs_img);

    return 0;
}
