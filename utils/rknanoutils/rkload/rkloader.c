/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Amaury Pouly
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
#include <string.h>
#include <libusb.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

bool g_debug = false;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* shamelessly copied from rk27xx utils */
static void encode_page(uint8_t *inpg, uint8_t *outpg, const int size)
{
    uint8_t key[] =
    {
        0x7C, 0x4E, 0x03, 0x04,
        0x55, 0x05, 0x09, 0x07,
        0x2D, 0x2C, 0x7B, 0x38,
        0x17, 0x0D, 0x17, 0x11
    };
    int i, i3, x, val, idx;

    uint8_t key1[0x100];
    uint8_t key2[0x100];

    for (i=0; i < 0x100; i++)
    {
        key1[i] = i;
        key2[i] = key[i & 0xf];
    }

    i3 = 0;
    for (i=0; i < 0x100; i++)
    {
        x = key1[i];
        i3 = key1[i] + i3;
        i3 += key2[i];
        i3 &= 0xff;
        key1[i] = key1[i3];
        key1[i3] = x;
    }

    idx = 0;
    for (i=0; i < size; i++)
    {
        x = key1[(i + 1) & 0xff];
        val = x;
        idx = (x + idx) & 0xff;
        key1[(i + 1) & 0xff] = key1[idx];
        key1[idx] = (x & 0xff);
        val = (key1[(i + 1)&0xff] + x) & 0xff;
        val = key1[val];
        outpg[i] = val ^ inpg[i];
    }
}

static uint16_t compute_crc(uint8_t *buf, int size)
{
    uint16_t result = 65535;
    for(; size; buf++, size--)
    {
        for(int bit = 128; bit; bit >>= 1)
        {
            if(result & 0x8000)
                result = (2 * result) ^ 0x1021;
            else
                result *= 2;
            if(*buf & bit)
                result ^= 0x1021;
        }
    }
    return result;
}

int send_dfu(libusb_device_handle *dev, void *buffer, long size)
{
    /* FIXME I never tried but if the image size is a multiple of 4096 we
     * probably need to send a last zero length packet for the DFU to boot */
    while(size >= 0)
    {
        int xfer = MIN(size, 4096);
        if(g_debug)
            printf("[rkloader] send %d bytes\n", xfer);
        int ret = libusb_control_transfer(dev,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 12, 0, 1137,
            buffer, xfer, 1000);
        if(ret < 0)
        {
            fprintf(stderr, "transfer error: %d\n", ret);
            return 1;
        }
        buffer += xfer;
        size -= xfer;
        if(xfer != 4096)
            break;
    }

    return 0;
}

static void usage(void)
{
    printf("usage: rkload [options] <file>\n");
    printf("options:\n");
    printf("  --help/-?   Display this help\n");
    printf("  --debug/-d  Enable debug output\n");
    printf("  --encode/-e Encode file before sending it\n");
}

int main(int argc, char **argv)
{
    bool encode = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"encode", no_argument, 0, 'e'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?de", long_options, NULL);
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
            case 'e':
                encode = true;
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

    libusb_init(NULL);
    libusb_set_debug(NULL, 3);
    if(g_debug)
        printf("[rkloader] opening device...\n");
    libusb_device_handle *dev = libusb_open_device_with_vid_pid(NULL, 0x071b, 0x3226);
    if(dev == NULL)
        return fprintf(stderr, "No device found\n");

    if(g_debug)
        printf("[rkloader] loading file...\n");
    FILE *f = fopen(argv[optind], "rb");
    if(f == NULL)
        return fprintf(stderr, "Cannot open file for reading: %m\n");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    /* allocate two more bytes for the crc */
    void *buffer = malloc(size + 2);
    fread(buffer, size, 1, f);
    fclose(f);
    /* encode buffer if needed */
    if(encode)
    {
        if(g_debug)
            printf("[rkloader] encoding buffer...\n");
        encode_page(buffer, buffer, size);
    }
    /* compute crc */
    if(g_debug)
        printf("[rkloader] computing crc...\n");
    uint16_t crc = compute_crc(buffer, size);
    *(uint8_t *)(buffer + size) = crc >> 8;
    *(uint8_t *)(buffer + size + 1) = crc & 0xff;

    /* send buffer */
    if(g_debug)
        printf("[rkloader] sending buffer...\n");
    int ret = send_dfu(dev, buffer, size + 2);

    free(buffer);
    libusb_close(dev);

    return ret;
}

