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
#include "mkimxboot.h"

struct imx_variant_t
{
    const char *name;
    enum imx_firmware_variant_t variant;
};

static struct imx_variant_t imx_variants[] =
{
    { "default", VARIANT_DEFAULT },
    { "zenxfi2-recovery", VARIANT_ZENXFI2_RECOVERY },
    { "zenxfi2-nand", VARIANT_ZENXFI2_NAND },
    { "zenxfi2-sd", VARIANT_ZENXFI2_SD },
    { "zenxfistyle-recovery", VARIANT_ZENXFISTYLE_RECOVERY },
    { "zenstyle-recovery", VARIANT_ZENSTYLE_RECOVERY },
};

#define NR_VARIANTS sizeof(imx_variants) / sizeof(imx_variants[0])

static void usage(void)
{
    printf("Usage: mkimxboot [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help   Display this message\n");
    printf("  -o <file>   Set output file\n");
    printf("  -i <file>   Set input file\n");
    printf("  -b <file>   Set boot file\n");
    printf("  -d/--debug  Enable debug output\n");
    printf("  -t <type>   Set type (dualboot, singleboot, recovery, charge)\n");
    printf("  -v <v>      Set variant\n");
    printf("  -x          Dump device informations\n");
    printf("  -w          Extract the original firmware\n");
    printf("  -p <ver>    Force product and component version\n");
    printf("Supported variants: (default is standard)\n");
    printf("  ");
    for(size_t i = 0; i < NR_VARIANTS; i++)
    {
        if(i != 0)
            printf(", ");
        printf("%s", imx_variants[i].name);
    }
    printf("\n");
    printf("By default a dualboot image is built\n");
    printf("This tools supports the following format for the boot file:\n");
    printf("- rockbox scramble format\n");
    printf("- elf format\n");
    printf("Additional checks will be performed on rockbox scramble format to\n");
    printf("ensure soundness of operation.\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *infile = NULL;
    char *outfile = NULL;
    char *bootfile = NULL;
    enum imx_firmware_variant_t variant = VARIANT_DEFAULT;
    enum imx_output_type_t type = IMX_DUALBOOT;
    bool debug = false;
    bool extract_of = false;
    const char *force_version = NULL;

    if(argc == 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"in-file", no_argument, 0, 'i'},
            {"out-file", required_argument, 0, 'o'},
            {"boot-file", required_argument, 0, 'b'},
            {"debug", no_argument, 0, 'd'},
            {"type", required_argument, 0, 't'},
            {"variant", required_argument, 0, 'v'},
            {"dev-info", no_argument, 0, 'x'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?di:o:b:t:v:xwp:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case 'd':
                debug = true;
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
                if(strcmp(optarg, "dualboot") == 0)
                    type = IMX_DUALBOOT;
                else if(strcmp(optarg, "singleboot") == 0)
                    type = IMX_SINGLEBOOT;
                else if(strcmp(optarg, "recovery") == 0)
                    type = IMX_RECOVERY;
                else if(strcmp(optarg, "charge") == 0)
                    type = IMX_CHARGE;
                else
                {
                    printf("Invalid boot type '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'v':
            {
                for(size_t i = 0; i < NR_VARIANTS; i++)
                {
                    if(strcmp(optarg, imx_variants[i].name) == 0)
                    {
                        variant = imx_variants[i].variant;
                        goto Lok;
                    }
                }
                printf("Invalid variant '%s'\n", optarg);
                return 1;

                Lok:
                break;
            }
            case 'x':
                dump_imx_dev_info("");
                printf("variant mapping:\n");
                for(int i = 0; i < sizeof(imx_variants) / sizeof(imx_variants[0]); i++)
                    printf("  %s -> variant=%d\n", imx_variants[i].name, imx_variants[i].variant);
                break;
            case 'w':
                extract_of = true;
                break;
            case 'p':
                force_version = optarg;
                break;
            default:
                abort();
        }
    }

    if(!infile)
    {
        printf("You must specify an input file\n");
        return 1;
    }
    if(!outfile)
    {
        printf("You must specify an output file\n");
        return 1;
    }
    if(!bootfile && !extract_of)
    {
        printf("You must specify an boot file\n");
        return 1;
    }
    if(optind != argc)
    {
        printf("Extra arguments on command line\n");
        return 1;
    }

    if(extract_of)
    {
        enum imx_error_t err = extract_firmware(infile, variant, outfile);
        printf("Result: %d\n", err);
        return 0;
    }

    struct imx_option_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.debug = debug;
    opt.output = type;
    opt.fw_variant = variant;
    opt.force_version = force_version;
    enum imx_error_t err = mkimxboot(infile, bootfile, outfile, opt);
    printf("Result: %d\n", err);
    return 0;
}
