/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Maurus Cuelenaere
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
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "chinachip.h"

/* From http://www.rockbox.org/wiki/ChinaChip */
struct header
{
    uint32_t signature;      /* WADF */
    uint32_t unk;
    int8_t   timestamp[12];  /* 200805081100 */
    uint32_t size;
    uint32_t checksum;
    uint32_t unk2;
    int8_t   identifier[32]; /* Chinachip PMP firmware V1.0 */
} __attribute__ ((packed));

static inline void int2le(unsigned char* addr, unsigned int val)
{
    addr[0] = val & 0xff;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

static inline unsigned int le2int(unsigned char* buf)
{
   return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
}

static long int filesize(FILE* fd)
{
    long int len;
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    return len;
}

#define FCLOSE(fd) fclose(fd); fd = NULL;
#define CCPMPBIN_HEADER_SIZE (sizeof(uint32_t)*2 + sizeof(uint8_t) + 9)
#define TOTAL_SIZE (fsize + CCPMPBIN_HEADER_SIZE + bsize)
enum cc_error chinachip_patch(const char* firmware, const char* bootloader,
                    const char* output, const char* ccpmp_backup)
{
    char header_time[13];
    time_t cur_time;
    struct tm* time_info;
    unsigned char* buf = NULL;
    FILE *fd = NULL, *bd = NULL, *od = NULL;
    unsigned int ccpmp_size = 0, i, fsize, bsize;
    signed int checksum = 0, ccpmp_pos;
    int result = E_OK;

    fd = fopen(firmware, "rb");
    if(!fd)
    {
        fprintf(stderr, "[ERR]  Can't open file %s!\n", firmware);
        result = E_OPEN_FIRMWARE;
        goto err;
    }
    bd = fopen(bootloader, "rb");
    if(!bd)
    {
        fprintf(stderr, "[ERR]  Can't open file %s!\n", bootloader);
        result = E_OPEN_BOOTLOADER;
        goto err;
    }

    bsize = filesize(bd);
    fprintf(stderr, "[INFO] Bootloader size is %d bytes\n", bsize);
    FCLOSE(bd);

    fsize = filesize(fd);
    fprintf(stderr, "[INFO] Firmware size is %d bytes\n", fsize);

    buf = malloc(TOTAL_SIZE);
    if(buf == NULL)
    {
        fprintf(stderr, "[ERR]  Can't allocate %d bytes!\n", fsize);
        result = E_MEMALLOC;
        goto err;
    }
    memset(buf, 0, TOTAL_SIZE);

    fprintf(stderr, "[INFO] Reading %s into memory...\n", firmware);
    if(fread(buf, fsize, 1, fd) != 1)
    {
        fprintf(stderr, "[ERR]  Can't read file %s to memory!\n", firmware);
        result = E_LOAD_FIRMWARE;
        goto err;
    }
    FCLOSE(fd);

    if(memcmp(buf, "WADF", 4))
    {
        fprintf(stderr, "[ERR]  File %s isn't a valid ChinaChip firmware!\n", firmware);
        result = E_INVALID_FILE;
        goto err;
    }

    ccpmp_pos = -1, i = 0x40;
    do
    {
        int filenamesize = le2int(&buf[i]);
        i += sizeof(uint32_t);

        if(!strncmp((char*) &buf[i], "ccpmp.bin", 9))
        {
            ccpmp_pos = i;
            ccpmp_size = le2int(&buf[i + sizeof(uint8_t) + filenamesize]);
        }
        else
            i += filenamesize + le2int(&buf[i + sizeof(uint8_t) + filenamesize])
                 + sizeof(uint32_t) + sizeof(uint8_t);
    } while(ccpmp_pos < 0 && i < fsize);

    if(i >= fsize)
    {
        fprintf(stderr, "[ERR]  Couldn't find ccpmp.bin in %s!\n", firmware);
        result = E_NO_CCPMP;
        goto err;
    }
    fprintf(stderr, "[INFO] Found ccpmp.bin at %d bytes\n", ccpmp_pos);

    if(ccpmp_backup)
    {
        int ccpmp_data_pos = ccpmp_pos + 9;
        bd = fopen(ccpmp_backup, "wb");
        if(!bd)
        {
            fprintf(stderr, "[ERR]  Can't open file %s!\n", ccpmp_backup);
            result = E_OPEN_BACKUP;
            goto err;
        }

        fprintf(stderr, "[INFO] Writing %d bytes to %s...\n", ccpmp_size, ccpmp_backup);
        if(fwrite(&buf[ccpmp_data_pos], ccpmp_size, 1, bd) != 1)
        {
            fprintf(stderr, "[ERR]  Can't write to file %s!\n", ccpmp_backup);
            result = E_WRITE_BACKUP;
            goto err;
        }
        FCLOSE(bd);
    }

    fprintf(stderr, "[INFO] Renaming it to ccpmp.old...\n");
    buf[ccpmp_pos + 6] = 'o';
    buf[ccpmp_pos + 7] = 'l';
    buf[ccpmp_pos + 8] = 'd';

    bd = fopen(bootloader, "rb");
    if(!bd)
    {
        fprintf(stderr, "[ERR]  Can't open file %s!\n", bootloader);
        result = E_OPEN_BOOTLOADER;
        goto err;
    }

    /* Also include path size */
    ccpmp_pos -= sizeof(uint32_t);

    fprintf(stderr, "[INFO] Making place for ccpmp.bin...\n");
    memmove(&buf[ccpmp_pos + bsize + CCPMPBIN_HEADER_SIZE],
            &buf[ccpmp_pos], fsize - ccpmp_pos);

    fprintf(stderr, "[INFO] Reading %s into memory...\n", bootloader);
    if(fread(&buf[ccpmp_pos + CCPMPBIN_HEADER_SIZE],
             bsize, 1, bd) != 1)
    {
        fprintf(stderr, "[ERR]  Can't read file %s to memory!\n", bootloader);
        result = E_LOAD_BOOTLOADER;
        goto err;
    }
    FCLOSE(bd);

    fprintf(stderr, "[INFO] Adding header to %s...\n", bootloader);
    int2le(&buf[ccpmp_pos            ], 9);                     /* Pathname Size */
    memcpy(&buf[ccpmp_pos + 4        ], "ccpmp.bin", 9);        /* Pathname */
    memset(&buf[ccpmp_pos + 4 + 9    ], 0x20, sizeof(uint8_t)); /* File Type */
    int2le(&buf[ccpmp_pos + 4 + 9 + 1], bsize);                 /* File Size */

    time(&cur_time);
    time_info = localtime(&cur_time);
    if(time_info == NULL)
    {
        fprintf(stderr, "[ERR]  Can't obtain current time!\n");
        result = E_GET_TIME;
        goto err;
    }

    snprintf(header_time, 13, "%04d%02d%02d%02d%02d", time_info->tm_year + 1900,
                                              time_info->tm_mon,
                                              time_info->tm_mday,
                                              time_info->tm_hour,
                                              time_info->tm_min);

    fprintf(stderr, "[INFO] Computing checksum...\n");
    for(i = sizeof(struct header); i < TOTAL_SIZE; i+=4)
        checksum += le2int(&buf[i]);

    fprintf(stderr, "[INFO] Updating main header...\n");
    memcpy(&buf[offsetof(struct header, timestamp)], header_time, 12);
    int2le(&buf[offsetof(struct header, size)     ], TOTAL_SIZE);
    int2le(&buf[offsetof(struct header, checksum) ], checksum);

    od = fopen(output, "wb");
    if(!od)
    {
        fprintf(stderr, "[ERR]  Can't open file %s!\n", output);
        result = E_OPEN_OUTFILE;
        goto err;
    }

    fprintf(stderr, "[INFO] Writing output to %s...\n", output);
    if(fwrite(buf, TOTAL_SIZE, 1, od) != 1)
    {
        fprintf(stderr, "[ERR]  Can't write to file %s!\n", output);
        result = E_WRITE_OUTFILE;
        goto err;
    }

err:
    if(buf)
        free(buf);
    if(fd)
        fclose(fd);
    if(bd)
        fclose(bd);
    if(od)
        fclose(od);

    return result;
}

