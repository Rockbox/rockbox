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
#include "hwstub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>

struct player_info_t
{
    const char *name;
    const char *username;
    int modelnum;
};

enum image_type_t
{
    IT_RAW,
    IT_ROCKBOX,
    IT_DETECT,
    /* positive values reserved for rockbox-specific models */
};

struct player_info_t players[] =
{
    { "zenv", "Zen V", 85 },
    { "zmoz", "Zen Mozaic", 87 },
    { "zen", "Zen", 90 },
    { "zxfi", "Zen X-Fi", 86 },
    { NULL, 0 },
};

enum image_type_t detect_type(unsigned char *buffer, size_t size)
{
    if(size < 8)
        return IT_RAW;
    int player;
    for(player = 0; players[player].name; player++)
        if(memcmp(buffer + 4, players[player].name, 4) == 0)
            break;
    if(players[player].name == NULL)
        return IT_RAW;
    unsigned long checksum = players[player].modelnum;
    for(size_t i = 8; i < size; i++)
        checksum += buffer[i];
    unsigned long expected = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    if(checksum != expected)
        return IT_RAW;
    return IT_ROCKBOX;
}

const char *get_player_name(unsigned char *buffer)
{
    for(int player = 0; players[player].name; player++)
        if(memcmp(buffer, players[player].name, 4) == 0)
            return players[player].username;
    return NULL;
}

bool could_be_rockbox(unsigned char *buffer, size_t size)
{
    /* usually target use 3 or 4 digits */
    if(size >= 8 && isprint(buffer[4]) && isprint(buffer[5]) && isprint(buffer[6]) &&
        (isprint(buffer[7]) || buffer[7] == 0))
    {
        unsigned long checksum = 0;
        for(size_t i = 8; i < size; i++)
            checksum += buffer[i];
        unsigned long expected = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
        unsigned long expected_modelnm = expected - checksum;
        if(expected_modelnm < 150)
            fprintf(stderr, "This file looks like a valid rockbox image but I don't know this player: %.4s (modelnum=%ld)\n",
                buffer + 4, expected_modelnm);
        else
            fprintf(stderr, "This file could be a valid rockbox image but I don't know this player and the checksum is strange: %.4s\n",
                buffer + 4);
        return true;
    }
    else
        return false;
}

void usage(void)
{
    printf("usage: hwstub_load [options] <addr> <file>\n");
    printf("options:\n");
    printf("  --help/-?     Display this help\n");
    printf("  --quiet/-q    Quiet output\n");
    printf("  --type/-t <t> Override file type\n");
    printf("file types:\n");
    printf("  raw      Load a raw binary blob\n");
    printf("  rockbox  Load a rockbox image produced by scramble\n");
    printf("  detect   Try to guess the format\n");
    printf("known players:");
    for(int i = 0; players[i].name; i++)
        printf(" %s", players[i].name);
    printf("\n");
    exit(1);
}

int main(int argc, char **argv)
{
    bool quiet = false;
    struct hwstub_device_t *hwdev;
    enum image_type_t type = IT_DETECT;

    // parse command line
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"quiet", no_argument, 0, 'q'},
            {"type", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?qt:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'q':
                quiet = true;
                break;
            case '?':
                usage();
                break;
            case 't':
                if(strcmp(optarg, "raw") == 0)
                    type = IT_RAW;
                else if(strcmp(optarg, "rockbox") == 0)
                    type = IT_ROCKBOX;
                else if(strcmp(optarg, "detect") == 0)
                    type = IT_DETECT;
                else
                {
                    fprintf(stderr, "Unknown file type '%s'\n", optarg);
                    return 1;
                }
                break;
            default:
                abort();
        }
    }

    if(optind + 2 != argc)
        usage();

    char *end;
    unsigned long addr = strtoul(argv[optind], &end, 0);
    if(*end)
    {
        fprintf(stderr, "Invalid load address\n");
        return 2;
    }

    FILE *f = fopen(argv[optind + 1], "rb");
    if(f == NULL)
    {
        fprintf(stderr, "Cannot open file for reading: %m\n");
        return 3;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buffer = (unsigned char*)malloc(size);
    fread(buffer, size, 1, f);
    fclose(f);

    if(type == IT_ROCKBOX || type == IT_DETECT)
    {
        enum image_type_t det = detect_type(buffer, size);
        if(type == IT_ROCKBOX && det != IT_ROCKBOX)
        {
            if(!could_be_rockbox(buffer, size))
                fprintf(stderr, "This file does not appear to be valid rockbox image.\n");
            return 4;
        }
        if(type == IT_DETECT && det == IT_RAW)
            could_be_rockbox(buffer, size);
        type = det;
        if(type == IT_ROCKBOX)
        {
            if(!quiet)
                printf("Rockox image is for player %s (%.4s)\n", get_player_name(buffer + 4), buffer + 4);
            memmove(buffer, buffer + 8, size - 8);
            size -= 8;
        }
    }

    if(!quiet)
    {
        if(type == IT_RAW)
            printf("Loading raw image at %#lx\n", addr);
        else
            printf("Loading rockbox image at %#lx\n", addr);
    }

    // create usb context
    libusb_context *ctx;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    // look for device
    if(!quiet)
        printf("Looking for device %#04x:%#04x...\n", HWSTUB_USB_VID, HWSTUB_USB_PID);

    libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx,
        HWSTUB_USB_VID, HWSTUB_USB_PID);
    if(handle == NULL)
    {
        fprintf(stderr, "No device found\n");
        return 1;
    }

    // admin stuff
    libusb_device *mydev = libusb_get_device(handle);
    if(!quiet)
    {
        printf("device found at %d:%d\n",
            libusb_get_bus_number(mydev),
            libusb_get_device_address(mydev));
    }
    hwdev = hwstub_open(handle);
    if(hwdev == NULL)
    {
        fprintf(stderr, "Cannot probe device!\n");
        return 1;
    }

    // get hwstub information
    struct hwstub_version_desc_t hwdev_ver;
    int ret = hwstub_get_desc(hwdev, HWSTUB_DT_VERSION, &hwdev_ver, sizeof(hwdev_ver));
    if(ret != sizeof(hwdev_ver))
    {
        fprintf(stderr, "Cannot get version!\n");
        goto Lerr;
    }
    if(hwdev_ver.bMajor != HWSTUB_VERSION_MAJOR || hwdev_ver.bMinor < HWSTUB_VERSION_MINOR)
    {
        printf("Warning: this tool is possibly incompatible with your device:\n");
        printf("Device version: %d.%d.%d\n", hwdev_ver.bMajor, hwdev_ver.bMinor, hwdev_ver.bRevision);
        printf("Host version: %d.%d\n", HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    }

    ret = hwstub_rw_mem(hwdev, 0, addr, buffer, size);
    if(ret != (int)size)
    {
        fprintf(stderr, "Image write failed: %d\n", ret);
        goto Lerr;
    }
    hwstub_jump(hwdev, addr);

    hwstub_release(hwdev);
    return 0;

    Lerr:
    // display log if handled
    fprintf(stderr, "Device log:\n");
    do
    {
        char buffer[128];
        int length = hwstub_get_log(hwdev, buffer, sizeof(buffer) - 1);
        if(length <= 0)
            break;
        buffer[length] = 0;
        fprintf(stderr, "%s", buffer);
    }while(1);
    hwstub_release(hwdev);
    return 1;
}
 
