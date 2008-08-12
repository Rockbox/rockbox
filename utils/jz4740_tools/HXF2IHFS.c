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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

void usage()
{
    fprintf(stderr, "Usage: HXF2IHFS <hxf_img> <ihfs_img> [ihfs_size]\n");
    exit(1);
}

typedef struct {
    uint32_t signature;
    char unknown1[4];
    char timestamp[12];
    uint32_t length;
    uint32_t unknown2;
    uint32_t unknown3;
    char id[32];
} hxf_header_t;

int hxf_sanity(const int hxf_img)
{
    struct stat statbuf;
    hxf_header_t hxf_hdr;
    int r, len_name, len_file, pos;

    printf("Starting sanity check for HXF image...\n");

    lseek(hxf_img, 0, SEEK_SET);
    read(hxf_img, &hxf_hdr, sizeof (hxf_hdr));

    printf("  Checking for HXF signature...\n");
    if (hxf_hdr.signature != 0x46444157)
        return 1;

    printf("  Checking for unknown value 1...\n");
    if (strncmp(hxf_hdr.unknown1, "0100", 4))
        return 1;

    printf("  Checking for length...\n");
    fstat(hxf_img, &statbuf);
    if (hxf_hdr.length != statbuf.st_size)
        return 1;

    printf("  Checking for unknown value 3...\n");
    if (hxf_hdr.unknown3 != 0x00000000)
        return 1;

    printf("  Checking for id...\n");
    if (memcmp(hxf_hdr.id, "Chinachip PMP firmware V1.0\0\0\0\0\0", 32))
        return 1;

    printf("  Checking for file completeness...\n");
    while (1) {
        r = read(hxf_img, &len_name, 4);
        if (r != 4)
            return 1;
        if (len_name == 0)
            break;

        r = lseek(hxf_img, len_name + 1, SEEK_CUR);
        if (r < 0)
            return 1;

        r = read(hxf_img, &len_file, 4);
        if (r != 4)
            return 1;
        if (len_file <= 0)
            return 1;

        r = lseek(hxf_img, len_file, SEEK_CUR);
        if (r < 0)
            return 1;
    }
    pos = lseek(hxf_img, 0, SEEK_CUR);
    if (pos != statbuf.st_size)
        return 1;

    return 0;
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

#define DEFAULT_IHFS_SIZE 65273856
#define DEADFACE_START 0x02140000


int hxf_ihfs_compatible(int hxf_img)
{
    int n, len_name, len_file;

    lseek(hxf_img, sizeof (hxf_header_t), SEEK_SET);

    n = 0;
    while (1) {
        read(hxf_img, &len_name, 4);
        if (len_name == 0)
            break;
        if (len_name > MAX_IHFS_PATH)
            return 1;
        lseek(hxf_img, len_name + 1, SEEK_CUR);
        read(hxf_img, &len_file, 4);
        lseek(hxf_img, len_file, SEEK_CUR);
        ++n;
    }

    if (n > MAX_FILES)
        return 1;

    return 0;
}

void hxf_to_ihfs(int hxf_img, int ihfs_img, int filesize)
{
    char buf[SECTOR_SIZE];
    char ff[SECTOR_SIZE];
    int i, n, rem;

    n = filesize / SECTOR_SIZE;
    rem = filesize % SECTOR_SIZE;

    for (i = 0; i < n; ++i) {
        read(hxf_img, buf, SECTOR_SIZE);
        write(ihfs_img, buf, SECTOR_SIZE);
    }

    if (rem > 0) {
        read(hxf_img, buf, rem);
        write(ihfs_img, buf, rem);
        /* pad with 0xff */
        memset(ff, 0xff, SECTOR_SIZE-rem);
        write(ihfs_img, ff, SECTOR_SIZE-rem);
    }
}

int main(int argc, char **argv)
{
    struct stat statbuf;
    int hxf_img;
    hxf_header_t hxf_hdr;
    char s[32];
    int i, n;

    int ihfs_img;
    ihfs_header_t ihfs_hdr;
    ihfs_file_table_t ihfs_ftbl;
    int ihfs_len;
    unsigned char deadface[SECTOR_SIZE];
    char ff[SECTOR_SIZE];

    int pathlen;
    char *path;
    char filetype;
    int filesize;
    int pos;

    /* check the arguments */
    if (argc != 3 && argc != 4)
        usage();

    stat(argv[1], &statbuf);
    if (!S_ISREG(statbuf.st_mode))
        usage();

    if (argc == 3) {
        ihfs_len = DEFAULT_IHFS_SIZE;
    } else {
        errno = 0;
        ihfs_len = strtol(argv[3], NULL, 10);
        if (errno != 0 || ihfs_len < 0 || (ihfs_len % SECTOR_SIZE) != 0)
            usage();
    }

    /* check the file */

    hxf_img = open(argv[1], O_RDONLY);

    if (hxf_sanity(hxf_img)) {
        printf("Non-HXF format!\n");
        return 1;
    } else {
        printf("Sanity check OK\n");
    }

    lseek(hxf_img, 0, SEEK_SET);
    read(hxf_img, &hxf_hdr, sizeof (hxf_hdr));

    printf("HXF info:\n");

    printf("  Signature: 0x%08x\n", hxf_hdr.signature);

    strncpy(s, hxf_hdr.unknown1, 4);
    s[4] = '\0';
    printf("  Unknown1: %s\n", s);

    strncpy(s, hxf_hdr.timestamp, 12);
    s[12] = '\0';
    printf("  Timestamp: %s\n", s);

    printf("  File size: %d bytes\n", hxf_hdr.length);
    printf("  Unknown2: 0x%08x\n", hxf_hdr.unknown2);
    printf("  Unknown3: 0x%08x\n", hxf_hdr.unknown3);
    printf("  Identifier: %s\n", hxf_hdr.id);

    if (hxf_ihfs_compatible(hxf_img)) {
        printf("This HXF image can't be converted into IHFS\n");
        return 1;
    } else {
        printf("HXF can be converted into IHFS\n");
    }

    /* initialize IHFS structures */
    ihfs_hdr.signature = 0x49484653;
    ihfs_hdr.fslen = ihfs_len / SECTOR_SIZE;
    ihfs_hdr.unknown1 = 0x00000004;
    ihfs_hdr.unknown2 = 0xfffff000;
    memcpy(ihfs_hdr.timestamp, hxf_hdr.timestamp, 12);
    ihfs_hdr.numfiles = 0;
    memset(ihfs_hdr.zeros, 0, 476);
    ihfs_hdr.marker = 0x55aa55aa;

    memset(&ihfs_ftbl, 0, sizeof (ihfs_ftbl));

    /* start converting */
    lseek(hxf_img, sizeof (hxf_header_t), SEEK_SET);

    ihfs_img = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
    lseek(ihfs_img, sizeof (ihfs_header_t) + 3 * SECTOR_SIZE + sizeof (ihfs_file_table_t), SEEK_SET);

    i = 0;
    while (lseek(hxf_img, 0, SEEK_CUR) < hxf_hdr.length) {
        read(hxf_img, &pathlen, 4);
        if (pathlen == 0)
            break;

        path = malloc(pathlen + 1);
        read(hxf_img, path, pathlen);
        path[pathlen] = '\0';
        read(hxf_img, &filetype, 1);
        read(hxf_img, &filesize, 4);

        /* update the file table and copy the data */
        strncpy(ihfs_ftbl.files[i].fullpath, path, MAX_IHFS_PATH);
        ihfs_ftbl.files[i].sector = lseek(ihfs_img, 0, SEEK_CUR) / 512;
        ihfs_ftbl.files[i].length = filesize;

        hxf_to_ihfs(hxf_img, ihfs_img, filesize);

        free(path);
        ++i;
    }
    ihfs_hdr.numfiles = i;

    /* finalize the ihfs image */

    pos = lseek(ihfs_img, 0, SEEK_CUR);
    if ((pos % SECTOR_SIZE) != 0) {
        printf("Something wrong happened during IHFS image creation at %d\n", pos);
        return 1;
    }
    if (pos < ihfs_len && pos < DEADFACE_START) {
        memset(ff, 0xff, SECTOR_SIZE);
        n = (DEADFACE_START - pos) / SECTOR_SIZE;
        for (i = 0; i < n; ++i)
            write(ihfs_img, ff, SECTOR_SIZE);
        pos = DEADFACE_START;
    }
    if (pos < ihfs_len) {
        for (i = 0; i < SECTOR_SIZE; i += 4) {
            deadface[i] = 0xde;
            deadface[i+1] = 0xad;
            deadface[i+2] = 0xfa;
            deadface[i+3] = 0xce;
        }
        n = (ihfs_len - pos) / SECTOR_SIZE;
        for (i = 0; i < n; ++i)
            write(ihfs_img, deadface, SECTOR_SIZE);
    }

    lseek(ihfs_img, 0, SEEK_SET);
    write(ihfs_img, &ihfs_hdr, sizeof (ihfs_hdr));
    memset(ff, 0xff, SECTOR_SIZE);
    write(ihfs_img, ff, SECTOR_SIZE);
    write(ihfs_img, ff, SECTOR_SIZE);
    write(ihfs_img, ff, SECTOR_SIZE);
    write(ihfs_img, &ihfs_ftbl, sizeof (ihfs_ftbl));

    /* cleanup */
    close(hxf_img);
    close(ihfs_img);
    
    printf("Done!\n");

    return 0;
}
