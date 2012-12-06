/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include "samsung.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>

static bool g_debug = false;
static char *g_out_prefix = NULL;

static void usage(void)
{
    printf("Usage: fwcrypt [options] content\n");
    printf("Options:\n");
    printf("  -o <prefix>\tSet output file\n");
    printf("  -m/--model <model>\tSet model\n");
    printf("  -v/--version <ver>\tSet version\n");
    printf("  -r/--region <region>\tSet region\n");
    printf("  -e/--extra <extra>\tSet extra\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -d/--debug\tDisplay debug messages\n");
    exit(1);
}

static int s_write(void *user, int offset, void *buf, int size)
{
    FILE *f = user;
    if(fseek(f, offset, SEEK_SET) != 0)
        return 0;
    return fwrite(buf, 1, size, f);
}

static void s_printf(void *user, bool error, const char *fmt, ...)
{
    if(!g_debug && !error)
        return;
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

int main(int argc, char **argv)
{
    struct samsung_firmware_t *fw = malloc(sizeof(struct samsung_firmware_t));
    memset(fw, 0, sizeof(struct samsung_firmware_t));
    
    if(argc <= 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"model", required_argument, 0, 'm'},
            {"version", required_argument, 0, 'v'},
            {"region", required_argument, 0, 'r'},
            {"extra", required_argument, 0, 'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:m:v:r:e:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'm':
                strncpy(fw->model, optarg, sizeof(fw->model));
                if(strlen(optarg) > sizeof(fw->model))
                    printf("Warning: truncate model string\n");
                break;
            case 'r':
                strncpy(fw->region, optarg, sizeof(fw->region));
                if(strlen(optarg) > sizeof(fw->region))
                    printf("Warning: truncate region string\n");
                break;
            case 'v':
                strncpy(fw->version, optarg, sizeof(fw->version));
                if(strlen(optarg) > sizeof(fw->version))
                    printf("Warning: truncate vesion string\n");
                break;
            case 'e':
                strncpy(fw->extra, optarg, sizeof(fw->extra));
                if(strlen(optarg) > sizeof(fw->extra))
                    printf("Warning: truncate extra string\n");
                break;
            default:
                abort();
        }
    }

    if(optind != argc - 1)
        usage();

    FILE *fin = fopen(argv[optind], "rb");
    if(fin == NULL)
    {
        printf("Cannot open file for reading: %m\n");
        samsung_free(fw);
        return 1;
    }
    fseek(fin, 0, SEEK_END);
    fw->data_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    fw->data = malloc(fw->data_size);
    if((int)fread(fw->data, 1, fw->data_size, fin) != fw->data_size)
    {
        printf("Cannot read input file: %m\n");
        samsung_free(fw);
        return 2;
    }
    fclose(fin);

    if(g_out_prefix)
    {
        FILE *f = fopen(g_out_prefix, "wb");
        if(f == NULL)
        {
            printf("Cannot open file for writing: %m\n");
            samsung_free(fw);
            return 1;
        }

        enum samsung_error_t err = samsung_write(s_write, s_printf, f, fw);
        if(err != SAMSUNG_SUCCESS)
        {
            printf("Error writing firmware: %d\n", err);
            samsung_free(fw);
            return 3;
        }
        fclose(f);
    }
    samsung_free(fw);

    return 0;
}
