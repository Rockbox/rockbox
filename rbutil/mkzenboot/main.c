/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "mkzenboot.h"

static void usage(void)
{
    printf("Usage: mkzenboot [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help  Display this message\n");
    printf("  -i <file>  Set input file\n");
    printf("  -o <file>  Set output file\n");
    printf("  -b <file>  Set boot file\n");
    printf("  -t <type>  Set output type\n");
    printf("  -d/--debug Enable debug output\n");
    printf("Output types: dualboot, deferred_dualboot, singleboot, recovery\n");
    printf("By default a dualboot image is built\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *outfile = NULL;
    char *bootfile = NULL;
    char *infile = NULL;
    struct zen_option_t opt;
    memset(&opt, 0, sizeof(opt));

    if(argc == 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?di:o:b:t:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case 'd':
                opt.debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'i':
                infile = optarg;
                break;
            case 'b':
                bootfile = optarg;
                break;
            case 't':
                if(strcmp(optarg, "dualboot") == 0) opt.output = ZEN_DUALBOOT;
                else if(strcmp(optarg, "mixedboot") == 0) opt.output = ZEN_MIXEDBOOT;
                else if(strcmp(optarg, "singleboot") == 0) opt.output = ZEN_SINGLEBOOT;
                else if(strcmp(optarg, "recovery") == 0) opt.output = ZEN_RECOVERY;
                else
                {
                    printf("Unknown output type '%s'\n", optarg);
                    return 1;
                }
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
    if(!infile)
    {
        printf("You must specify an input file\n");
        return 1;
    }
    if(optind != argc)
    {
        printf("Extra arguments on command line\n");
        return 1;
    }

    enum zen_error_t err = mkzenboot(infile, bootfile, outfile, opt);
    printf("Result: %d\n", err);
    return 0;
}
