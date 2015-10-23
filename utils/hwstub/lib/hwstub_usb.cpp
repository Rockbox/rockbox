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
#include "hwstub_usb.hpp"
#include <cstring> /* for memcpy */

const uint8_t VR_GET_CPU_INFO = 0;
const uint8_t VR_SET_DATA_ADDRESS = 1;
const uint8_t VR_SET_DATA_LENGTH = 2;
const uint8_t VR_FLUSH_CACHES = 3;
const uint8_t VR_PROGRAM_START1 = 4;
const uint8_t VR_PROGRAM_START2 = 5;

/**
 * Context
 */

hwstub_usb_context::hwstub_usb_context(libusb_context *ctx, bool cleanup_ctx)
    :m_usb_ctx(ctx), m_cleanup_ctx(cleanup_ctx)
{
}

hwstub_usb_context::~hwstub_usb_context()
{
    if(m_cleanup_ctx)
        libusb_exit(m_usb_ctx);
}

std::shared_ptr<hwstub_usb_context> hwstub_usb_context::create(libusb_context *ctx, bool cleanup_ctx)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<hwstub_usb_context>(new hwstub_usb_context(ctx, cleanup_ctx));
}

libusb_context *hwstub_usb_context::native_context()
{
    return m_usb_ctx;
}

libusb_device *hwstub_usb_context::from_ctx_dev(hwstub_ctx_dev_t dev)
{
    return reinterpret_cast<libusb_device*>(dev);
}

hwstub_context::hwstub_ctx_dev_t hwstub_usb_context::to_ctx_dev(libusb_device *dev)
{
    return static_cast<hwstub_ctx_dev_t>(dev);
}

hwstub_error hwstub_usb_context::fetch_device_list(std::vector<hwstub_ctx_dev_t>& list, void*& ptr)
{
    libusb_device **usb_list;
    ssize_t ret = libusb_get_device_list(m_usb_ctx, &usb_list);
    if(ret < 0)
        return HWSTUB_ERROR;
    ptr = (void *)usb_list;
    list.clear();
    for(int i = 0; i < ret; i++)
        if(hwstub_usb_device::is_hwstub_dev(usb_list[i]))
            list.push_back(to_ctx_dev(usb_list[i]));
    return HWSTUB_SUCCESS;
}

void hwstub_usb_context::destroy_device_list(void *ptr)
{
    /* remove all references */
    libusb_free_device_list((libusb_device **)ptr, 1);
}

hwstub_error hwstub_usb_context::create_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device>& hwdev)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    hwdev.reset(new hwstub_usb_device(shared_from_this(), from_ctx_dev(dev)));
    return HWSTUB_SUCCESS;
}

bool hwstub_usb_context::match_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device> hwdev)
{
    hwstub_usb_device *udev = dynamic_cast<hwstub_usb_device*>(hwdev.get());
    return udev != nullptr && udev->native_device() == dev;
}

/**
 * Device
 */
hwstub_usb_device::hwstub_usb_device(std::shared_ptr<hwstub_context> ctx, libusb_device *dev)
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

