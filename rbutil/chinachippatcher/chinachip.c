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
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "chinachip.h"

#define tr(x) x /* Qt translation support */

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

#ifdef STANDALONE
#define ERR(fmt, ...)   err(userdata, "[ERR]  "fmt"\n", ##__VA_ARGS__)
#define INFO(fmt, ...)  info(userdata, "[INFO] "fmt"\n", ##__VA_ARGS__)
#define tr(x) x
#else
#define ERR(fmt, ...)   err(userdata, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)  info(userdata, fmt, ##__VA_ARGS__)
#endif
#define FCLOSE(fd) fclose(fd); fd = NULL;
#define CCPMPBIN_HEADER_SIZE (sizeof(uint32_t)*2 + sizeof(uint8_t) + 9)
#define TOTAL_SIZE (fsize + CCPMPBIN_HEADER_SIZE + bsize)
int chinachip_patch(const char* firmware, const char* bootloader,
                    const char* output, const char* ccpmp_backup,
                    void (*info)(void*, char*, ...),
                    void (*err)(void*, char*, ...),
                    void* userdata)
{
    char header_time[13];
    time_t cur_time;
    struct tm* time_info;
    unsigned char* buf = NULL;
    FILE *fd = NULL, *bd = NULL, *od = NULL;
    unsigned int ccpmp_size = 0, i, fsize, bsize;
    signed int checksum = 0, ccpmp_pos;

    fd = fopen(firmware, "rb");
    if(!fd)
    {
        ERR(tr("Can't open file %s!"), firmware);
        goto err;
    }
    bd = fopen(bootloader, "rb");
    if(!bd)
    {
        ERR(tr("Can't open file %s!"), bootloader);
        goto err;
    }

    bsize = filesize(bd);
    INFO(tr("Bootloader size is %d bytes"), bsize);
    FCLOSE(bd);

    fsize = filesize(fd);
    INFO(tr("Firmware size is %d bytes"), fsize);

    buf = malloc(TOTAL_SIZE);
    if(buf == NULL)
    {
        ERR(tr("Can't allocate %d bytes!"), fsize);
        goto err;
    }
    memset(buf, 0, TOTAL_SIZE);

    INFO(tr("Reading %s into memory..."), firmware);
    if(fread(buf, fsize, 1, fd) != 1)
    {
        ERR(tr("Can't read file %s to memory!"), firmware);
        goto err;
    }
    FCLOSE(fd);

    if(memcmp(buf, "WADF", 4))
    {
        ERR(tr("File %s isn't a valid ChinaChip firmware!"), firmware);
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
        ERR(tr("Couldn't find ccpmp.bin in %s!"), firmware);
        goto err;
    }
    INFO(tr("Found ccpmp.bin at %d bytes"), ccpmp_pos);

    if(ccpmp_backup)
    {
        int ccpmp_data_pos = ccpmp_pos + 9;
        bd = fopen(ccpmp_backup, "wb");
        if(!bd)
        {
            ERR(tr("Can't open file %s!"), ccpmp_backup);
            goto err;
        }

        INFO(tr("Writing %d bytes to %s..."), ccpmp_size, ccpmp_backup);
        if(fwrite(&buf[ccpmp_data_pos], ccpmp_size, 1, bd) != 1)
        {
            ERR(tr("Can't write to file %s!"), ccpmp_backup);
            goto err;
        }
        FCLOSE(bd);
    }

    INFO(tr("Renaming it to ccpmp.old..."));
    buf[ccpmp_pos + 6] = 'o';
    buf[ccpmp_pos + 7] = 'l';
    buf[ccpmp_pos + 8] = 'd';

    bd = fopen(bootloader, "rb");
    if(!bd)
    {
        ERR(tr("Can't open file %s!"), bootloader);
        goto err;
    }

    /* Also include path size */
    ccpmp_pos -= sizeof(uint32_t);

    INFO(tr("Making place for ccpmp.bin..."));
    memmove(&buf[ccpmp_pos + bsize + CCPMPBIN_HEADER_SIZE],
            &buf[ccpmp_pos], fsize - ccpmp_pos);

    INFO(tr("Reading %s into memory..."), bootloader);
    if(fread(&buf[ccpmp_pos + CCPMPBIN_HEADER_SIZE],
             bsize, 1, bd) != 1)
    {
        ERR(tr("Can't read file %s to memory!"), bootloader);
        goto err;
    }
    FCLOSE(bd);

    INFO(tr("Adding header to %s..."), bootloader);
    int2le(&buf[ccpmp_pos            ], 9);                     /* Pathname Size */
    memcpy(&buf[ccpmp_pos + 4        ], "ccpmp.bin", 9);        /* Pathname */
    memset(&buf[ccpmp_pos + 4 + 9    ], 0x20, sizeof(uint8_t)); /* File Type */
    int2le(&buf[ccpmp_pos + 4 + 9 + 1], bsize);                 /* File Size */

    time(&cur_time);
    time_info = localtime(&cur_time);
    if(time_info == NULL)
    {
        ERR(tr("Can't obtain current time!"));
        goto err;
    }

    snprintf(header_time, 13, "%04d%02d%02d%02d%02d", time_info->tm_year + 1900,
                                              time_info->tm_mon,
                                              time_info->tm_mday,
                                              time_info->tm_hour,
                                              time_info->tm_min);

    INFO(tr("Computing checksum..."));
    for(i = sizeof(struct header); i < TOTAL_SIZE; i+=4)
        checksum += le2int(&buf[i]);

    INFO(tr("Updating main header..."));
    memcpy(&buf[offsetof(struct header, timestamp)], header_time, 12);
    int2le(&buf[offsetof(struct header, size)     ], TOTAL_SIZE);
    int2le(&buf[offsetof(struct header, checksum) ], checksum);

    od = fopen(output, "wb");
    if(!od)
    {
        ERR(tr("Can't open file %s!"), output);
        goto err;
    }

    INFO(tr("Writing output to %s..."), output);
    if(fwrite(buf, TOTAL_SIZE, 1, od) != 1)
    {
        ERR(tr("Can't write to file %s!"), output);
        goto err;
    }
    fclose(od);
    free(buf);

    return 0;

err:
    if(buf)
        free(buf);
    if(fd)
        fclose(fd);
    if(bd)
        fclose(bd);
    if(od)
        fclose(od);

    return -1;
}


#ifdef STANDALONE

#define VERSION         "0.1"
#define PRINT(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__)

static void info(void* userdata, char* fmt, ...)
{
    (void)userdata;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static void err(void* userdata, char* fmt, ...)
{
    (void)userdata;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void usage(char* name)
{
    PRINT("Usage:");
    PRINT(" %s <firmware> <bootloader> <firmware_output> [backup]", name);
    PRINT("\nExample:");
    PRINT(" %s VX747.HXF bootloader.bin output.HXF ccpmp.bak", name);
    PRINT(" This will copy ccpmp.bin in VX747.HXF as ccpmp.old and replace it"
          " with bootloader.bin, the output will get written to output.HXF."
          " The old ccpmp.bin will get written to ccpmp.bak.");
}

int main(int argc, char* argv[])
{
    PRINT("ChinaChipPatcher v" VERSION " - (C) Maurus Cuelenaere 2009");
    PRINT("This is free software; see the source for copying conditions. There is NO");
    PRINT("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

    if(argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    return chinachip_patch(argv[1], argv[2], argv[3], argc > 4 ? argv[4] : NULL,
                           &info, &err, NULL);
}
#endif
