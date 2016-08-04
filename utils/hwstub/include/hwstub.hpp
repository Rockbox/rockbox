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
#include <ostream>

namespace hwstub {

class context;
class device;
class handle;
class context_poller;

/** C++ equivalent of /dev/null for streams */
extern std::ostream cnull;
extern std::wostream wcnull;

/** Errors */
enum class error
{
    SUCCESS, /** Success */
    ERROR, /** Unspecified error */
    DISCONNECTED, /** Device has been disconnected */
    PROBE_FAILURE, /** Device did not pass probing */
    NO_CONTEXT, /** The context has been destroyed */
    USB_ERROR, /** Unspecified USB error */
    DUMMY, /** Call on dummy device/handle */
    NO_SERVER, /** The server could not be reached */
    SERVER_DISCONNECTED, /** Context got disconnected from the server */
    SERVER_MISMATCH, /** The server is not compatible with hwstub */
    PROTOCOL_ERROR, /** Network protocol error */
    NET_ERROR, /** Network error */
    TIMEOUT, /** Operation timed out */
    OVERFLW, /** Operation stopped to prevent buffer overflow */
    UNIMPLEMENTED, /** Operation has not been implemented */
    UNSUPPORTED, /** Operation is not supported by device/protocol */
};

/** Return a string explaining the error */
std::string error_string(error err);

/** NOTE Multithreading:
 * Unless specified, all methods are thread-safe
 */

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
class context : public std::enable_shared_from_this<context>
{
protected:
    context();
public:
    /** On destruction, the context will destroy all the devices. */
    virtual ~context();
    /** Get device list, clears the list in argument first. All devices in the list
     * are still connected (or believe to be). This function will update the device
     * list. */
    error get_device_list(std::vector<std::shared_ptr<device>>& list);
    /** Force the context to update its internal list of devices. */
    error update_list();
    /** Ask the context to automatically poll for device changes.
     * Note that this might spawn a new thread to do so, in which case it will
     * be destroyed/stop on deletetion or when stop_polling() is called. If
     * polling is already enabled, this function will change the polling interval. */
    void start_polling(std::chrono::milliseconds interval = std::chrono::milliseconds(250));
    /** Stop polling. */
    void stop_polling();
    /** Register a notification callback with arguments (context,arrived,device)
     * WARNING the callback may be called asynchronously ! */
    typedef std::function<void(std::shared_ptr<context>, bool, std::shared_ptr<device>)> notification_callback_t;
    typedef size_t callback_ref_t;
    callback_ref_t register_callback(const notification_callback_t& fn);
    void unregister_callback(callback_ref_t ref);
    /** Return a dummy device that does nothing. A dummy device might be useful
     * in cases where one still wants a valid pointer to no device. This dummy
     * device does not appear in the list, it can be opened and will fail all requests. */
    error get_dummy_device(std::shared_ptr<device>& dev);
    /** Set/clear debug output for this context */
    void set_debug(std::ostream& os);
    inline void clear_debug() { set_debug(cnull); }
    /** Get debug output for this context */
    std::ostream& debug();

protected:
    /** Notify the context about a device. If arrived is true, the device is
     * added to the list and a reference will be added to it. If arrived is false,
     * the device is marked as disconnected(), removed from the list and a
     * reference will be removed from it. Adding a device that matches an
     * existing one will do nothing. */
    void change_device(bool arrived, std::shared_ptr<device> dev);
    /** Do device notification */
    void notify_device(bool arrived, std::shared_ptr<device> dev);
    /** Opaque device type */
    typedef void* ctx_dev_t;
    /** Fetch the device list. Each item in the list is an opaque pointer. The function
     * can also provide a pointer that will be used to free the list resources
     * if necessary. Return <0 on error. */
    virtual error fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr) = 0;
    /** Destroy the resources created to get the list. */
    virtual void destroy_device_list(void *ptr) = 0;
    /** Create a new hwstub device from the opaque pointer. Return <0 on error.
     * This function needs not add a reference to the newly created device. */
    virtual error create_device(ctx_dev_t dev, std::shared_ptr<device>& hwdev) = 0;
    /** Return true if the opaque pointer corresponds to the device. Only called
     * from map_device(). */
    virtual bool match_device(ctx_dev_t dev, std::shared_ptr<device> hwdev) = 0;
    /** Check if a device matches another one in the list */
    bool contains_dev(const std::vector<device*>& list, ctx_dev_t dev);

    struct callback_t
    {
        notification_callback_t callback;
        callback_ref_t ref;
    };

    std::shared_ptr<context_poller> m_poller; /* poller object */
    std::recursive_mutex m_mutex; /* list mutex */
    std::vector<std::shared_ptr<device>> m_devlist; /* list of devices */
    std::vector<callback_t> m_callbacks; /* list of callbacks */
    callback_ref_t m_next_cb_ref; /* next callback reference */
    std::ostream *m_debug; /* debug stream */
};

/** Context Poller
 *
 * This class provides a way to regularly poll a context for device changes.
 * NOTE this class is not meant to be used directly since context already
 * provides access to it via start_polling() and stop_polling() */
class context_poller
{
public:
    context_poller(std::weak_ptr<context> ctx, std::chrono::milliseconds interval = std::chrono::milliseconds(250));
    ~context_poller();
    /** Set polling interval (in milliseconds) (works even if polling already enabled) */
    void set_interval(std::chrono::milliseconds interval);
    /** Start polling */
    void start();
    /** Stop polling. After return, no function will be made. */
    void stop();

protected:
    static void thread(context_poller *poller);
    void poll();

