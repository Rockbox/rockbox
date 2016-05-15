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
#include <stdbool.h>

#define VR_GET_CPU_INFO         0
#define VR_SET_DATA_ADDRESS     1
#define VR_SET_DATA_LENGTH      2
#define VR_FLUSH_CACHES         3
#define VR_PROGRAM_START1       4
#define VR_PROGRAM_START2       5

bool g_verbose = false;

int jz_cpuinfo(libusb_device_handle *dev)
{
    if(g_verbose)
        printf("Get CPU Info...\n");
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
    if(g_verbose)
        printf("Set address to 0x%lx...\n", addr);
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_ADDRESS, addr >> 16, addr & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot set address: %d\n", ret);
    return ret;
}

int jz_set_length(libusb_device_handle *dev, unsigned long length)
{
    if(g_verbose)
        printf("Set length to 0x%lx...\n", length);
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_LENGTH, length >> 16, length & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot set length: %d\n", ret);
    return ret;
}

int jz_start1(libusb_device_handle *dev, unsigned long addr)
{
    if(g_verbose)
        printf("Start 1 at 0x%lx...\n", addr);
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START1, addr >> 16, addr & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot start1: %d\n", ret);
    return ret;
}

int jz_start2(libusb_device_handle *dev, unsigned long addr)
{
    if(g_verbose)
        printf("Start 2 at 0x%lx...\n", addr);
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START2, addr >> 16, addr & 0xffff, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot start2: %d\n", ret);
    return ret;
}

int jz_flush_caches(libusb_device_handle *dev)
{
    if(g_verbose)
        printf("Flush caches...\n");
    int ret = libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_FLUSH_CACHES, 0, 0, NULL, 0, 1000);
    if(ret != 0)
        printf("Cannot flush caches: %d\n", ret);
    return ret;
}