bool hwstub_usb_device::is_hwstub_dev(libusb_device *dev)
{
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = nullptr;
    if(libusb_get_device_descriptor(dev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(dev, 0, &config) != 0)
        goto Lend;
    /* Try to find Rockbox hwstub interface or a JZ device */
    if(hwstub_usb_rb_handle::find_intf(&dev_desc, config) >= 0 ||
            hwstub_usb_jz_handle::is_boot_dev(&dev_desc, config))
    {
        libusb_free_config_descriptor(config);
        return true;
    }
Lend:
    if(config)
        libusb_free_config_descriptor(config);
    return false;
}

hwstub_error hwstub_usb_device::open_dev(std::shared_ptr<hwstub_handle>& handle)
{
    int intf = -1;
    /* open the device */
    libusb_device_handle *h;
    int err = libusb_open(m_dev, &h);
    if(err != LIBUSB_SUCCESS)
        return HWSTUB_ERROR;
    /* fetch some descriptors */
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *config = nullptr;
    if(libusb_get_device_descriptor(m_dev, &dev_desc) != 0)
        goto Lend;
    if(libusb_get_config_descriptor(m_dev, 0, &config) != 0)
        goto Lend;
    /* Try to find Rockbox hwstub interface */
    if((intf = hwstub_usb_rb_handle::find_intf(&dev_desc, config)) >= 0)
    {
        libusb_free_config_descriptor(config);
        /* create the handle */
        // NOTE: can't use make_shared() because of the protected ctor */
        handle.reset(new hwstub_usb_rb_handle(shared_from_this(), h, intf));
    }
    /* Maybe this is a JZ device ? */
    else if(hwstub_usb_jz_handle::is_boot_dev(&dev_desc, config))
    {
        libusb_free_config_descriptor(config);
        /* create the handle */
        // NOTE: can't use make_shared() because of the protected ctor */
        handle.reset(new hwstub_usb_jz_handle(shared_from_this(), h));
    }
    else
    {
        libusb_free_config_descriptor(config);
        return HWSTUB_ERROR;
    }
    /* the class will perform some probing on creation: check that it actually worked */
    if(handle->valid())
        return HWSTUB_SUCCESS;
    /* abort */
    handle.reset(); // will close the libusb handle
    return HWSTUB_ERROR;

Lend:
    if(config)
        libusb_free_config_descriptor(config);
    libusb_close(h);
    return HWSTUB_ERROR;
}

bool hwstub_usb_device::has_multiple_open() const
{
    /* libusb only allows one handle per device */
    return false;
}

/**
 * USB handle
 */
hwstub_usb_handle::hwstub_usb_handle(std::shared_ptr<hwstub_device> dev, libusb_device_handle *handle)
    :hwstub_handle(dev), m_handle(handle)
{
}

hwstub_usb_handle::~hwstub_usb_handle()
{
    libusb_close(m_handle);
}

ssize_t hwstub_usb_handle::interpret_libusb_error(int err)
{
    if(err >= 0)
        return err;
    if(err == LIBUSB_ERROR_NO_DEVICE)
        return HWSTUB_DISCONNECTED;
    else
        return HWSTUB_USB_ERROR;
}

hwstub_error hwstub_usb_handle::interpret_libusb_error(int err, size_t expected_val)
{
    ssize_t ret = interpret_libusb_error(err);
    if(ret < 0)
        return (hwstub_error)ret;
    if((size_t)ret != expected_val)
        return HWSTUB_ERROR;
    return HWSTUB_SUCCESS;
}

/**
 * Rockbox Handle
 */

hwstub_usb_rb_handle::hwstub_usb_rb_handle(std::shared_ptr<hwstub_device> dev,
        libusb_device_handle *handle, int intf)
    :hwstub_usb_handle(dev, handle), m_intf(intf), m_transac_id(0), m_buf_size(1)
{
    m_probe_status = HWSTUB_SUCCESS;
    /* claim interface */
    if(libusb_claim_interface(m_handle, m_intf) != 0)
        m_probe_status = HWSTUB_PROBE_FAILURE;
    /* check version */
    if(m_probe_status == HWSTUB_SUCCESS)
    {
        struct hwstub_version_desc_t ver_desc;
        m_probe_status = get_version_desc(ver_desc);
        if(m_probe_status == HWSTUB_SUCCESS)
        {
            if(ver_desc.bMajor != HWSTUB_VERSION_MAJOR ||
                    ver_desc.bMinor < HWSTUB_VERSION_MINOR)
                m_probe_status = HWSTUB_PROBE_FAILURE;
        }
    }
    /* get buffer size */
    if(m_probe_status == HWSTUB_SUCCESS)
    {
        struct hwstub_layout_desc_t layout_desc;
        m_probe_status = get_layout_desc(layout_desc);
        if(m_probe_status == HWSTUB_SUCCESS)
            m_buf_size = layout_desc.dBufferSize;
    }
}

hwstub_usb_rb_handle::~hwstub_usb_rb_handle()
{
}

size_t hwstub_usb_rb_handle::get_buffer_size()
{
    return m_buf_size;
}

hwstub_error hwstub_usb_rb_handle::status() const
{
    hwstub_error err = hwstub_usb_handle::status();
    if(err == HWSTUB_SUCCESS)
        err = m_probe_status;
    return err;
}

ssize_t hwstub_usb_rb_handle::get_dev_desc(uint16_t desc, void *buf, size_t buf_sz)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        LIBUSB_REQUEST_GET_DESCRIPTOR, desc << 8, m_intf, (unsigned char *)buf, buf_sz, 1000));
}

ssize_t hwstub_usb_rb_handle::get_dev_log(void *buf, size_t buf_sz)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        HWSTUB_GET_LOG, 0, m_intf, (unsigned char *)buf, buf_sz, 1000));
}

hwstub_error hwstub_usb_rb_handle::exec_dev(uint32_t addr, uint16_t flags)
{
    struct hwstub_exec_req_t exec;
    exec.dAddress = addr;
    exec.bmFlags = flags;
    ssize_t ret = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_EXEC, 0, m_intf, (unsigned char *)&exec, sizeof(exec), 1000), sizeof(exec));
    return (hwstub_error)ret;
}

