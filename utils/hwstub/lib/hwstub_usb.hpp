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
#ifndef __HWSTUB_USB_HPP__
#define __HWSTUB_USB_HPP__

#include "hwstub_usb.hpp"
#include <libusb.h>


/** USB context
 *
 * Context based on libusb. */
class hwstub_usb_context : public hwstub_context
{
protected:
    hwstub_usb_context(libusb_context *ctx, bool cleanup_ctx);
public:
    virtual ~hwstub_usb_context();
    /** Return native libusb context */
    libusb_context *native_context();
    /** Create a USB context. If cleanup_ctx is true, libusb_exit() will be
     * called on the context on deletion of this class. */
    static std::shared_ptr<hwstub_usb_context> create(libusb_context *ctx, bool cleanup_ctx = false);

protected:
    /* NOTE hwstub_ctx_dev_t = libusb_device* */
    libusb_device *from_ctx_dev(hwstub_ctx_dev_t dev);
    hwstub_ctx_dev_t to_ctx_dev(libusb_device *dev);
    virtual hwstub_error fetch_device_list(std::vector<hwstub_ctx_dev_t>& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual hwstub_error create_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device>& hwdev);
    virtual bool match_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device> hwdev);

    libusb_context *m_usb_ctx; /* libusb context (might be NULL) */
    bool m_cleanup_ctx; /* cleanup context on delete ? */
};

/** USB device
 *
 * Device based on libusb_device. */
class hwstub_usb_device : public hwstub_device
{
    friend class hwstub_usb_context; /* for ctor */
protected:
    hwstub_usb_device(std::shared_ptr<hwstub_context> ctx, libusb_device *dev);
public:
    virtual ~hwstub_usb_device();
    /** Return native libusb device */
    libusb_device *native_device();

protected:
    /** Return true if this might be a hwstub device and should appear in the list */
    static bool is_hwstub_dev(libusb_device *dev);

    virtual hwstub_error open_dev(std::shared_ptr<hwstub_handle>& handle);
    virtual bool has_multiple_open() const;

    libusb_device *m_dev; /* USB device */
};

/** USB handle
 *
 * Handle based on libusb_device_handle. */
class hwstub_usb_handle : public hwstub_handle
{
protected:
    hwstub_usb_handle(std::shared_ptr<hwstub_device> dev, libusb_device_handle *handle);
public:
    virtual ~hwstub_usb_handle();

protected:
    ssize_t interpret_libusb_error(int err);
    hwstub_error interpret_libusb_error(int err, size_t expected_value);

    libusb_device_handle *m_handle; /* USB handle */
};

/** Rockbox USB handle
 *
 * HWSTUB/Rockbox protocol. */
class hwstub_usb_rb_handle : public hwstub_usb_handle
{
    friend class hwstub_usb_device; /* for find_intf() */
protected:
    hwstub_usb_rb_handle(std::shared_ptr<hwstub_device> dev, libusb_device_handle *handle, int intf);
public:
    virtual ~hwstub_usb_rb_handle();

protected:
    virtual ssize_t read_dev(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    virtual ssize_t write_dev(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);
    virtual ssize_t get_dev_desc(uint16_t desc, void *buf, size_t buf_sz);
    virtual ssize_t get_dev_log(void *buf, size_t buf_sz);
    virtual hwstub_error exec_dev(uint32_t addr, uint16_t flags);
    virtual hwstub_error status() const;
    virtual size_t get_buffer_size();
    /* Probe a device to check if it is an hwstub device and return the interface
     * number, or <0 on error. */
    static int find_intf(struct libusb_device_descriptor *dev,
        struct libusb_config_descriptor *config);

    hwstub_error m_probe_status; /* probing status */
    int m_intf; /* interface number */
    uint16_t m_transac_id; /* transaction ID */
    size_t m_buf_size; /* Device buffer size */
};

/** JZ USB handle
 *
 * JZ boot protocol */
class hwstub_usb_jz_handle : public hwstub_usb_handle
{
    friend class hwstub_usb_device; /* for is_boot_dev() */
protected:
    hwstub_usb_jz_handle(std::shared_ptr<hwstub_device> dev, libusb_device_handle *handle);
public:
    virtual ~hwstub_usb_jz_handle();

protected:
    virtual ssize_t read_dev(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    virtual ssize_t write_dev(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);
    virtual ssize_t get_dev_desc(uint16_t desc, void *buf, size_t buf_sz);
    virtual ssize_t get_dev_log(void *buf, size_t buf_sz);
    virtual hwstub_error exec_dev(uint32_t addr, uint16_t flags);
    virtual hwstub_error status() const;
    virtual size_t get_buffer_size();
    hwstub_error probe();

    hwstub_error jz_cpuinfo(char cpuinfo[8]);
    hwstub_error jz_set_addr(uint32_t addr);
    hwstub_error jz_set_length(uint32_t size);
    ssize_t jz_upload(void *data, size_t length);
    ssize_t jz_download(void *data, size_t length);
    hwstub_error jz_start1(uint32_t addr);
    hwstub_error jz_flush_caches();
    hwstub_error jz_start2(uint32_t addr);
    /* Probe a device to check if it is a jz boot device */
    static bool is_boot_dev(struct libusb_device_descriptor *dev,
        struct libusb_config_descriptor *config);

    hwstub_error m_probe_status; /* probing status */
    struct hwstub_version_desc_t m_desc_version;
    struct hwstub_layout_desc_t m_desc_layout;
    struct hwstub_target_desc_t m_desc_target;
    struct hwstub_jz_desc_t m_desc_jz;
};

#endif /* __HWSTUB_USB_HPP__ */
