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
#include "hwstub.hpp"

/**
 * Context
 */

hwstub_usb_context::hwstub_usb_context(libusb_context *ctx)
    :m_usb_ctx(ctx)
{
}

hwstub_usb_context::~hwstub_usb_context()
{
}

bool hwstub_usb_context::has_hotplug()
{
    return false;
}

libusb_context *hwstub_usb_context::native_context()
{
    return m_usb_ctx;
}


hwstub_error hwstub_usb_context::fetch_device_list(std::vector< hwstub_ctx_dev_t >& list, void*& ptr)
{
    libusb_device **usb_list;
    ssize_t ret = libusb_get_device_list(m_usb_ctx, &usb_list);
    if(ret < 0)
        return HWSTUB_ERROR;
    ptr = (void *)usb_list;
    list.resize(ret);
    for(int i = 0; i < ret; i++)
        list[i] = usb_list[i];
    return HWSTUB_SUCCESS;
}

void hwstub_usb_context::destroy_device_list(void *ptr)
{
    /* remove all references */
    libusb_free_device_list((libusb_device **)ptr, 1);
}

hwstub_error hwstub_usb_context::create_device(hwstub_ctx_dev_t dev, hwstub_device*& hwdev)
{
    hwdev = new hwstub_usb_device(this, (libusb_device *)dev);
    return HWSTUB_SUCCESS;
}

bool hwstub_usb_context::match_device(hwstub_ctx_dev_t dev, hwstub_device *hwdev)
{
    hwstub_usb_device *udev = dynamic_cast< hwstub_usb_device* >(hwdev);
    return udev != nullptr && udev->native_device() == dev;
}

/**
 * Device
 */
hwstub_usb_device::hwstub_usb_device(hwstub_context *ctx, libusb_device *dev)
    :hwstub_device(ctx), m_dev(dev)
{
    libusb_ref_device(dev);
}

hwstub_usb_device::~hwstub_usb_device()
{
    libusb_unref_device(m_dev);
}

libusb_device *hwstub_usb_device::native_device()
{
    return m_dev;
}

hwstub_error hwstub_usb_device::do_open(hwstub_handle*& handle)
{
    return HWSTUB_ERROR;
}

bool hwstub_usb_device::has_multiple_open()
{
    return false;
}