ssize_t hwstub_usb_rb_handle::read_dev(uint8_t breq, uint32_t addr,
    void *buf, size_t sz, bool atomic)
{
    struct hwstub_read_req_t read;
    read.dAddress = addr;
    ssize_t ret = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        HWSTUB_READ, m_transac_id, m_intf, (unsigned char *)&read, sizeof(read), 1000));
    if(ret != (ssize_t)sizeof(read))
        ret = HWSTUB_ERROR; 
    if(ret < 0)
        return ret;
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN,
        breq, m_transac_id++, m_intf, (unsigned char *)buf, sz, 1000));
}

ssize_t hwstub_usb_rb_handle::write_dev(uint8_t breq, uint32_t addr,
    const void *buf, size_t sz, bool atomic)
{
    size_t hdr_sz = sizeof(struct hwstub_write_req_t);
    uint8_t *tmp_buf = new uint8_t[sz + hdr_sz];
    struct hwstub_write_req_t *req = reinterpret_cast<struct hwstub_write_req_t *>(tmp_buf);
    req->dAddress = addr;
    memcpy(tmp_buf + hdr_sz, buf, sz);
    ssize_t ret = interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        breq, m_transac_id++, m_intf, (unsigned char *)req, sz + hdr_sz, 1000), sz + hdr_sz);
    delete tmp_buf;
    return ret;
}

int hwstub_usb_rb_handle::find_intf(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config)
{
    (void) dev;
    /* search hwstub interface */
    for(unsigned i = 0; i < config->bNumInterfaces; i++)
    {
        /* hwstub interface has only one setting */
        if(config->interface[i].num_altsetting != 1)
            continue;
        const struct libusb_interface_descriptor *intf = &config->interface[i].altsetting[0];
        /* check class/subclass/protocol */
        if(intf->bInterfaceClass == HWSTUB_CLASS &&
                intf->bInterfaceSubClass == HWSTUB_SUBCLASS &&
                intf->bInterfaceProtocol == HWSTUB_PROTOCOL)
            /* found it ! */
            return i;
    }
    return HWSTUB_ERROR;
}

/**
 * JZ Handle
 */

namespace
{
    uint16_t jz_bcd(char *bcd)
    {
        uint16_t v = 0;
        for(int i = 0; i < 4; i++)
            v = (bcd[i] - '0') | v << 4;
        return v;
    }
}

hwstub_usb_jz_handle::hwstub_usb_jz_handle(std::shared_ptr<hwstub_device> dev,
        libusb_device_handle *handle)
    :hwstub_usb_handle(dev, handle)
{
    m_probe_status = probe();
}

hwstub_usb_jz_handle::~hwstub_usb_jz_handle()
{
}

hwstub_error hwstub_usb_jz_handle::probe()
{
    char cpuinfo[8];
    /* Get CPU info and devise descriptor */
    hwstub_error err = jz_cpuinfo(cpuinfo);
    if(err != HWSTUB_SUCCESS)
        return err;
    struct libusb_device_descriptor dev_desc;
    err = interpret_libusb_error(libusb_get_device_descriptor(
        libusb_get_device(m_handle), &dev_desc), 0);
    if(err != HWSTUB_SUCCESS)
        return err;
    /** parse CPU info */
    /* if cpuinfo if of the form JZxxxxVy then extract xxxx */
    if(cpuinfo[0] == 'J' && cpuinfo[1] == 'Z' && cpuinfo[6] == 'V')
        m_desc_jz.wChipID = jz_bcd(cpuinfo + 2);
    /* if cpuinfo if of the form Bootxxxx then extract xxxx */
    else if(strncmp(cpuinfo, "Boot", 4) == 4)
        m_desc_jz.wChipID = jz_bcd(cpuinfo + 4);
    /* else use usb id */
    else
        m_desc_jz.wChipID = dev_desc.idProduct;

    /** Retrieve product string */
    ssize_t len = interpret_libusb_error(libusb_get_string_descriptor_ascii(m_handle,
        dev_desc.iProduct, (unsigned char *)m_desc_target.bName, sizeof(m_desc_target.bName)));
    if(len < 0)
        return (hwstub_error)err;

    /** Fill descriptors */

    m_desc_version.bLength = sizeof(m_desc_version);
    m_desc_version.bDescriptorType = HWSTUB_DT_VERSION;
    m_desc_version.bMajor = HWSTUB_VERSION_MAJOR;
    m_desc_version.bMinor = HWSTUB_VERSION_MINOR;
    m_desc_version.bRevision = 0;

    m_desc_layout.bLength = sizeof(m_desc_layout);
    m_desc_layout.bDescriptorType = HWSTUB_DT_LAYOUT;
    m_desc_layout.dCodeStart = 0xbfc00000; /* ROM */
    m_desc_layout.dCodeSize = 0x2000; /* 8kB per datasheet */
    m_desc_layout.dStackStart = 0; /* As far as I can tell, the ROM uses no stack */
    m_desc_layout.dStackSize = 0;
    m_desc_layout.dBufferStart =  0x080000000;
    m_desc_layout.dBufferSize = 0x4000;

    m_desc_target.bLength = sizeof(m_desc_target);
    m_desc_target.bDescriptorType = HWSTUB_DT_TARGET;
    m_desc_target.dID = HWSTUB_TARGET_JZ;

    m_desc_jz.bLength = sizeof(m_desc_jz);
    m_desc_jz.bDescriptorType = HWSTUB_DT_JZ;

    return HWSTUB_SUCCESS;
}

