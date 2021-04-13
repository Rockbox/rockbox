/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

jz_context* jz = NULL;
const jz_device_info* dev_info = NULL;
int dev_action = -1;
jz_paramlist* action_params = NULL;

void usage(void)
{
    printf("Usage:\n"
           "  jztool [global options] <device> <action> [action options]\n"
           "\n"
           "Global options:\n"
           "\n"
           "  -h, --help            Display this help\n"
           "  -q, --quiet           Don't log anything except errors\n"
           "  -v, --verbose         Display detailed logging output\n"
           "  -l, --loglevel LEVEL  Set log level\n");

    printf("Supported devices:\n\n");
    int n = jz_get_num_device_info();
    for(int i = 0; i < n; ++i) {
        const jz_device_info* info = jz_get_device_info_indexed(i);
        printf("  %s - %s\n", info->name, info->description);
    }

    printf(
"\n"
"Available actions for fiiom3k:\n"
"\n"
"  install --spl <spl.m3k> --bootloader <bootloader.m3k>\n"
"          [--without-backup yes] [--backup IMAGE]\n"
"    Install or update the Rockbox bootloader on a device.\n"
"\n"
"    If --backup is given, back up the current bootloader to IMAGE before\n"
"    installing the new bootloader. The installer will normally refuse to\n"
"    overwrite your current bootloader; pass '--without-backup yes' if you\n"
"    really want to proceed without taking a backup.\n"
"\n"
"    WARNING: it is NOT RECOMMENDED to install the Rockbox bootloader\n"
"    without taking a backup of the original firmware bootloader. It may\n"
"    be very difficult or impossible to recover your player without one.\n"
"    At least one M3Ks is known to not to work with the Rockbox bootloader,\n"
"    so it is very important to take a backup.\n"
"\n"
"  backup --image IMAGE\n"
"    Backup the current bootloader to the file IMAGE\n"
"\n"
"  restore --image IMAGE\n"
"    Restore a bootloader image backup from the file IMAGE\n"
"\n");

    exit(4);
}

void cleanup(void)
{
    if(action_params)
        jz_paramlist_free(action_params);
    if(jz)
        jz_context_destroy(jz);
}

int main(int argc, char** argv)
{
    if(argc < 2)
        usage();

    /* Library initialization */
    jz = jz_context_create();
    if(!jz) {
        fprintf(stderr, "ERROR: Can't create context");
        return 1;
    }

    atexit(cleanup);
    jz_context_set_log_cb(jz, jz_log_cb_stderr);
    jz_context_set_log_level(jz, JZ_LOG_NOTICE);

    /* Parse global options */
    --argc, ++argv;
    while(argc > 0 && argv[0][0] == '-') {
        if(!strcmp(*argv, "-h") || !strcmp(*argv, "--help"))
            usage();
        else if(!strcmp(*argv, "-q") || !strcmp(*argv, "--quiet"))
            jz_context_set_log_level(jz, JZ_LOG_ERROR);
        else if(!strcmp(*argv, "-v") || !strcmp(*argv, "--verbose"))
            jz_context_set_log_level(jz, JZ_LOG_DETAIL);
        else if(!strcmp(*argv, "-l") || !strcmp(*argv, "--loglevel")) {
            ++argv;
            if(--argc == 0) {
                jz_log(jz, JZ_LOG_ERROR, "Missing argument to option %s", *argv);
                exit(2);
            }

            enum jz_log_level level;
            if(!strcmp(*argv, "ignore"))
                level = JZ_LOG_IGNORE;
            else if(!strcmp(*argv, "error"))
                level = JZ_LOG_ERROR;
            else if(!strcmp(*argv, "warning"))
                level = JZ_LOG_WARNING;
            else if(!strcmp(*argv, "notice"))
                level = JZ_LOG_NOTICE;
            else if(!strcmp(*argv, "detail"))
                level = JZ_LOG_DETAIL;
            else if(!strcmp(*argv, "debug"))
                level = JZ_LOG_DEBUG;
            else {
                jz_log(jz, JZ_LOG_ERROR, "Invalid log level '%s'", *argv);
                exit(2);
            }

            jz_context_set_log_level(jz, level);
        } else {
            jz_log(jz, JZ_LOG_ERROR, "Invalid global option '%s'", *argv);
            exit(2);
        }

        --argc, ++argv;
    }

    /* Read the device type */
    if(argc == 0) {
        jz_log(jz, JZ_LOG_ERROR, "No device specified (try jztool --help)");
        exit(2);
    }

    dev_info = jz_get_device_info_named(*argv);
    if(!dev_info) {
        jz_log(jz, JZ_LOG_ERROR, "Unknown device '%s' (try jztool --help)", *argv);
        exit(2);
    }

    /* Read the action */
    --argc, ++argv;
    if(argc == 0) {
        jz_log(jz, JZ_LOG_ERROR, "No action specified (try jztool --help)");
        exit(2);
    }

    for(dev_action = 0; dev_action < dev_info->num_actions; ++dev_action)
        if(!strcmp(*argv, dev_info->action_names[dev_action]))
            break;

    if(dev_action == dev_info->num_actions) {
        jz_log(jz, JZ_LOG_ERROR, "Unknown action '%s' (try jztool --help)", *argv);
        exit(2);
    }

    /* Parse the action options */
    action_params = jz_paramlist_new();
    if(!action_params) {
        jz_log(jz, JZ_LOG_ERROR, "Out of memory: can't create paramlist");
        exit(1);
    }

    const char* const* allowed_params = dev_info->action_params[dev_action];

    --argc, ++argv;
    while(argc > 0 && argv[0][0] == '-') {
        if(argv[0][1] != '-') {
            jz_log(jz, JZ_LOG_ERROR, "Invalid option '%s' for action", *argv);
            exit(2);
        }

        bool bad_option = true;
        for(int i = 0; allowed_params[i] != NULL; ++i) {
            if(!strcmp(&argv[0][2], allowed_params[i])) {
                ++argv;
                if(--argc == 0) {
                    jz_log(jz, JZ_LOG_ERROR, "Missing argument for parameter '%s'", *argv);
                    exit(2);
                }

                int rc = jz_paramlist_set(action_params, allowed_params[i], *argv);
                if(rc < 0) {
                    jz_log(jz, JZ_LOG_ERROR, "Out of memory");
                    exit(1);
                }

                bad_option = false;
            }
        }

        if(bad_option) {
            jz_log(jz, JZ_LOG_ERROR, "Invalid option '%s' for action", *argv);
            exit(2);
        }

        --argc, ++argv;
    }

    if(argc != 0) {
        jz_log(jz, JZ_LOG_ERROR, "Excess arguments on command line");
        exit(2);
    }

    /* Invoke action handler */
    int rc = dev_info->action_funcs[dev_action](jz, action_params);
    return (rc < 0) ? 1 : 0;
}
