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
#ifndef __HWSTUB_HPP__
#define __HWSTUB_HPP__

#include "hwstub_protocol.h"
#include <string>
#include <mutex>
#include <vector>
#include <libusb.h>
#include <cstdint>
#include <atomic>

class hwstub_context;
class hwstub_device;
class hwstub_handle;

enum hwstub_error
{
    HWSTUB_SUCCESS = 0,
    HWSTUB_ERROR = -1,
};

/** NOTE Multithreading:
 * Unless specified, all methods are thread-safe
 */

/** Context
 *
 * A context provides a list of available devices and may notify devices
 * arrival and departure. */
class hwstub_context
{
public:
    hwstub_context();
    /** On destruction, the context will destroy all the devices. */
    virtual ~hwstub_context();
    /** Get device list, clears the list in argument first. All devices in the list
     * are still connected (or believe to be). This function will update the device
     * list. This methods adds a new reference to each device, it is up to the caller
     * to unreference those.
     * If notify is true, notifcation callback will be called. */
    hwstub_error get_device_list(std::vector< hwstub_device* >& list, bool notify = true);
    /** Lock context. When locked, the list of devices cannot change and notification
     * callbacks are not called. */
    void lock();
    /** Unlock context */
    void unlock();
    /** Notify the context about a device. If arrived is true, the device is
     * added to the list and a reference will be added to it. If arrived is false,
     * the device is marked as disconnected(), removed from the list and a
     * reference will be removed from it.
     * If notify is true, notifcation callback will be called. */
    void change_device(bool arrived, hwstub_device *dev, bool notify = true);
    /** Returns true if the context supports hotplug notifications.
     * If the context supports hotplug, it can notify about device list changes
     * and update its internal list asynchronously. */
    virtual bool has_hotplug() = 0;
    /** Force the context to update its internal list of devices.
     * If notify is true, notifcation callback will be called. */
    hwstub_error update_list(bool notify = true);

protected:
    /** Do device notification */
    void notify_device(bool arrived, hwstub_device *dev);
    /** Opaque device type */
    typedef void* hwstub_ctx_dev_t;
    /** Fetch the device list. Each item in the list is an opaque pointer. The function
     * can also provide a pointer that will be used to free the list resources
     * if necessary. Return <0 on error. */
    virtual hwstub_error fetch_device_list(std::vector< hwstub_ctx_dev_t >& list, void*& ptr) = 0;
    /** Destroy the resources created to get the list. */
    virtual void destroy_device_list(void *ptr) = 0;
    /** Create a new hwstub device from the opaque pointer. Return <0 on error.
     * This function needs not add a reference to the newly created device. */
    virtual hwstub_error create_device(hwstub_ctx_dev_t dev, hwstub_device*& hwdev) = 0;
    /** Return true if the opaque pointer corresponds to the device. Only called
     * from map_device(). */
    virtual bool match_device(hwstub_ctx_dev_t dev, hwstub_device *hwdev) = 0;

    std::recursive_mutex m_mutex; /* list mutex */
    std::vector< hwstub_device* > m_devlist; /* list of devices */
};

/** USB context
 * 
 * Context based on libusb. */
