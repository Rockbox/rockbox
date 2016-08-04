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

namespace hwstub {
namespace usb {

/** USB context
 *
 * Context based on libusb. */
class context : public hwstub::context
{
protected:
    context(libusb_context *ctx, bool cleanup_ctx);
public:
    virtual ~context();
    /** Return native libusb context */
    libusb_context *native_context();
    /** Create a USB context. If cleanup_ctx is true, libusb_exit() will be
     * called on the context on deletion of this class. If ctx is NULL, libusb_init()
     * will be called with NULL so there is no need to init the default context. */
    static std::shared_ptr<context> create(libusb_context *ctx, bool cleanup_ctx = false,
        std::string *error = nullptr);

protected:
    /* NOTE ctx_dev_t = libusb_device* */
    libusb_device *from_ctx_dev(ctx_dev_t dev);
    ctx_dev_t to_ctx_dev(libusb_device *dev);
    virtual error fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual error create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev);
    virtual bool match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev);

    libusb_context *m_usb_ctx; /* libusb context (might be NULL) */
    bool m_cleanup_ctx; /* cleanup context on delete ? */
};

/** USB device
 *
 * Device based on libusb_device. */
class device : public hwstub::device
{
    friend class context; /* for ctor */
protected:
    device(std::shared_ptr<hwstub::context> ctx, libusb_device *dev);
public:
    virtual ~device();
    /** Return native libusb device */
    libusb_device *native_device();
    /** Get bus number */
    uint8_t get_bus_number();
    /** Get device address */
    uint8_t get_address();
    /** Get device VID */
    uint16_t get_vid();
    /** Get device PID */
    uint16_t get_pid();

protected:
    /** Return true if this might be a hwstub device and should appear in the list */
    static bool is_hwstub_dev(libusb_device *dev);

    virtual error open_dev(std::shared_ptr<hwstub::handle>& handle);
    virtual bool has_multiple_open() const;

    libusb_device *m_dev; /* USB device */
};

/** USB handle
 *
 * Handle based on libusb_device_handle. */
class handle : public hwstub::handle
{
protected:
    handle(std::shared_ptr<hwstub::device> dev, libusb_device_handle *handle);
public:
    virtual ~handle();
    /** set operation timeout */
    void set_timeout(std::chrono::milliseconds ms);

protected:
    /* interpret libusb error: >=0 means SUCCESS, others are treated as errors,
     * LIBUSB_ERROR_NO_DEVICE is treated as DISCONNECTED */
    error interpret_libusb_error(int err);
    /* interpret libusb error: <0 returns interpret_libusb_error(err), otherwise
     * returns SUCCESS if err == expected_value */
    error interpret_libusb_error(int err, size_t expected_value);
    /* interpret libusb error: <0 returns interpret_libusb_error(err), otherwise
     * returns SUCCESS and write size in out_size */
    error interpret_libusb_size(int err, size_t& out_size);

    libusb_device_handle *m_handle; /* USB handle */
    unsigned int m_timeout; /* in milliseconds */
};

/** Rockbox USB handle
 *
 * HWSTUB/Rockbox protocol. */
class rb_handle : public handle
{
    friend class device; /* for find_intf() */
protected:
    rb_handle(std::shared_ptr<hwstub::device> dev, libusb_device_handle *handle, int intf);
public:
    virtual ~rb_handle();

protected:
    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic);
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz);
    virtual error get_dev_log(void *buf, size_t& buf_sz);
    virtual error exec_dev(uint32_t addr, uint16_t flags);
    virtual error cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
        size_t out_size, void *in_data, size_t *in_size);
    virtual error status() const;
    virtual size_t get_buffer_size();
    /* Probe a device to check if it is an hwstub device and return the interface
     * number, or <0 on error. */
    static bool find_intf(struct libusb_device_descriptor *dev,
        struct libusb_config_descriptor *config, int& intf);

    error m_probe_status; /* probing status */
    int m_intf; /* interface number */
    uint16_t m_transac_id; /* transaction ID */
    size_t m_buf_size; /* Device buffer size */
};

/** JZ USB handle
 *
 * JZ boot protocol */
class jz_handle : public handle
{
    friend class device; /* for is_boot_dev() */
protected:
    jz_handle(std::shared_ptr<hwstub::device> dev, libusb_device_handle *handle);
public:
    virtual ~jz_handle();

protected:
    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic);
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz);
    virtual error get_dev_log(void *buf, size_t& buf_sz);
    virtual error exec_dev(uint32_t addr, uint16_t flags);
    virtual error cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
        size_t out_size, void *in_data, size_t *in_size);
    virtual error status() const;
    virtual size_t get_buffer_size();
    error probe();
    error probe_jz4760b();
    error read_reg32(uint32_t addr, uint32_t& value);
    error write_reg32(uint32_t addr, uint32_t value);

    error jz_cpuinfo(char cpuinfo[8]);
    error jz_set_addr(uint32_t addr);
    error jz_set_length(uint32_t size);
    error jz_upload(void *data, size_t& length);
    error jz_download(const void *data, size_t& length);
    error jz_start1(uint32_t addr);
    error jz_flush_caches();
    error jz_start2(uint32_t addr);
    /* Probe a device to check if it is a jz boot device */
    static bool is_boot_dev(struct libusb_device_descriptor *dev,
        struct libusb_config_descriptor *config);

    error m_probe_status; /* probing status */
    struct hwstub_version_desc_t m_desc_version;
    struct hwstub_layout_desc_t m_desc_layout;
    struct hwstub_target_desc_t m_desc_target;
    struct hwstub_jz_desc_t m_desc_jz;
};

} // namespace usb
} // namespace hwstub

#endif /* __HWSTUB_USB_HPP__ */
