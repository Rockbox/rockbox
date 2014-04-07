/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Amaury Pouly
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

struct dev_info_t
{
    uint16_t vendor_id;
    uint16_t product_id;
    unsigned xfer_size;
};

struct dev_info_t g_dev_info[] =
{
    {0x0781, 0x0720, 64}, /* Sandisk E200/C200/View */
    {0x0b70, 0x0003, 64}, /* Portal Player */
};

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

static void put32le(uint8_t *buf, uint32_t i)
{
    *buf++ = i & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 24) & 0xff;
}

static int send_recovery(libusb_device_handle *dev, int xfer_size, uint8_t *data, int size)
{
    // there should be no kernel driver attached but in doubt...
    libusb_detach_kernel_driver(dev, 0);
    libusb_claim_interface(dev, 0);

    uint8_t size_buffer[4];
    put32le(size_buffer, size);
    int xfered;
    int ret = libusb_bulk_transfer(dev, 1, size_buffer, sizeof(size_buffer), &xfered, 1000);
    if(ret < 0 || xfered != sizeof(size_buffer))
    {
        printf("transfer error at init step: %d", ret);
        return 1;
    }

    int sent = 0;
    while(sent < size)
    {
        int xfered;
        int len = MIN(size - sent, xfer_size);
        int ret = libusb_bulk_transfer(dev, 1, data + sent, len, &xfered, 1000);
        if(ret < 0 || xfered != len)
        {
            printf("transfer error at send offset %d: %d\n", sent, ret);
            return 1;
        }
        sent += xfered;
    }
    return 0;
}

static void usage(void)
{
    printf("sbloader [options] file\n");
    printf("options:\n");
    printf("  -h/-?/--help    Display this help\n");
    printf("  -d/--debug      Enable debug output\n");
    printf("  -x <size>       Force transfer size\n");
    printf("  -u <vid>:<pid>  Force USB PID and VID\n");
    printf("  -b <bus>:<dev>  Force USB bus and device\n");
    printf("The following devices are known to this tool:\n");
    for(unsigned i = 0; i < sizeof(g_dev_info) / sizeof(g_dev_info[0]); i++)
    {
        printf("  %04x:%04x (%d bytes/xfer)\n", g_dev_info[i].vendor_id,
            g_dev_info[i].product_id, g_dev_info[i].xfer_size);
    }
    printf("You can select a particular device by USB PID and VID.\n");
    printf("In case this is ambiguous, use bus and device number.\n");
    printf("Protocol is infered if possible and unspecified.\n");
    printf("Transfer size is infered if possible.\n");
    exit(1);
}

static bool dev_match(libusb_device *dev, struct dev_info_t *arg_di,
    int usb_bus, int usb_dev, int *db_idx)
{
    // match bus/dev
    if(usb_bus != -1)
        return libusb_get_bus_number(dev) == usb_bus && libusb_get_device_address(dev) == usb_dev;
    // get device descriptor
    struct libusb_device_descriptor desc;
    if(libusb_get_device_descriptor(dev, &desc))
        return false;
    // match command line vid/pid if specified
    if(arg_di->vendor_id != 0)
        return desc.idVendor == arg_di->vendor_id && desc.idProduct == arg_di->product_id;
    // match known vid/pid
    for(unsigned i = 0; i < sizeof(g_dev_info) / sizeof(g_dev_info[0]); i++)
        if(desc.idVendor == g_dev_info[i].vendor_id && desc.idProduct == g_dev_info[i].product_id)
        {
            if(db_idx)
                *db_idx = i;
            return true;
        }
    return false;
}

static void print_match(libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    if(libusb_get_device_descriptor(dev, &desc))
        printf("????:????");
    else
        printf("%04x:%04x", desc.idVendor, desc.idProduct);
    printf(" @ %d.%d\n", libusb_get_bus_number(dev), libusb_get_device_address(dev));
}

int main(int argc, char **argv)
{
    if(argc <= 1)
        usage();
    struct dev_info_t di = {.vendor_id = 0, .product_id = 0, .xfer_size = 64};
    int usb_bus = -1;
    int usb_dev = -1;
    int force_xfer_size = 0;
    /* parse command line */
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dx:u:b:p:", long_options, NULL);
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
            case 'x':
            {
                char *end;
                force_xfer_size = strtoul(optarg, &end, 0);
                if(*end)
                {
                    printf("Invalid transfer size!\n");
                    exit(2);
                }
                break;
            }
            case 'u':
            {
                char *end;
                di.vendor_id = strtoul(optarg, &end, 16);
                if(*end != ':')
                {
                    printf("Invalid USB PID!\n");
                    exit(3);
                }
                di.product_id = strtoul(end + 1, &end, 16);
                if(*end)
                {
                    printf("Invalid USB VID!\n");
                    exit(4);
                }
                break;
            }
            case 'b':
            {
                char *end;
                usb_bus = strtol(optarg, &end, 0);
                if(*end != ':')
                {
                    printf("Invalid USB bus!\n");
                    exit(5);
                }
                usb_dev = strtol(end, &end, 0);
                if(*end)
                {
                    printf("Invalid USB device!\n");
                    exit(6);
                }
                break;
            }
            default:
                printf("Internal error: unknown option '%c'\n", c);
                abort();
        }
    }

    if(optind + 1 != argc)
        usage();
    const char *filename = argv[optind];
    /* lookup device */
    libusb_init(NULL);
    libusb_set_debug(NULL, 3);
    libusb_device **list;
    ssize_t list_size = libusb_get_device_list(NULL, &list);
    libusb_device_handle *dev = NULL;
    int db_idx = -1;

    {
        libusb_device *mdev = NULL;
        int nr_matches = 0;
        for(int i = 0; i < list_size; i++)
        {
            // match bus/dev if specified
            if(dev_match(list[i], &di, usb_bus, usb_dev, &db_idx))
            {
                mdev = list[i];
                nr_matches++;
            }
        }
        if(nr_matches == 0)
        {
            printf("No device found\n");
            exit(8);
        }
        if(nr_matches > 1)
        {
            printf("Several devices match the specified parameters:\n");
            for(int i = 0; i < list_size; i++)
            {
                // match bus/dev if specified
                if(dev_match(list[i], &di, usb_bus, usb_dev, NULL))
                {
                    printf("  ");
                    print_match(list[i]);
                }
            }
        }
        printf("Device: ");
        print_match(mdev);
        libusb_open(mdev, &dev);
    }
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        return 1;
    }
    /* get protocol */
    int xfer_size = di.xfer_size;
    if(db_idx >= 0)
        xfer_size = g_dev_info[db_idx].xfer_size;
    if(force_xfer_size > 0)
        xfer_size = force_xfer_size;
    /* open file */
    FILE *f = fopen(filename, "r");
    if(f == NULL)
    {
        perror("Cannot open file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    printf("Transfer size: %d\n", xfer_size);
    uint8_t *file_buf = malloc(size);
    if(fread(file_buf, size, 1, f) != 1)
    {
        perror("read error");
        fclose(f);
        return 1;
    }
    fclose(f);
    /* send file */
    return send_recovery(dev, xfer_size, file_buf, size);
}

 
