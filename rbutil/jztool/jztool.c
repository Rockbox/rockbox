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
jz_usbdev* usbdev = NULL;
const jz_device_info* dev_info = NULL;
const jz_cpu_info* cpu_info = NULL;

void usage_x1000(void)
{
    printf(
"Usage:\n"
"  jztool fiiom3k load <bootloader.m3k>\n"
"  jztool shanlingq1 load <bootloader.q1>\n"
"  jztool erosq load <bootloader.erosq>\n"
"\n"
"The 'load' command is used to boot the Rockbox bootloader in recovery\n"
"mode, which allows you to install the Rockbox bootloader and backup or\n"
"restore bootloader images. You need to connect your player in USB boot\n"
"mode in order to use this tool.\n"
"\n"
"To connect the player in USB boot mode, follow these steps:\n"
"\n"
"1. Ensure the player is fully powered off.\n"
"2. Plug one end of the USB cable into your player.\n"
"3. Hold down your player's USB boot key (see below).\n"
"4. Plug the other end of the USB cable into your computer.\n"
"5. Let go of the USB boot key.\n"
"\n"
"USB boot keys:\n"
"\n"
"  FiiO M3K    - Volume Down\n"
"  Shanling Q1 - Play\n"
"  Eros Q      - Menu\n"
"\n"
"Not all players give a visible indication that they are in USB boot mode.\n"
"If you're having trouble connecting your player, try resetting it by\n"
"holding the power button for 10 seconds, and try the above steps again.\n"
"\n"
"Note for Windows users: you need to install the WinUSB driver using a\n"
"3rd-party tool such as Zadig <https://zadig.akeo.ie> before this tool\n"
"can access your player in USB boot mode. You need to run Zadig while the\n"
"player is plugged in and in USB boot mode. For more details check the\n"
"jztool README.md file or visit <https://rockbox.org/wiki/IngenicX1000>.\n"
"\n");

    exit(4);
}

int cmdline_x1000(int argc, char** argv)
{
    if(argc < 2 || strcmp(argv[0], "load")) {
        usage_x1000();
        return 2;
    }

    int rc = jz_usb_open(jz, &usbdev, cpu_info->vendor_id, cpu_info->product_id);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Cannot open USB device: %d", rc);
        return 1;
    }

    rc = jz_x1000_boot(usbdev, dev_info->device_type, argv[1]);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Boot failed: %d", rc);
        return 1;
    }

    return 0;
}

void usage(void)
{
    printf("Usage:\n"
           "  jztool [global options] <device> <command> [command arguments]\n"
           "\n"
           "Global options:\n"
           "  -h, --help            Display this help\n"
           "  -q, --quiet           Don't log anything except errors\n"
           "  -v, --verbose         Display detailed logging output\n\n");

    printf("Supported devices:\n\n");
    for(int i = 0; i < JZ_NUM_DEVICES; ++i) {
        const jz_device_info* info = jz_get_device_info_indexed(i);
        printf("  %s - %s\n", info->name, info->description);
    }

    printf("\n"
           "For device-specific help run 'jztool DEVICE' without arguments,\n"
           "eg. 'jztool fiiom3k' will display help for the FiiO M3K.\n");

    exit(4);
}

void cleanup(void)
{
    if(usbdev)
        jz_usb_close(usbdev);
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

    cpu_info = jz_get_cpu_info(dev_info->cpu_type);

    /* Dispatch to device handler */
    --argc, ++argv;
    switch(dev_info->device_type) {
    case JZ_DEVICE_FIIOM3K:
    case JZ_DEVICE_SHANLINGQ1:
    case JZ_DEVICE_EROSQ:
        return cmdline_x1000(argc, argv);

    default:
        jz_log(jz, JZ_LOG_ERROR, "INTERNAL ERROR: unhandled device type");
        return 1;
    }
}
