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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include "misc.h"
#include "fwu.h"
#include "afi.h"
#include "fw.h"

bool g_debug = false;
char *g_out_prefix = NULL;
char *g_in_file = NULL;

/* [add]: string to add when there is no extension
 * [replace]: string to replace extension */
static void build_out_prefix(char *add, char *replace, bool slash)
{
    if(g_out_prefix)
        return;
    /** copy input filename with extra space */
    g_out_prefix = malloc(strlen(g_in_file) + strlen(add) + 16);
    strcpy(g_out_prefix, g_in_file);
    /** remove extension and add '/' */
    char *filename = strrchr(g_out_prefix, '/');
    // have p points to the beginning or after the last '/'
    filename = (filename == NULL) ? g_out_prefix : filename + 1;
    // extension ?
    char *dot = strrchr(filename, '.');
    if(dot)
    {
        *dot = 0; // cut at the dot
        strcat(dot, replace);
    }
    else
        strcat(filename, add); // add extra string

    if(slash)
    {
        strcat(filename, "/");
        /** make sure the directory exists */
        mkdir(g_out_prefix, S_IRWXU | S_IRGRP | S_IROTH);
    }
}

static int do_fwu(uint8_t *buf, size_t size, enum fwu_mode_t mode)
{
    int ret = fwu_decrypt(buf, &size, mode);
    if(ret != 0)
        return ret;

    build_out_prefix(".afi", ".afi", false);
    cprintf(GREY, "Descrambling to %s... ", g_out_prefix);
    FILE *f = fopen(g_out_prefix, "wb");
    if(f)
    {
        fwrite(buf, size, 1, f);
        fclose(f);
        cprintf(RED, "Ok\n");
        return 0;
    }
    else
    {
        color(RED);
        perror("Failed");
        return 1;
    }
}

static int unpack_afi_fw_cb(const char *filename, uint8_t *buf, size_t size)
{
    char *name = malloc(strlen(g_out_prefix) + strlen(filename) + 16);
    sprintf(name, "%s%s", g_out_prefix, filename);

    cprintf(GREY, "Unpacking to %s... ", name);
    FILE *f = fopen(name, "wb");
    if(f)
    {
        fwrite(buf, size, 1, f);
        fclose(f);
        cprintf(RED, "Ok\n");
        return 0;
    }
    else
    {
        color(RED);
        perror("Failed");
        return 1;
    }
}

static int do_afi(uint8_t *buf, size_t size)
{
    build_out_prefix(".fw", "", true);
    return afi_unpack(buf, size, &unpack_afi_fw_cb);
}

static int do_fw(uint8_t *buf, size_t size)
{
    build_out_prefix(".unpack", "", true);
    return fw_unpack(buf, size, &unpack_afi_fw_cb);
}
static void usage(void)
{
    printf("Usage: atjboottool [options] firmware\n");
    printf("Options:\n");
    printf("  -o <path>         Set output file or output prefix\n");
    printf("  -h/--help         Display this message\n");
    printf("  -d/--debug        Display debug messages\n");
    printf("  -c/--no-color     Disable color output\n");
    printf("  --fwu             Unpack a FWU firmware file\n");
    printf("  --afi             Unpack a AFI archive file\n");
    printf("  --fw              Unpack a FW archive file\n");
    printf("  --atj2127         Force ATJ2127 decryption mode\n");
    printf("The default is to try to guess the format.\n");
    printf("If several formats are specified, all are tried.\n");
    printf("If no output prefix is specified, a default one is picked.\n");
    exit(1);
}

int main(int argc, char **argv)
{
    bool try_fwu = false;
    bool try_afi = false;
    bool try_fw = false;
    enum fwu_mode_t fwu_mode = FWU_AUTO;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'c'},
            {"fwu", no_argument, 0, 'u'},
            {"afi", no_argument, 0, 'a'},
            {"fw", no_argument, 0, 'w'},
            {"atj2127", no_argument, 0, '2'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hdco:a2", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'c':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
                break;
            case 'h':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'a':
                try_afi = true;
                break;
            case 'u':
                try_fwu = true;
                break;
            case 'w':
                try_fw = true;
                break;
            case '2':
                fwu_mode = FWU_ATJ2127;
                break;
            default:
                abort();
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    g_in_file = argv[optind];
    FILE *fin = fopen(g_in_file, "r");
    if(fin == NULL)
    {
        perror("Cannot open boot file");
        return 1;
    }
    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    void *buf = malloc(size);
    if(buf == NULL)
    {
        perror("Cannot allocate memory");
        return 1;
    }

    if(fread(buf, size, 1, fin) != 1)
    {
        perror("Cannot read file");
        return 1;
    }

    fclose(fin);

    int ret = -99;
    if(try_fwu || fwu_check(buf, size))
        ret = do_fwu(buf, size, fwu_mode);
    else if(try_afi || afi_check(buf, size))
        ret = do_afi(buf, size);
    else if(try_fw || fw_check(buf, size))
        ret = do_fw(buf, size);
    else
    {
        cprintf(GREY, "No valid format found\n");
        ret = 1;
    }

    if(ret != 0)
    {
        cprintf(GREY, "Error: %d", ret);
        printf("\n");
        ret = 2;
    }
    free(buf);

    color(OFF);

    return ret;
}

