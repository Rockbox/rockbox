/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mknwzboot.h"

static void usage(void)
{
    printf("Usage: mknwzboot [options | file]...\n");
    printf("Options:\n");
    printf("  -h/--help   Display this message\n");
    printf("  -o <file>   Set output file\n");
    printf("  -b <file>   Set boot file\n");
    printf("  -d/--debug  Enable debug output\n");
    printf("  -x          Dump device informations\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *outfile = NULL;
    char *bootfile = NULL;
    bool debug = false;

    if(argc == 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"out-file", required_argument, 0, 'o'},
            {"boot-file", required_argument, 0, 'b'},
            {"debug", no_argument, 0, 'd'},
            {"dev-info", no_argument, 0, 'x'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "ho:b:dx", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case 'd':
                debug = true;
                break;
            case 'h':
                usage();
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'b':
                bootfile = optarg;
                break;
            case 'x':
                dump_nwz_dev_info("");
                break;
            default:
                abort();
        }
    }

    if(!outfile)
    {
        printf("You must specify an output file\n");
        return 1;
    }
    if(!bootfile)
    {
        printf("You must specify a boot file\n");
        return 1;
    }
    if(optind != argc)
    {
        printf("Extra arguments on command line\n");
        return 1;
    }

    int err = mknwzboot(bootfile, outfile, debug);
    printf("Result: %d\n", err);
    return err;
}