class hwstub_usb_context : public hwstub_context
{
public:
    hwstub_usb_context(libusb_context *ctx);
    ~hwstub_usb_context();
    virtual bool has_hotplug();
    /** Return native libusb context */
    libusb_context *native_context();

protected:
    /* hwstub_ctx_dev_t = libusb_device* */
    virtual hwstub_error fetch_device_list(std::vector< hwstub_ctx_dev_t >& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual hwstub_error create_device(hwstub_ctx_dev_t dev, hwstub_device*& hwdev);
    virtual bool match_device(hwstub_ctx_dev_t dev, hwstub_device *hwdev);

    libusb_context *m_usb_ctx; /* libusb context (might be NULL) */
};

/** Device
 *
 * A device belong to a context. The device is referenc counted and is destroyed
 * when its reference count decreased to zero. */
class hwstub_device
{
public:
    hwstub_device(hwstub_context *ctx);
    virtual ~hwstub_device();
    /** Open a handle to the device. Several handles may be opened concurrently. */
    hwstub_error open(hwstub_handle *& handle);
    /** A reference to the device. */
    void ref();
    /** Remove a reference to the device. If the count decrements to zero, device
     * is destroyed. */
    void unref();
    /** Lock device. When locked, operations from other threads will block, this
     * includes modifying connection status and reference count. */
    void lock();
    /** Unlock device. */
    void unlock();
    /** Disconnect the device. This will notify the context that the device is gone. */
    void disconnect();
    /** Returns true if the device is still connected. */
    bool connected();

protected:
    /** Open the device and return a handle. If the device does not support multiple
     * handles (as reported by has_multiple_open()), proxy handles will be
     * created to serialise operations. */
    virtual hwstub_error do_open(hwstub_handle*& handle) = 0;
    /** Return true if device can be opened multiple times. In this case, each
     * call to open() will generate a call to do_open(). Otherwise, proxy handles
     * will be created and do_open() will only be called when the device is not
     * yet opended. */
    virtual bool has_multiple_open() = 0;

    hwstub_context *m_ctx; /* pointer to context */
    std::atomic< int > m_refcnt; /* reference count */
    std::recursive_mutex m_mutex; /* device state mutex: ref count, connection status */
    bool m_connected; /* false once device is disconnected */
};

/** USB device */
class hwstub_usb_device : public hwstub_device
{
public:
    hwstub_usb_device(hwstub_context *ctx, libusb_device *dev);
    ~hwstub_usb_device();
    /** Return native libusb device */
    libusb_device *native_device();

protected:
    virtual hwstub_error do_open(hwstub_handle*& handle);
    virtual bool has_multiple_open();

    libusb_device *m_dev; /* USB device */
};

class hwstub_handle
{
public:
    hwstub_handle(hwstub_device *dev);
    virtual ~hwstub_handle();
    virtual ssize_t get_desc(uint16_t desc, void *buf, size_t buf_sz) = 0;
    virtual ssize_t get_log(void *buf, size_t buf_sz) = 0;
    virtual int exec(uint32_t addr, uint16_t flags) = 0;
    /* read and write function will handle the maximum buffer size constraint
     * transparently */
    ssize_t read(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    ssize_t write(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);

protected:
    /* buffer size is guaranteed to be <= m_dev->m_buf_sz */
    virtual ssize_t read_buf(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic) = 0;
    virtual ssize_t write_buf(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic) = 0;

    hwstub_device *m_dev; /* pointer to device */
    std::recursive_mutex *m_mutex; /* operation mutex to serialise operations */
};

class hwstub_usb_handle : public hwstub_handle
{
public:
    hwstub_usb_handle(hwstub_device *dev, libusb_device_handle *handle);
    ~hwstub_usb_handle();

protected:

    libusb_device_handle *m_handle; /* USB handle */
};

class hwstub_usb_rb_handle : public hwstub_usb_handle
{
public:
    hwstub_usb_rb_handle(hwstub_device *dev, libusb_device_handle *handle, int intf);
    ~hwstub_usb_rb_handle();
    virtual ssize_t get_desc(uint16_t desc, void *buf, size_t buf_sz);
    virtual ssize_t get_log(void *buf, size_t buf_sz);
    virtual int exec(uint32_t addr, uint16_t flags);

protected:
    virtual ssize_t read_buf(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    virtual ssize_t write_buf(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);

    int m_intf; /* interface number */
    uint16_t m_transac_id; /* transaction ID */
};

class hwstub_usb_jz_handle : public hwstub_usb_handle
{
public:
    hwstub_usb_jz_handle(hwstub_device *dev, libusb_device_handle *handle);
    ~hwstub_usb_jz_handle();
    virtual ssize_t get_desc(uint16_t desc, void *buf, size_t buf_sz);
    virtual ssize_t get_log(void *buf, size_t buf_sz);
    virtual int exec(uint32_t addr, uint16_t flags);

protected:
    virtual ssize_t read_buf(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    virtual ssize_t write_buf(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);
};

#endif /* __HWSTUB_HPP__ */
 