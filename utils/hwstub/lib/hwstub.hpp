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
#include <cstdint>
#include <atomic>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

class hwstub_context;
class hwstub_device;
class hwstub_handle;

enum hwstub_error
{
    HWSTUB_SUCCESS = 0, /** Success */
    HWSTUB_ERROR = -1, /** Unspecified error */
    HWSTUB_DISCONNECTED = -2, /** Device has been disconnected */
    HWSTUB_PROBE_FAILURE = -3, /** Device did not pass probing */
    HWSTUB_NO_CONTEXT = -4, /** The context has been destroyed */
    HWSTUB_USB_ERROR = -5, /** Unspecified USB error */
};

/** NOTE Multithreading:
 * Unless specified, all methods are thread-safe
 */

class hwstub_context_poller;

/** Context
 *
 * A context provides a list of available devices and may notify devices
 * arrival and departure.
 *
 * A context provides a way to regularly poll for derive changes. There are two
 * ways to manually force an update:
 * - on call to get_device_list(), the list is already refetched
 * - on call to update_list() to force list update
 * Note that automatic polling is disabled by default.
 */
class hwstub_context : public std::enable_shared_from_this<hwstub_context>
{
protected:
    hwstub_context();
public:
    /** On destruction, the context will destroy all the devices. */
    virtual ~hwstub_context();
    /** Get device list, clears the list in argument first. All devices in the list
     * are still connected (or believe to be). This function will update the device
     * list. */
    hwstub_error get_device_list(std::vector<std::shared_ptr<hwstub_device>>& list);
    /** Force the context to update its internal list of devices. */
    hwstub_error update_list();
    /** Ask the context to automatically poll for device changes.
     * Note that this might spawn a new thread to do so, in which case it will
     * be destroyed/stop on deletetion or when stop_polling() is called. If
     * polling is already enabled, this function will change the polling interval. */
    void start_polling(std::chrono::milliseconds interval = std::chrono::milliseconds(250));
    /** Stop polling. */
    void stop_polling();
    /** Register a notification callback with arguments (context,arrived,device)
     * WARNING the callback may be called asynchronously ! */
    typedef std::function<void(std::shared_ptr<hwstub_context>, bool, std::shared_ptr<hwstub_device>)> notification_callback_t;
    void register_callback(const notification_callback_t& fn);

protected:
    /** Notify the context about a device. If arrived is true, the device is
     * added to the list and a reference will be added to it. If arrived is false,
     * the device is marked as disconnected(), removed from the list and a
     * reference will be removed from it. Adding a device that matches an
     * existing one will do nothing. */
    void change_device(bool arrived, std::shared_ptr<hwstub_device> dev);
    /** Do device notification */
    void notify_device(bool arrived, std::shared_ptr<hwstub_device> dev);
    /** Opaque device type */
    typedef void* hwstub_ctx_dev_t;
    /** Fetch the device list. Each item in the list is an opaque pointer. The function
     * can also provide a pointer that will be used to free the list resources
     * if necessary. Return <0 on error. */
    virtual hwstub_error fetch_device_list(std::vector<hwstub_ctx_dev_t>& list, void*& ptr) = 0;
    /** Destroy the resources created to get the list. */
    virtual void destroy_device_list(void *ptr) = 0;
    /** Create a new hwstub device from the opaque pointer. Return <0 on error.
     * This function needs not add a reference to the newly created device. */
    virtual hwstub_error create_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device>& hwdev) = 0;
    /** Return true if the opaque pointer corresponds to the device. Only called
     * from map_device(). */
    virtual bool match_device(hwstub_ctx_dev_t dev, std::shared_ptr<hwstub_device> hwdev) = 0;
    /** Check if a device matches another one in the list */
    bool contains_dev(const std::vector<hwstub_device*>& list, hwstub_ctx_dev_t dev);

    std::shared_ptr<hwstub_context_poller> m_poller; /* poller object */
    std::recursive_mutex m_mutex; /* list mutex */
    std::vector<std::shared_ptr<hwstub_device>> m_devlist; /* list of devices */
    std::vector<notification_callback_t> m_callbacks; /* list of callbacks */
};

/** Context Poller
 *
 * This class provides a way to regularly poll a context for device changes.
 * NOTE this class is not meant to be used directly since hwstub_context already
 * provides access to it via start_polling() and stop_polling() */
class hwstub_context_poller
{
public:
    hwstub_context_poller(std::weak_ptr<hwstub_context> ctx, std::chrono::milliseconds interval = std::chrono::milliseconds(250));
    ~hwstub_context_poller();
    /** Set polling interval (in milliseconds) (works even if polling already enabled) */
    void set_interval(std::chrono::milliseconds interval);
    /** Start polling */
    void start();
    /** Stop polling. After return, no function will be made. */
    void stop();

protected:
    static void thread(hwstub_context_poller *poller);
    void poll();

    std::weak_ptr<hwstub_context> m_ctx; /* context */
    bool m_running; /* are we running ? */
    bool m_exit; /* exit flag for the thread */
    std::thread m_thread; /* polling thread */
    std::mutex m_mutex; /* mutex lock */
    std::condition_variable m_cond; /* signalling condition */
    std::chrono::milliseconds m_interval; /* Interval */
};

/** Device
 *
 * A device belongs to a context.
 * Note that a device only keeps a weak pointer to the context, so it is possible
 * for the context to be destroyed during the life of the device, in which case
 * all operations on it will fail. */
