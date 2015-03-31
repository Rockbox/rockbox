/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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
#include <stdlib.h>
#include <stdint.h>
#include <libusb.h>
#include <getopt.h>

#define VR_GET_CPU_INFO         0
#define VR_SET_DATA_ADDRESS     1
#define VR_SET_DATA_LENGTH      2
#define VR_FLUSH_CACHES         3
#define VR_PROGRAM_START1       4
#define VR_PROGRAM_START2       5

int jz_cpuinfo(libusb_device_handle *dev)
{
    uint8_t cpuinfo[9];
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_GET_CPU_INFO, 0, 0, cpuinfo, 8, 1000);
    if(ret != 8)
    {
        printf("Cannot get CPU info: %d\n", ret);
        return ret;
    }
    cpuinfo[8] = 0;
    printf("CPU Info: %s\n", cpuinfo);
    return 0;
}

int jz_set_addr(libusb_device_handle *dev, unsigned long addr)
{
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_ADDRESS, addr >> 16, addr & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot set address: %d\n", ret);
    return ret;
}

int jz_set_length(libusb_device_handle *dev, unsigned long length)
{
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_LENGTH, length >> 16, length & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot set length: %d\n", ret);
    return ret;
}

int jz_upload(libusb_device_handle *dev, const char *file, unsigned long length)
{
    void *data = malloc(length);
    int xfered;
    int ret = libusb_bulk_transfer(dev, LIBUSB_ENDPOINT_IN | 1, data, length,
        &xfered, 1000);
    if(ret != 0)
        printf("Cannot upload data from device: %d\n", ret);
    if(xfered != length)
    {
        printf("Device did not send all the data\n");
        ret = -1;
    }
    if(ret == 0)
    {
        FILE *f = fopen(file, "wb");
        if(f != NULL)
        {
            fwrite(data, 1, length, f);
            fclose(f);
        }
        else
        {
            printf("Cannot open file for writing\n");
            ret = -2;
        }
    }
    free(data);
    return ret;
}

void usage()
{
    printf("usage: usbboot [options]\n");
    printf("\n");
    printf("basic options:\n");
    printf("  --stage1 <file>     Upload first stage program (<=16Kio)\n");
    printf("  --stage2 <file>     Upload second stage program to SDRAM\n");
    printf("  --s2-addr <addr>    Change second stage address (default is )\n");
    printf("advanced options:\n");
    printf("  --addr <addr>       Set address for next operation\n");
    printf("  --length <len>      Set length for next operation\n");
    printf("  --download <file>   Download data in file to the device (use file length)\n");
    printf("  --download2 <file>  Download data in file to the device (use length option)\n");
    printf("  --upload <file>     Upload data from the device to the file\n");
    printf("  --cpuinfo           Print CPU info\n");
    printf("  --flush-caches      Flush CPU caches\n");
    printf("  --start1 <addr>     Execute first stage from I-cache\n");
    printf("  --start2 <addr>     Execute second stage\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc <= 1)
        usage();
    int ret = 0;
    libusb_init(NULL);
    libusb_device_handle *dev = libusb_open_device_with_vid_pid(NULL, 0x601a, 0x4760);
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        goto Lret;
    }

    enum
    {
        OPT_ADDR = 0x100, OPT_LENGTH, OPT_UPLOAD, OPT_CPUINFO,
    };
    unsigned long last_length = 0;
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"cpuinfo", no_argument, 0, OPT_CPUINFO},
            {"addr", required_argument, 0, OPT_ADDR},
            {"length", required_argument, 0, OPT_LENGTH},
            {"upload", required_argument, 0, OPT_UPLOAD},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "h", long_options, NULL);
        char *end = 0;
        unsigned long param;
        if(c == OPT_ADDR || c == OPT_LENGTH)
        {
            param = strtoul(optarg, &end, 0);
            if(*end)
            {
                printf("Invalid argument '%s'\n", optarg);
                ret = 1;
                goto Lret;
            }
        }
        if(c == -1)
            break;
        switch(c)
        {
            default:
            case -1:
                break;
            case 'h':
                usage();
                break;
            case OPT_ADDR:
                ret = jz_set_addr(dev, param);
                break;
            case OPT_LENGTH:
                last_length = param;
                ret = jz_set_length(dev, param);
                break;
            case OPT_UPLOAD:
                ret = jz_upload(dev, optarg, last_length);
                break;
            case OPT_CPUINFO:
                ret = jz_cpuinfo(dev);
                break;
        }
        if(ret != 0)
            goto Lret;
    }
    if(optind != argc)
    {
        printf("Error: extra arguments on command line\n");
        ret = 1;
        goto Lret;
    }

Lret:
    libusb_close(dev);
    libusb_exit(NULL);
    return ret;
}