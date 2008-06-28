/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Based on sendfile.c from libmtp: http://libmtp.sourceforge.net
 * Modified by Maurus Cuelenaere and Nicolas Pennequin.
 *
 * Copyright (C) 2005-2007 Linus Walleij
 * Copyright (C) 2006 Chris A. Debenham
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

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "libmtp.h"

LIBMTP_mtpdevice_t *device;

static int progress(u_int64_t const sent, u_int64_t const total,
                    void const *const data)
{
    int percent = (sent * 100) / total;
#ifdef __WIN32__
    printf("Progress: %I64u of %I64u (%d%%)\r", sent, total, percent);
#else
    printf("Progress: %llu of %llu (%d%%)\r", sent, total, percent);
#endif
    fflush(stdout);
    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: sendfirm <local filename>\n");
}

int sendfile_function(char *from_path)
{
    printf("Sending %s\n", from_path);
    uint64_t filesize;
#ifdef __USE_LARGEFILE64
    struct stat64 sb;
#else
    struct stat sb;
#endif
    LIBMTP_file_t *genfile;
    int ret;
    uint32_t parent_id = 0;

#ifdef __USE_LARGEFILE64
    if (stat64(from_path, &sb) == -1)
    {
#else
    if (stat(from_path, &sb) == -1)
    {
#endif
        fprintf(stderr, "%s: ", from_path);
        perror("stat");
        exit(1);
    }

#ifdef __USE_LARGEFILE64
    filesize = sb.st_size;
#else
    filesize = (uint64_t) sb.st_size;
#endif

    genfile = LIBMTP_new_file_t();
    genfile->filesize = filesize;
    genfile->filename = strdup("nk.bin");
    genfile->filetype = LIBMTP_FILETYPE_FIRMWARE;

    printf("Sending file...\n");
    ret = LIBMTP_Send_File_From_File(device, from_path, genfile, progress,
                                     NULL, parent_id);
    printf("\n");
    if (ret != 0)
    {
        printf("Error sending file.\n");
        LIBMTP_Dump_Errorstack(device);
        LIBMTP_Clear_Errorstack(device);
    }
    else
    {
        printf("New file ID: %d\n", genfile->item_id);
    }

    LIBMTP_destroy_file_t(genfile);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return 1;
    }

    LIBMTP_Init();

    fprintf(stdout, "libmtp version: " LIBMTP_VERSION_STRING "\n\n");

    device = LIBMTP_Get_First_Device();
    if (device == NULL)
    {
        printf("No devices.\n");
        return 0;
    }

    sendfile_function(argv[1]);

    LIBMTP_Release_Device(device);

    exit(0);
}