class hwstub_device : public std::enable_shared_from_this<hwstub_device>
{
protected:
    hwstub_device(std::shared_ptr<hwstub_context> ctx);
public:
    virtual ~hwstub_device();
    /** Open a handle to the device. Several handles may be opened concurrently. */
    hwstub_error open(std::shared_ptr<hwstub_handle>& handle);
    /** Disconnect the device. This will notify the context that the device is gone. */
    void disconnect();
    /** Returns true if the device is still connected. */
    bool connected();
    /** Get context (might be empty) */
    std::shared_ptr<hwstub_context> get_context();

protected:
    /** Some subsystems allow for hardware to be open several times and so do not.
     * For example, libusb only allows one handle per device. To workaround this issue,
     * open() will do some magic to allow for several open() even when the hardware
     * supports only one. If the device does not support multiple
     * handles (as reported by has_multiple_open()), open() will only call open_dev()
     * the first time the device is opened and will redirect other open() calls to
     * this handle using proxy handles. If the device supports multiple handles,
     * open() will simply call open_dev() each time.
     * The open_dev() does not need to care about this magic and only needs to
     * open the device and returns the handle to it.
     * NOTE this function is always called with the mutex locked already. */
    virtual hwstub_error open_dev(std::shared_ptr<hwstub_handle>& handle) = 0;
    /** Return true if device can be opened multiple times. In this case, each
     * call to open() will generate a call to do_open(). Otherwise, proxy handles
     * will be created for each open() and do_open() will only be called the first
     * time. */
    virtual bool has_multiple_open() const = 0;

    std::weak_ptr<hwstub_context> m_ctx; /* pointer to context */
    std::atomic<int> m_refcnt; /* reference count */
    std::recursive_mutex m_mutex; /* device state mutex: ref count, connection status */
    bool m_connected; /* false once device is disconnected */
    std::weak_ptr<hwstub_handle> m_handle; /* weak pointer to the opened handle (if !has_multiple_open()) */
};

/** Handle
 *
 * A handle is tied to a device and provides access to the stub operation.
 * The handle is reference counted and is destroyed
 * when its reference count decreased to zero.
 */
class hwstub_handle : public std::enable_shared_from_this<hwstub_handle>
{
protected:
    /** A handle will always hold a reference to the device */
    hwstub_handle(std::shared_ptr<hwstub_device> dev);
public:
    /** When destroyed, the handle will release its reference to the device */
    virtual ~hwstub_handle();
    /** Return associated device */
    std::shared_ptr<hwstub_device> get_device();
    /** Fetch a descriptor */
    ssize_t get_desc(uint16_t desc, void *buf, size_t buf_sz);
    /** Fetch part of the log */
    ssize_t get_log(void *buf, size_t buf_sz);
    /** Ask the stub to execute some code.
     * NOTE: this may kill the stub */
    hwstub_error exec(uint32_t addr, uint16_t flags);
    /** Read/write some device memory.
     * NOTE: the stub may or may not recover from bad read/write, so this may kill it.
     * NOTE: the default implemtentation of read() and write() will split transfers
     *       according to the buffer size and call read_dev() and write_dev() */
    ssize_t read(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic);
    ssize_t write(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic);
    /** Get device buffer size: any read() or write() greater than this size
     * will be split into several transaction to avoid overflowing device
     * buffer. */
    virtual size_t get_buffer_size() = 0;
    /** Check a handle status. A successful handle does not guarantee successful
     * operations. An invalid handle will typically report probing failure (the
     * device did not pass probing) or disconnection.
     * The default implemtentation will test if context still exists and connection status. */
    virtual hwstub_error status() const;
    /** Shorthand for status() == HWSTUB_SUCCESS */
    inline bool valid() const { return status() == HWSTUB_SUCCESS; }

    /** Helper functions */
    hwstub_error get_version_desc(hwstub_version_desc_t& desc);
    hwstub_error get_layout_desc(hwstub_layout_desc_t& desc);
    hwstub_error get_stmp_desc(hwstub_stmp_desc_t& desc);
    hwstub_error get_pp_desc(hwstub_pp_desc_t& desc);
    hwstub_error get_jz_desc(hwstub_jz_desc_t& desc);
    hwstub_error get_target_desc(hwstub_target_desc_t& desc);

protected:
    /** The get_desc(), get_log(), exec(), read() and write() function
     * take care of details so that each implementation can safely assume that
     * the hwstub context exists and will not be destroyed during the execution
     * of the function. It will also return early if the device has been disconnected.
     * 
     * NOTE on read() and write():
     * Since devices have a limited buffer, big transfers must be split into
     * smaller ones. The high-level read() and write() functions perform this
     * splitting in a generic way, based on get_buffer_size(), and calling read_dev()
     * and write_dev() which do the actual operation.
     * These function can safely assume that sz <= get_buffer_size(). */
    virtual ssize_t read_dev(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic) = 0;
    virtual ssize_t write_dev(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic) = 0;
    virtual ssize_t get_dev_desc(uint16_t desc, void *buf, size_t buf_sz) = 0;
    virtual ssize_t get_dev_log(void *buf, size_t buf_sz) = 0;
    virtual hwstub_error exec_dev(uint32_t addr, uint16_t flags) = 0;

    std::shared_ptr<hwstub_device> m_dev; /* pointer to device */
    std::atomic<int> m_refcnt; /* reference count */
    std::recursive_mutex m_mutex; /* operation mutex to serialise operations */
};

#endif /* __HWSTUB_HPP__ */
