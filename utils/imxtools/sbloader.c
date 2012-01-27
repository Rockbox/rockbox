/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Amaury Pouly
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

void put32le(uint8_t *buf, uint32_t i)
{
    *buf++ = i & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 24) & 0xff;
}

void put32be(uint8_t *buf, uint32_t i)
{
    *buf++ = (i >> 24) & 0xff;
    *buf++ = (i >> 16) & 0xff;
    *buf++ = (i >> 8) & 0xff;
    *buf++ = i & 0xff;
}

struct dev_info_t
{
    uint16_t vendor_id;
    uint16_t product_id;
    unsigned xfer_size;
};

struct dev_info_t g_dev_info[] =
{
    {0x066f, 0x3780, 1024}, /* i.MX233 / STMP3780 */
    {0x066f, 0x3770, 48}, /* STMP3770 */
    {0x15A2, 0x004F, 1024}, /* i.MX28 */
};

int main(int argc, char **argv)
{
    int ret;
    FILE *f;
    int i, xfer_size, nr_xfers, recv_size;

    if(argc != 3)
    {
        printf("usage: %s <xfer size> <file>\n", argv[0]);
        printf("If <xfer size> is set to zero, the preferred one is used.\n");
        return 1;
    }

    char *end;
    xfer_size = strtol(argv[1], &end, 0);
    if(end != (argv[1] + strlen(argv[1])))
    {
        printf("Invalid transfer size !\n");
        return 1;
    }
    
    libusb_device_handle *dev;
    
    libusb_init(NULL);
    
    libusb_set_debug(NULL, 3);

    for(unsigned i = 0; i < sizeof(g_dev_info) / sizeof(g_dev_info[0]); i++)
    {
        dev = libusb_open_device_with_vid_pid(NULL,
            g_dev_info[i].vendor_id, g_dev_info[i].product_id);
        if(dev == NULL)
            continue;
        if(xfer_size == 0)
            xfer_size = g_dev_info[i].xfer_size;
        printf("Found a match for %04x:%04x\n",
            g_dev_info[i].vendor_id, g_dev_info[i].product_id);
        break;
    }
    if(dev == NULL)
    {
        printf("Cannot open device\n");
        return 1;
    }
    
    libusb_detach_kernel_driver(dev, 0);
    libusb_detach_kernel_driver(dev, 4);
    
    libusb_claim_interface (dev, 0);
    libusb_claim_interface (dev, 4);
    
    if(!dev)
    {
        printf("No dev\n");
        exit(1);
    }

    f = fopen(argv[2], "r");
    if(f == NULL)
    {
        perror("cannot open file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    printf("Transfer size: %d\n", xfer_size);
    nr_xfers = (size + xfer_size - 1) / xfer_size;
    uint8_t *file_buf = malloc(nr_xfers * xfer_size);
    memset(file_buf, 0xff, nr_xfers * xfer_size); // pad with 0xff
    if(fread(file_buf, size, 1, f) != 1)
    {
        perror("read error");
        fclose(f);
        return 1;
    }
    fclose(f);
    
    uint8_t *xfer_buf = malloc(1 + xfer_size);
    uint8_t *p = xfer_buf;
    
    *p++ = 0x01;         /* Report id */
    
    /* Command block wrapper */
    *p++ = 'B';          /* Signature */
    *p++ = 'L';
    *p++ = 'T';
    *p++ = 'C';
    put32le(p, 0x1);     /* Tag */
    p += 4;
    put32le(p, size);    /* Payload size */
    p += 4;
    *p++ = 0;            /* Flags (host to device) */
    p += 2;              /* Reserved */
    
    /* Command descriptor block */
    *p++ = 0x02;         /* Firmware download */
    put32be(p, size);    /* Download size */
    
    ret = libusb_control_transfer(dev, 
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x9, 0x201, 0,
        xfer_buf, xfer_size + 1, 1000);
    if(ret < 0)
    {
        printf("transfer error at init step\n");
        return 1;
    }

    for(i = 0; i < nr_xfers; i++)
    {
        xfer_buf[0] = 0x2;
        memcpy(&xfer_buf[1], &file_buf[i * xfer_size], xfer_size);
        
        ret = libusb_control_transfer(dev, 
            LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 
            0x9, 0x202, 0, xfer_buf, xfer_size + 1, 1000);
        if(ret < 0)
        {
            printf("transfer error at send step %d\n", i);
            return 1;
        }
    }

    ret = libusb_interrupt_transfer(dev, 0x81, xfer_buf, xfer_size, &recv_size,
        1000);
    if(ret < 0)
    {
        printf("transfer error at final stage\n");
        return 1;
    }
    
    printf("ret %i\n", ret);
    
    return 0;
}

