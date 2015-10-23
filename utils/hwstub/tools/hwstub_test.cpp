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
#include <cstdio>
#include "hwstub.hpp"

libusb_context *g_usb_ctx = nullptr;
hwstub_context *g_ctx = nullptr;

void print_error(hwstub_error err, bool nl = true)
{
    switch(err)
    {
        case HWSTUB_SUCCESS: printf("success"); break;
        case HWSTUB_ERROR: printf("error"); break;
        default: printf("unknown(%d)", (int)err); break;
    }
    if(nl)
        printf("\n");
}

void print_device(hwstub_device *dev)
{
    hwstub_usb_device *udev = dynamic_cast< hwstub_usb_device* >(dev);
    if(udev)
    {
        libusb_device *uudev = udev->native_device();
        struct libusb_device_descriptor dev_desc;
        libusb_get_device_descriptor(uudev, &dev_desc);
        printf("USB device @ %d.%u: ID %04x:%04x\n", libusb_get_bus_number(uudev),
            libusb_get_device_address(uudev), dev_desc.idVendor, dev_desc.idProduct);
    }
    else
        printf("Unknown device\n");
}

void print_list()
{
    std::vector< hwstub_device* > list;
    hwstub_error ret = g_ctx->get_device_list(list);
    if(ret != HWSTUB_SUCCESS)
    {
        printf("Cannot get device list: %d\n", ret);
        return;
    }
    for(size_t i = 0; i < list.size(); i++)
        print_device(list[i]);
    for(auto d : list)
        d->unref();
}

int main(int argc, char **argv)
{
    libusb_init(&g_usb_ctx);
    g_ctx = new hwstub_usb_context(g_usb_ctx);
    print_list();
    /*
    while(true)
    {
        g_ctx->update_list();
    }
    */
    delete g_ctx;
    libusb_exit(g_usb_ctx);

    return 0;
}