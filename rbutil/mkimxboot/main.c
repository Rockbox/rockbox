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

struct model_t
{
    const char *name;
    enum imx_model_t model;
};

struct model_t imx_models[] =
{
    { "unknown", MODEL_UNKNOWN },
    { "fuzeplus", MODEL_FUZEPLUS },
    { "zenxfi2", MODEL_ZENXFI2 },
    { "zenxfi3", MODEL_ZENXFI3 },
    { "zenxfistyle", MODEL_ZENXFISTYLE },
    { "zenstyle", MODEL_ZENSTYLE },
    { "nwze370", MODEL_NWZE370 },
    { "nwze360", MODEL_NWZE360 },
};

#define NR_MODELS sizeof(imx_models) / sizeof(imx_models[0])

static void usage(void)
{
    printf("Usage: mkimxboot [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help   Display this message\n");
    printf("  -o <file>   Set output file\n");
    printf("  -i <file>   Set input file\n");
    printf("  -b <file>   Set boot file\n");
    printf("  -d/--debug  Enable debug output\n");
    printf("  -t <type>   Set type (dualboot, singleboot, recovery, origfw, charge)\n");
    printf("  -v <v>      Set variant\n");
    printf("  -x          Dump device informations\n");
    printf("  -p <ver>    Force product and component version\n");
    printf("  -5 <type>   Compute <type> MD5 sum of the input file\n");
    printf("  -m <model>  Specify model (useful for soft MD5 sum)\n");
    printf("Supported variants: (default is standard)\n");
    printf("  ");
    for(size_t i = 0; i < NR_VARIANTS; i++)
    {
        if(i != 0)
            printf(", ");
        printf("%s", imx_variants[i].name);
    }
    printf("\n");
    printf("Supported models: (default is unknown)\n");
    for(size_t i = 0; i < NR_MODELS; i++)
    {
        if(i != 0)
            printf(", ");
        printf("%s", imx_models[i].name);
    }
    printf("\n");
    printf("By default a dualboot image is built\n");
    printf("This tools supports the following format for the boot file:\n");
    printf("- rockbox scramble format\n");
    printf("- elf format\n");
    printf("Additional checks will be performed on rockbox scramble format to\n");
    printf("ensure soundness of operation.\n");
    printf("There are two types of MD5 sums: 'full' or 'soft'.\n");
    printf("- full MD5 sum applies to the whole file\n");
    printf("- soft MD5 sum for SB files accounts for relevant parts only\n");
    exit(1);
}

static int print_md5(const char *file, const char *type)
{
    uint8_t md5sum[16];
    enum imx_error_t err;
    if(strcmp(type, "full") == 0)
        err = compute_md5sum(file, md5sum);
    else if(strcmp(type, "soft") == 0)
        err = compute_soft_md5sum(file, md5sum);
    else
    {
        printf("Invalid md5sum type '%s'\n", type);
        return 1;
    }
    if(err != IMX_SUCCESS)
    {
        printf("There was an error when computing the MD5 sum: %d\n", err);
        return 2;
    }
    printf("%s MD5 sum: ", type);
    for(int i = 0; i < 16; i++)
        printf("%02x", md5sum[i]);
    printf("\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char *infile = NULL;
    char *outfile = NULL;
    char *bootfile = NULL;
    enum imx_firmware_variant_t variant = VARIANT_DEFAULT;
    enum imx_output_type_t type = IMX_DUALBOOT;
    enum imx_model_t model = MODEL_UNKNOWN;
    bool debug = false;
    const char *md5type = NULL;
    const char *force_version = NULL;

    if(argc == 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"in-file", no_argument, 0, 'i'},
            {"out-file", required_argument, 0, 'o'},
            {"boot-file", required_argument, 0, 'b'},
            {"debug", no_argument, 0, 'd'},
            {"type", required_argument, 0, 't'},
            {"variant", required_argument, 0, 'v'},
            {"dev-info", no_argument, 0, 'x'},
            {"model", required_argument, 0, 'm'},
            {"md5", required_argument, 0, '5'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hdi:o:b:t:v:xp:m:5:", long_options, NULL);
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
                else if(strcmp(optarg, "origfw") == 0)
                    type = IMX_ORIG_FW;
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
            case 'p':
                force_version = optarg;
                break;
            case '5':
                md5type = optarg;
                break;
            case 'm':
                if(model != MODEL_UNKNOWN)
                {
                    printf("You cannot specify two models\n");
                    return 1;
                }
                for(int i = 0; i < NR_MODELS; i++)
                    if(strcmp(optarg, imx_models[i].name) == 0)
                    {
                        model = imx_models[i].model;
                        break;
                    }
                if(model == MODEL_UNKNOWN)
                    printf("Unknown model '%s'\n", optarg);
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

    if(md5type)
        return print_md5(infile, md5type);

    if(!outfile)
    {
        printf("You must specify an output file\n");
        return 1;
    }

    if(!bootfile && type != IMX_ORIG_FW)
    {
        printf("You must specify an boot file\n");
        return 1;
    }

    if(optind != argc)
    {
        printf("Extra arguments on command line\n");
        return 1;
    }

    struct imx_option_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.debug = debug;
    opt.output = type;
    opt.fw_variant = variant;
    opt.force_version = force_version;
    opt.model = model;
    enum imx_error_t err = mkimxboot(infile, bootfile, outfile, opt);
    printf("Result: %d\n", err);
    return 0;
}
