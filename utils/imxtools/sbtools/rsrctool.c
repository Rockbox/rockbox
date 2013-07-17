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

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <getopt.h>

#include "crypto.h"
#include "rsrc.h"
#include "misc.h"

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

#define crypto_cbc(...) \
    do { int ret = crypto_cbc(__VA_ARGS__); \
        if(ret != CRYPTO_ERROR_SUCCESS) \
            bug("crypto_cbc error: %d\n", ret); \
    }while(0)

/* globals */

char *g_out_prefix;

static void extract_rsrc_file(struct rsrc_file_t *file)
{
    (void) file;
}

static void usage(void)
{
    printf("Usage: rsrctool [options] rsrc-file\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <prefix>\tEnable output and set prefix\n");
    printf("  -d/--debug\tEnable debug output*\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  -a/--add-key <key>\tAdd single key (hex or usbotp)\n");
    printf("  -n/--no-color\tDisable output colors\n");
    printf("  -l/--loopback <file>\tProduce rsrc file out of extracted description*\n");
    printf("  -f/--force\tForce reading even without a key*\n");
    printf("Options marked with a * are for debug purpose only\n");
    exit(1);
}

static void rsrc_printf(void *user, bool error, color_t c, const char *fmt, ...)
{
    (void) user;
    (void) error;
    if(!g_debug)
        return;
    va_list args;
    va_start(args, fmt);
    color(c);
    vprintf(fmt, args);
    va_end(args);
}

static struct crypto_key_t g_zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

int main(int argc, char **argv)
{
    const char *loopback = NULL;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {"no-color", no_argument, 0, 'n'},
            {"loopback", required_argument, 0, 'l'},
            {"force", no_argument, 0, 'f' },
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:k:za:nl:f", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'l':
                if(loopback)
                    bug("Only one loopback file can be specified !\n");
                loopback = optarg;
                break;
            case 'n':
                enable_color(false);
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
            case 'f':
                g_force = true;
                break;
            case 'k':
            {
                if(!add_keys_from_file(optarg))
                    bug("Cannot add keys from %s\n", optarg);
                break;
            }
            case 'z':
            {
                add_keys(&g_zero_key, 1);
                break;
            }
            case 'a':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key specified as argument\n");
                if(*s != 0)
                    bug("Trailing characters after key specified as argument\n");
                add_keys(&key, 1);
                break;
            }
            default:
                bug("Internal error: unknown option '%c'\n", c);
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    const char *rsrc_filename = argv[optind];

    enum rsrc_error_t err;
    struct rsrc_file_t *file = rsrc_read_file(rsrc_filename, NULL, rsrc_printf, &err);
    if(file == NULL)
    {
        color(OFF);
        printf("RSRC read failed: %d\n", err);
        return 1;
    }
    if(g_debug)
        printf("%d entries read from file\n", file->nr_entries);

    color(OFF);
    if(g_out_prefix)
        extract_rsrc_file(file);
    if(g_debug)
    {
        color(GREY);
        printf("[Debug output]\n");
        rsrc_dump(file, NULL, rsrc_printf);
    }
    if(loopback)
    {
        rsrc_write_file(file, loopback);
    }
    rsrc_free(file);
    clear_keys();

    return 0;
}

