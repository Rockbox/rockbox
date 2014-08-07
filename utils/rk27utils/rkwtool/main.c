/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "rkw.h"

#define VERSION "v0.1"

static void banner(void)
{
    printf("RKWtool " VERSION " (C) Marcin Bukat 2014\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

static void usage(char *name)
{
    banner();

    printf("Usage: %s [-i] [-b] [-e] [-a] [-o prefix] file.rkw\n", name);
    printf("-i\t\tprint info about RKW file\n");
    printf("-b\t\textract nand bootloader images (s1.bin and s2.bin)\n");
    printf("-e\t\textract firmware files stored in RKST section\n");
    printf("-o prefix\twhen extracting firmware files put it there\n");
    printf("-a\t\textract additional file(s) (usually Rock27Boot.bin)\n");
    printf("-A\t\textract all data\n");
    printf("file.rkw\tRKW file to be processed\n");
}

int main(int argc, char **argv)
{
    int opt;
    struct rkw_info_t *rkw_info = NULL;
    char *prefix = NULL;
    bool info = false;
    bool extract = false;
    bool bootloader = false;
    bool addfile = false;

    while ((opt = getopt(argc, argv, "iebo:aA")) != -1)
    {
        switch (opt)
        {
            case 'i':
                info = true;
                break;

            case 'e':
                extract = true;
                break;

            case 'b':
                bootloader = true;
                break;

            case 'o':
                prefix = optarg;
                break;

            case 'a':
                addfile = true;
                break;

            case 'A':
                extract = true;
                bootloader = true;
                addfile = true;
                break;

            default:
                usage(argv[0]);
                break;
        }
    }

    if ((argc - optind) != 1 ||
        (!info && !extract && ! bootloader && !addfile))
    {
        usage(argv[0]);
        return -1;
    }

    banner();

    rkw_info = rkw_slurp(argv[optind]);

    if (rkw_info)
    {
        if (info)
        {
            rkrs_list_named_items(rkw_info);
            rkst_list_named_items(rkw_info);
        }

        if (extract)
            unpack_rkst(rkw_info, prefix);

        if (bootloader)
            unpack_bootloader(rkw_info, prefix);

        if (addfile)
            unpack_addfile(rkw_info, prefix);

        rkw_free(rkw_info);
        return 0;
    }

    return -1;
}
