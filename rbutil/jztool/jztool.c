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

void usage_fiiom3k(void)
{
    printf("Usage:\n"
           "  jztool fiiom3k load <bootloader.m3k>\n"
           "\n"
           "The 'load' command is used to boot the Rockbox bootloader in\n"
           "recovery mode, which allows you to install the Rockbox bootloader\n"
           "and backup or restore bootloader images. You need to connect the\n"
           "M3K in USB boot mode in order to use this tool.\n"
           "\n"
           "On Windows, you will need to install the WinUSB driver for the M3K\n"
           "using a 3rd-party tool such as Zadig <https://zadig.akeo.ie>. For\n"
           "more details check the jztool README.md file or the Rockbox wiki at\n"
           "<https://rockbox.org/wiki/FiioM3K>.\n"
           "\n"
           "To connect the M3K in USB boot mode, plug the microUSB into the\n"
           "M3K, and hold the VOL- button while plugging the USB into your\n"
           "computer. If successful, the button light will turn on and the\n"
           "LCD will remain black. If you encounter any errors and need to\n"
           "reconnect the device, you must force a power off by holding POWER\n"
           "for more than 10 seconds.\n"
           "\n"
           "Once the Rockbox bootloader is installed on your M3K, you can\n"
           "access the recovery menu by holding VOL+ while powering on the\n"
           "device.\n");
    exit(4);
}

int cmdline_fiiom3k(int argc, char** argv)
{
    if(argc < 2 || strcmp(argv[0], "load")) {
        usage_fiiom3k();
        return 2;
    }

    int rc = jz_usb_open(jz, &usbdev, dev_info->vendor_id, dev_info->product_id);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Cannot open USB device: %d", rc);
        return 1;
    }

    rc = jz_fiiom3k_boot(usbdev, argv[1]);
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
    int n = jz_get_num_device_info();
    for(int i = 0; i < n; ++i) {
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

    /* Dispatch to device handler */
    --argc, ++argv;
    switch(dev_info->device_type) {
    case JZ_DEVICE_FIIOM3K:
        return cmdline_fiiom3k(argc, argv);

    default:
        jz_log(jz, JZ_LOG_ERROR, "INTERNAL ERROR: unhandled device type");
        return 1;
    }
}
