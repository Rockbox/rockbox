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

static bool g_debug = false;
static char *g_out_prefix = NULL;

static void usage(void)
{
    printf("Usage: fwdecrypt [options] firmware\n");
    printf("Options:\n");
    printf("  -o <prefix>\tSet output file\n");
    printf("  -f/--force\tForce to continue on errors\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -d/--debug\tDisplay debug messages\n");
    exit(1);
}

static int s_read(void *user, int offset, void *buf, int size)
{
    FILE *f = user;
    if(fseek(f, offset, SEEK_SET) != 0)
        return 0;
    return fread(buf, 1, size, f);
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
    if(argc <= 1)
        usage();

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:", long_options, NULL);
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
            default:
                abort();
        }
    }

    if(optind != argc - 1)
        usage();

    FILE *f = fopen(argv[optind], "rb");
    if(f == NULL)
    {
        printf("Cannot open file for reading: %m\n");
        return 1;
    }

    enum samsung_error_t err;
    struct samsung_firmware_t *fw = samsung_read(s_read, s_printf,
        f, &err);
    if(fw == NULL || err != SAMSUNG_SUCCESS)
    {
        printf("Error reading firmware: %d\n", err);
        samsung_free(fw);
        return 1;
    }
    fclose(f);

    if(g_out_prefix)
    {
        f = fopen(g_out_prefix, "wb");
        if(f == NULL)
        {
            printf("Cannot open file for writing: %m\n");
            return 2;
        }
        fwrite(fw->data, 1, fw->data_size, f);
        fclose(f);
    }

    samsung_free(fw);

    return 0;
}