size_t hwstub_usb_jz_handle::get_buffer_size()
{
    return m_desc_layout.dBufferSize;
}

hwstub_error hwstub_usb_jz_handle::status() const
{
    hwstub_error err = hwstub_usb_handle::status();
    if(err == HWSTUB_SUCCESS)
        err = m_probe_status;
    return err;
}
ssize_t hwstub_usb_jz_handle::get_dev_desc(uint16_t desc, void *buf, size_t buf_sz)
{
    void *p = nullptr;
    switch(desc)
    {
        case HWSTUB_DT_VERSION: p = &m_desc_version; break;
        case HWSTUB_DT_LAYOUT: p = &m_desc_layout; break;
        case HWSTUB_DT_TARGET: p = &m_desc_target; break;
        case HWSTUB_DT_JZ: p = &m_desc_jz; break;
        default: break;
    }
    if(p == nullptr)
        return HWSTUB_ERROR;
    /* size is in the bLength field of the descriptor */
    size_t desc_sz = *(uint8_t *)p;
    buf_sz = std::min(buf_sz, desc_sz);
    memcpy(buf, p, buf_sz);
    return buf_sz;
}

ssize_t hwstub_usb_jz_handle::get_dev_log(void *buf, size_t buf_sz)
{
    return HWSTUB_SUCCESS;
}

hwstub_error hwstub_usb_jz_handle::exec_dev(uint32_t addr, uint16_t flags)
{
    return HWSTUB_ERROR;
}

ssize_t hwstub_usb_jz_handle::read_dev(uint8_t breq, uint32_t addr,
    void *buf, size_t sz, bool atomic)
{
    return HWSTUB_ERROR;
}

ssize_t hwstub_usb_jz_handle::write_dev(uint8_t breq, uint32_t addr,
    const void *buf, size_t sz, bool atomic)
{
    return HWSTUB_ERROR;
}

bool hwstub_usb_jz_handle::is_boot_dev(struct libusb_device_descriptor *dev,
    struct libusb_config_descriptor *config)
{
    (void) config;
    /* don't bother checking the config descriptor and use the device ID only */
    return dev->idVendor ==  0x601a && dev->idProduct >= 0x4740 && dev->idProduct <= 0x4780;
}

hwstub_error hwstub_usb_jz_handle::jz_cpuinfo(char cpuinfo[8])
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_GET_CPU_INFO, 0, 0, (unsigned char *)cpuinfo, 8, 1000), 8);
}

hwstub_error hwstub_usb_jz_handle::jz_set_addr(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_ADDRESS, addr >> 16, addr & 0xffff, NULL, 0, 1000), 0);
}

hwstub_error hwstub_usb_jz_handle::jz_set_length(uint32_t size)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_SET_DATA_LENGTH, size >> 16, size & 0xffff, NULL, 0, 1000), 0);
}

ssize_t hwstub_usb_jz_handle::jz_upload(void *data, size_t length)
{
    int xfered;
    ssize_t ret = interpret_libusb_error(libusb_bulk_transfer(m_handle,
        LIBUSB_ENDPOINT_IN | 1, (unsigned char *)data, length, &xfered, 1000));
    return ret == HWSTUB_SUCCESS ? xfered : ret;
}

ssize_t hwstub_usb_jz_handle::jz_download(void *data, size_t length)
{
    int xfered;
    ssize_t ret = interpret_libusb_error(libusb_bulk_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | 1, (unsigned char *)data, length, &xfered, 1000));
    return ret == HWSTUB_SUCCESS ? xfered : ret;
}

hwstub_error hwstub_usb_jz_handle::jz_start1(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START1, addr >> 16, addr & 0xffff, NULL, 0, 1000), 0);
}

hwstub_error hwstub_usb_jz_handle::jz_flush_caches()
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_FLUSH_CACHES, 0, 0, NULL, 0, 1000), 0);
}

hwstub_error hwstub_usb_jz_handle::jz_start2(uint32_t addr)
{
    return interpret_libusb_error(libusb_control_transfer(m_handle,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        VR_PROGRAM_START2, addr >> 16, addr & 0xffff, NULL, 0, 1000), 0);
}