    std::weak_ptr<context> m_ctx; /* context */
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
class device : public std::enable_shared_from_this<device>
{
protected:
    device(std::shared_ptr<context> ctx);
public:
    virtual ~device();
    /** Open a handle to the device. Several handles may be opened concurrently. */
    error open(std::shared_ptr<handle>& handle);
    /** Disconnect the device. This will notify the context that the device is gone. */
    void disconnect();
    /** Returns true if the device is still connected. */
    bool connected();
    /** Get context (might be empty) */
    std::shared_ptr<context> get_context();

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
    virtual error open_dev(std::shared_ptr<handle>& handle) = 0;
    /** Return true if device can be opened multiple times. In this case, each
     * call to open() will generate a call to do_open(). Otherwise, proxy handles
     * will be created for each open() and do_open() will only be called the first
     * time. */
    virtual bool has_multiple_open() const = 0;

    std::weak_ptr<context> m_ctx; /* pointer to context */
    std::recursive_mutex m_mutex; /* device state mutex: ref count, connection status */
    bool m_connected; /* false once device is disconnected */
    std::weak_ptr<handle> m_handle; /* weak pointer to the opened handle (if !has_multiple_open()) */
};

/** Handle
 *
 * A handle is tied to a device and provides access to the stub operation.
 * The handle is reference counted and is destroyed
 * when its reference count decreased to zero.
 */
class handle : public std::enable_shared_from_this<handle>
{
protected:
    /** A handle will always hold a reference to the device */
    handle(std::shared_ptr<device> dev);
public:
    /** When destroyed, the handle will release its reference to the device */
    virtual ~handle();
    /** Return associated device */
    std::shared_ptr<device> get_device();
    /** Fetch a descriptor, buf_sz is the size of the buffer and is updated to
     * reflect the number of bytes written to the buffer. */
    error get_desc(uint16_t desc, void *buf, size_t& buf_sz);
    /** Fetch part of the log, buf_sz is the size of the buffer and is updated to
     * reflect the number of bytes written to the buffer. */
    error get_log(void *buf, size_t& buf_sz);
    /** Ask the stub to execute some code.
     * NOTE: this may kill the stub */
    error exec(uint32_t addr, uint16_t flags);
    /** Read/write some device memory. sz is the size of the buffer and is updated to
     * reflect the number of bytes written to the buffer.
     * NOTE: the stub may or may not recover from bad read/write, so this may kill it.
     * NOTE: the default implemtentation of read() and write() will split transfers
     *       according to the buffer size and call read_dev() and write_dev() */
    error read(uint32_t addr, void *buf, size_t& sz, bool atomic);
    error write(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    /** Execute a general cop operation: if out_data is not null, data is appended to header,
     * if in_data is not null, a read operation follows to retrieve some data.
     * The in_data parameters is updated to reflect the number of transfered bytes */
    error cop_op(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
        size_t out_size, void *in_data, size_t *in_size);
    /** Execute a coprocessor read operation */
    error read32_cop(uint8_t args[HWSTUB_COP_ARGS], uint32_t& value);
    /** Execute a coprocessor write operation */
    error write32_cop(uint8_t args[HWSTUB_COP_ARGS], uint32_t value);
    /** Get device buffer size: any read() or write() greater than this size
     * will be split into several transaction to avoid overflowing device
     * buffer. */
    virtual size_t get_buffer_size() = 0;
    /** Check a handle status. A successful handle does not guarantee successful
     * operations. An invalid handle will typically report probing failure (the
     * device did not pass probing) or disconnection.
     * The default implemtentation will test if context still exists and connection status. */
    virtual error status() const;
    /** Shorthand for status() == HWSTUB_SUCCESS */
    inline bool valid() const { return status() == error::SUCCESS; }

    /** Helper functions */
    error get_version_desc(hwstub_version_desc_t& desc);
    error get_layout_desc(hwstub_layout_desc_t& desc);
    error get_stmp_desc(hwstub_stmp_desc_t& desc);
    error get_pp_desc(hwstub_pp_desc_t& desc);
    error get_jz_desc(hwstub_jz_desc_t& desc);
    error get_target_desc(hwstub_target_desc_t& desc);

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
    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic) = 0;
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic) = 0;
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz) = 0;
    virtual error get_dev_log(void *buf, size_t& buf_sz) = 0;
    virtual error exec_dev(uint32_t addr, uint16_t flags) = 0;
    /* cop operation: if out_data is not null, data is appended to header, if
     * in_data is not null, a READ2 operation follows to retrieve some data
     * The in_data parameters is updated to reflect the number of transfered bytes*/
    virtual error cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
        size_t out_size, void *in_data, size_t *in_size) = 0;

    std::shared_ptr<device> m_dev; /* pointer to device */
    std::atomic<int> m_refcnt; /* reference count */
    std::recursive_mutex m_mutex; /* operation mutex to serialise operations */
};

/** Dummy device */
class dummy_device : public device
{
    friend class context; /* for ctor */
protected:
    dummy_device(std::shared_ptr<context> ctx);
public:
    virtual ~dummy_device();

protected:
    virtual error open_dev(std::shared_ptr<handle>& handle);
    virtual bool has_multiple_open() const;
};

/** Dummy handle */
class dummy_handle : public handle
{
    friend class dummy_device;
protected:
    dummy_handle(std::shared_ptr<device> dev);
public:
    virtual ~dummy_handle();

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

    struct hwstub_version_desc_t m_desc_version;
    struct hwstub_layout_desc_t m_desc_layout;
    struct hwstub_target_desc_t m_desc_target;
};

} // namespace hwstub

#endif /* __HWSTUB_HPP__ */