int jz_upload(libusb_device_handle *dev, const char *file, unsigned long length)
{
    if(g_verbose)
        printf("Upload %lu bytes...\n", length);
    void *data = malloc(length);
    int xfered;
    int ret = libusb_bulk_transfer(dev, LIBUSB_ENDPOINT_IN | 1, data, length,
        &xfered, 10000);
    if(ret != 0)
        printf("Cannot upload data from device: %d\n", ret);
    if(ret == 0 && xfered != length)
    {
        printf("Device did not send all the data\n");
        ret = -1;
    }
    if(ret == 0)
    {
        FILE *f = fopen(file, "wb");
        if(f != NULL)
        {
            if(fwrite(data, length, 1, f) != 1)
            {
                printf("Cannot write file\n");
                ret = -3;
            }
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

int jz_download(libusb_device_handle *dev, const char *file)
{
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        printf("Cannot open file for reading\n");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(g_verbose)
        printf("Download %lu bytes..\n", length);
    void *data = malloc(length);
    if(fread(data, length, 1, f) != 1)
    {
        printf("Cannot read file\n");
        free(data);
        fclose(f);
        return -1;
    }
    fclose(f);
    int xfered;
    int ret = libusb_bulk_transfer(dev, LIBUSB_ENDPOINT_OUT | 1, data, length,
        &xfered, 1000);
    if(ret != 0)
        printf("Cannot download data from device: %d\n", ret);
    if(ret == 0 && xfered != length)
    {
        printf("Device did not receive all the data\n");
        ret = -1;
    }
    free(data);
    return ret;
}

int renumerate(libusb_device_handle **dev)
{
    if(g_verbose)
        printf("Look for device again...\n");
    libusb_close(*dev);
    *dev = libusb_open_device_with_vid_pid(NULL, 0x601a, 0x4760);
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        return -1;
    }
    return 0;
}

int jz_stage1(libusb_device_handle *dev, unsigned long addr, const char *file)
{
    int ret = jz_set_addr(dev, addr);
    if(ret != 0)
        return ret;
    ret = jz_download(dev, file);
    if(ret != 0)
        return ret;
    return jz_start1(dev, addr);
}

void usage()
{
    printf("Usage: usbboot [options]\n");
    printf("\n");
    printf("Basic options:\n");
    printf("  --stage1 <file>     Upload first stage program (<=16Kio)\n");
    printf("  --s1-addr <addr>    Change first stage address (default is 0x80000000)\n");
    printf("  --stage2 <file>     Upload second stage program to SDRAM\n");
    printf("  --s2-addr <addr>    Change second stage address (default is 0x80000000)\n");
    printf("  --ram <target>      Setup SDRAM for <target>, see list below\n");
    printf("\n");
    printf("Advanced options:\n");
    printf("  --addr <addr>       Set address for next operation\n");
    printf("  --length <len>      Set length for next operation\n");
    printf("  --download <file>   Download data in file to the device (use file length)\n");
    printf("  --upload <file>     Upload data from the device to the file\n");
    printf("  --cpuinfo           Print CPU info\n");
    printf("  --flush-caches      Flush CPU caches\n");
    printf("  --start1 <addr>     Execute first stage from I-cache\n");
    printf("  --start2 <addr>     Execute second stage\n");
    printf("  --wait <time>       Wait <time> seconds\n");
    printf("  --renumerate        Try to look for device again\n");
    printf("  --ram <ramopt>      Setup SDRAM with parameters, see descrition below\n");
    printf("  -v                  Be verbose\n");
    printf("\n");
    printf("Targets for RAM setup:\n");
    printf("  fiiox1              Use <>\n");
    printf("\n");
    printf("Format of <ramopt> for RAM setup:\n");
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
        return -1;
    }
    if(libusb_claim_interface(dev, 0) != 0)
    {
        printf("Cannot claim interface\n");
        libusb_close(dev);
        return -2;
    }

    enum
    {
        OPT_ADDR = 0x100, OPT_LENGTH, OPT_UPLOAD, OPT_CPUINFO, OPT_DOWNLOAD,
        OPT_START1, OPT_WAIT, OPT_RENUMERATE, OPT_START2, OPT_FLUSH_CACHES,
        OPT_S1_ADDR, OPT_STAGE1
    };
    unsigned long last_length = 0;
    unsigned long s1_addr = 0x80000000;
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"cpuinfo", no_argument, 0, OPT_CPUINFO},
            {"addr", required_argument, 0, OPT_ADDR},
            {"length", required_argument, 0, OPT_LENGTH},
            {"upload", required_argument, 0, OPT_UPLOAD},
            {"download", required_argument, 0, OPT_DOWNLOAD},
            {"start1", required_argument, 0, OPT_START1},
            {"wait", required_argument, 0, OPT_WAIT},
            {"renumerate", no_argument, 0, OPT_RENUMERATE},
            {"start2", required_argument, 0, OPT_START2},
            {"flush-caches", no_argument, 0, OPT_FLUSH_CACHES},
            {"s1-addr", required_argument, 0, OPT_S1_ADDR},
            {"stage1", required_argument, 0, OPT_STAGE1},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hv", long_options, NULL);
        char *end = 0;
        unsigned long param;
        if(c == OPT_ADDR || c == OPT_LENGTH || c == OPT_START1 || c == OPT_WAIT
                || c == OPT_S1_ADDR)
        {
            param = strtoul(optarg, &end, 0);
            if(*end)
            {
                printf("Invalid argument '%s'\n", optarg);
                ret = 1;
                break;
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
            case 'v':
                g_verbose = true;
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
            case OPT_DOWNLOAD:
                ret = jz_download(dev, optarg);
                break;
            case OPT_CPUINFO:
                ret = jz_cpuinfo(dev);
                break;
            case OPT_START1:
                ret = jz_start1(dev, param);
                break;
            case OPT_WAIT:
                if(g_verbose)
                    printf("Wait for %lu seconds...\n", param);
                sleep(param);
                break;
            case OPT_RENUMERATE:
                ret = renumerate(&dev);
                break;
            case OPT_START2:
                ret = jz_start2(dev, param);
                break;
            case OPT_FLUSH_CACHES:
                ret = jz_flush_caches(dev);
                break;
            case OPT_S1_ADDR:
                s1_addr = param;
                break;
            case OPT_STAGE1:
                ret = jz_stage1(dev, s1_addr, optarg);
                break;
        }
        if(ret != 0)
            break;
    }
    if(optind != argc)
    {
        printf("Error: extra arguments on command line\n");
        ret = 1;
    }

    libusb_close(dev);
    libusb_exit(NULL);
    return ret;
}
