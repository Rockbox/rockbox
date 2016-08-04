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
#include <algorithm>
#include <cstring>

namespace hwstub {

std::ostream cnull(0);
std::wostream wcnull(0);

std::string error_string(error err)
{
    switch(err)
    {
        case error::SUCCESS: return "success";
        case error::ERROR: return "unspecified error";
        case error::DISCONNECTED: return "device was disconnected";
        case error::PROBE_FAILURE: return "probing failed";
        case error::NO_CONTEXT: return "context was destroyed";
        case error::USB_ERROR: return "unspecified USB error";
        case error::DUMMY: return "operation on dummy device";
        case error::NO_SERVER: return "server could not be reached";
        case error::SERVER_DISCONNECTED: return "server disconnected";
        case error::SERVER_MISMATCH: return "incompatible server";
        case error::NET_ERROR: return "network error";
        case error::PROTOCOL_ERROR: return "network protocol error";
        case error::TIMEOUT: return "timeout";
        case error::OVERFLW: return "overflow";
        case error::UNIMPLEMENTED: return "operation is not implemented";
        case error::UNSUPPORTED: return "operation unsupported";
        default: return "unknown error";
    }
}

/**
 * Context
 */

context::context()
    :m_next_cb_ref(0)
{
    clear_debug();
}

context::~context()
{
}

void context::set_debug(std::ostream& os)
{
    m_debug = &os;
}

std::ostream& context::debug()
{
    return *m_debug;
}

error context::get_device_list(std::vector<std::shared_ptr<device>>& list)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    error err = update_list();
    if(err != error::SUCCESS)
        return err;
    list.resize(m_devlist.size());
    for(size_t i = 0; i < m_devlist.size(); i++)
        list[i] = m_devlist[i];
    return error::SUCCESS;
}

error context::get_dummy_device(std::shared_ptr<device>& dev)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    dev.reset(new dummy_device(shared_from_this()));
    return error::SUCCESS;
}

void context::notify_device(bool arrived, std::shared_ptr<device> dev)
{
    for(auto cb : m_callbacks)
        cb.callback(shared_from_this(), arrived, dev);
}

void context::change_device(bool arrived, std::shared_ptr<device> dev)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if(arrived)
    {
        /* add to the list (assumed it's not already there) */
        m_devlist.push_back(dev);
        /* notify */
        notify_device(arrived, dev);
    }
    else
    {
        dev->disconnect();
        /* notify first */
        notify_device(arrived, dev);
        auto it = std::find(m_devlist.begin(), m_devlist.end(), dev);
        if(it != m_devlist.end())
            m_devlist.erase(it);
    }
}

error context::update_list()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* fetch new list */
    std::vector<ctx_dev_t> new_list;
    void* ptr;
    error err = fetch_device_list(new_list, ptr);
    if(err != error::SUCCESS)
        return err;
    /* determine devices that have left */
    std::vector<std::shared_ptr<device>> to_del;
    for(auto dev : m_devlist)
    {
        bool still_there = false;
        for(auto new_dev : new_list)
            if(match_device(new_dev, dev))
                still_there = true;
        if(!still_there)
            to_del.push_back(dev);
    }
    for(auto dev : to_del)
        change_device(false, dev);
    /* determine new devices */
    std::vector<ctx_dev_t> to_add;
    for(auto new_dev : new_list)
    {
        bool exists = false;
        for(auto dev : m_devlist)
            if(match_device(new_dev, dev))
                exists = true;
        if(!exists)
            to_add.push_back(new_dev);
    }
    /* create new devices */
    for(auto dev : to_add)
    {
        std::shared_ptr<device> new_dev;
        err = create_device(dev, new_dev);
        if(err == error::SUCCESS)
            change_device(true, new_dev);
    }
    /* destroy list */
    destroy_device_list(ptr);
    return error::SUCCESS;
}

context::callback_ref_t context::register_callback(const notification_callback_t& fn)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    struct callback_t cb;
    cb.callback = fn;
    cb.ref = m_next_cb_ref++;
    m_callbacks.push_back(cb);
    return cb.ref;
}

void context::unregister_callback(callback_ref_t ref)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    for(auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
    {
        if((*it).ref == ref)
        {
            m_callbacks.erase(it);
            return;
        }
    }
}

void context::start_polling(std::chrono::milliseconds interval)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* create poller on demand */
    if(!m_poller)
        m_poller.reset(new context_poller(shared_from_this(), interval));
    else
        m_poller->set_interval(interval);
    m_poller->start();
}

void context::stop_polling()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_poller->stop();
}

/**
 * Context Poller
 */

context_poller::context_poller(std::weak_ptr<context> ctx, std::chrono::milliseconds interval)
    :m_ctx(ctx), m_running(false), m_exit(false), m_interval(interval)
{
    m_thread = std::thread(context_poller::thread, this);
}

context_poller::~context_poller()
{
    /* set exit flag, wakeup thread, wait for exit */
    m_mutex.lock();
    m_exit = true;
    m_mutex.unlock();
    m_cond.notify_one();
    m_thread.join();
}

void context_poller::set_interval(std::chrono::milliseconds interval)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change interval, wakeup thread to take new interval into account */
    m_interval = interval;
    m_cond.notify_one();
}

void context_poller::start()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change running flag, wakeup thread to start polling */
    m_running = true;
    m_cond.notify_one();
}

void context_poller::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change running flag, wakeup thread to stop polling */
    m_running = false;
    m_cond.notify_one();
}

void context_poller::poll()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while(true)
    {
        /* if asked, exit */
        if(m_exit)
            break;
        /* if running, poll and then sleep for some time */
        if(m_running)
        {
            std::shared_ptr<context> ctx = m_ctx.lock();
            if(ctx)
                ctx->update_list();
            ctx.reset();
            m_cond.wait_for(lock, m_interval);
        }
        /* if not, sleep until awaken */
        else
            m_cond.wait(lock);
    }
}

void context_poller::thread(context_poller *poller)
{
    poller->poll();
}

/**
 * Device
 */
device::device(std::shared_ptr<context> ctx)
    :m_ctx(ctx), m_connected(true)
{
}

device::~device()
{
}

error device::open(std::shared_ptr<handle>& handle)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* do not even try if device is disconnected */
    if(!connected())
        return error::DISCONNECTED;
    /* NOTE at the moment handle is state-less which means that we can
     * safely give the same handle each time open() is called without callers
     * interfering with each other. If handle eventually get a state,
     * one will need to create a proxy class to encapsulate the state */
    handle = m_handle.lock();
    if(has_multiple_open() || !handle)
    {
        error err = open_dev(handle);
        m_handle = handle;
        return err;
    }
    else
        return error::SUCCESS;
}

void device::disconnect()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_connected = false;
}

bool device::connected()
{
    return m_connected;
}

std::shared_ptr<context> device::get_context()
{
    return m_ctx.lock();
}

/**
 * Handle
 */

handle::handle(std::shared_ptr<device > dev)
    :m_dev(dev)
{
}

handle::~handle()
{
}

std::shared_ptr<device> handle::get_device()
{
    return m_dev;
}

error handle::get_desc(uint16_t desc, void *buf, size_t& buf_sz)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    return get_dev_desc(desc, buf, buf_sz);
}

error handle::get_log(void *buf, size_t& buf_sz)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    return get_dev_log(buf, buf_sz);
}

error handle::exec(uint32_t addr, uint16_t flags)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    return exec_dev(addr, flags);
}

error handle::read(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    /* split transfer as needed */
    size_t cnt = 0;
    uint8_t *bufp = (uint8_t *)buf;
    while(sz > 0)
    {
        size_t xfer = std::min(sz, get_buffer_size());
        err = read_dev(addr, buf, xfer, atomic);
        if(err != error::SUCCESS)
            return err;
        sz -= xfer;
        bufp += xfer;
        addr += xfer;
        cnt += xfer;
    }
    sz = cnt;
    return error::SUCCESS;
}

error handle::write(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    /* split transfer as needed */
    size_t cnt = 0;
    const uint8_t *bufp = (uint8_t *)buf;
    while(sz > 0)
    {
        size_t xfer = std::min(sz, get_buffer_size());
        err = write_dev(addr, buf, xfer, atomic);
        if(err != error::SUCCESS)
            return err;
        sz -= xfer;
        bufp += xfer;
        addr += xfer;
        cnt += xfer;
    }
    sz = cnt;
    return error::SUCCESS;
}

error handle::cop_op(uint8_t op, uint8_t args[HWSTUB_COP_ARGS], const void *out_data,
    size_t out_size, void *in_data, size_t *in_size)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<context> ctx = m_dev->get_context();
    if(!ctx)
        return error::NO_CONTEXT;
    /* ensure valid status */
    error err = status();
    if(err != error::SUCCESS)
        return err;
    return cop_dev(op, args, out_data, out_size, in_data, in_size);
}

error handle::read32_cop(uint8_t args[HWSTUB_COP_ARGS], uint32_t& value)
{
    size_t sz = sizeof(value);
    error err = cop_op(HWSTUB_COP_READ, args, nullptr, 0, &value, &sz);
    if(err != error::SUCCESS)
        return err;
    if(sz != sizeof(value))
        return error::ERROR;
    return error::SUCCESS;
}

error handle::write32_cop(uint8_t args[HWSTUB_COP_ARGS], uint32_t value)
{
    return cop_op(HWSTUB_COP_WRITE, args, &value, sizeof(value), nullptr, nullptr);
}

error handle::status() const
{
    /* check context */
    if(!m_dev->get_context())
        return error::NO_CONTEXT;
    if(!m_dev->connected())
        return error::DISCONNECTED;
    else
        return error::SUCCESS;
}

namespace
{
    template<typename T>
    error helper_get_desc(handle *h, uint8_t type, T& desc)
    {
        size_t sz = sizeof(desc);
        error ret = h->get_desc(type, &desc, sz);
        if(ret != error::SUCCESS)
            return ret;
        if(sz != sizeof(desc) || desc.bDescriptorType != type ||
                desc.bLength != sizeof(desc))
            return error::ERROR;
        else
            return error::SUCCESS;
    }
}

error handle::get_version_desc(hwstub_version_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_VERSION, desc);
}

error handle::get_layout_desc(hwstub_layout_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_LAYOUT, desc);
}

error handle::get_stmp_desc(hwstub_stmp_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_STMP, desc);
}

error handle::get_pp_desc(hwstub_pp_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_PP, desc);
}

error handle::get_jz_desc(hwstub_jz_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_JZ, desc);
}

error handle::get_target_desc(hwstub_target_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_TARGET, desc);
}

/** Dummy device */
dummy_device::dummy_device(std::shared_ptr<context> ctx)
    :device(ctx)
{
}

dummy_device::~dummy_device()
{
}

error dummy_device::open_dev(std::shared_ptr<handle>& handle)
{
    handle.reset(new dummy_handle(shared_from_this()));
    return error::SUCCESS;
}

bool dummy_device::has_multiple_open() const
{
    return true;
}

/** Dummy handle */
dummy_handle::dummy_handle(std::shared_ptr<device> dev)
    :handle(dev)
{
    m_desc_version.bLength = sizeof(m_desc_version);
    m_desc_version.bDescriptorType = HWSTUB_DT_VERSION;
    m_desc_version.bMajor = HWSTUB_VERSION_MAJOR;
    m_desc_version.bMinor = HWSTUB_VERSION_MINOR;
    m_desc_version.bRevision = 0;

    m_desc_layout.bLength = sizeof(m_desc_layout);
    m_desc_layout.bDescriptorType = HWSTUB_DT_LAYOUT;
    m_desc_layout.dCodeStart = 0;
    m_desc_layout.dCodeSize = 0;
    m_desc_layout.dStackStart = 0;
    m_desc_layout.dStackSize = 0;
    m_desc_layout.dBufferStart = 0;
    m_desc_layout.dBufferSize = 1;

    m_desc_target.bLength = sizeof(m_desc_target);
    m_desc_target.bDescriptorType = HWSTUB_DT_TARGET;
    m_desc_target.dID = HWSTUB_TARGET_UNK;
    strcpy(m_desc_target.bName, "Dummy target");
}

dummy_handle::~dummy_handle()
{
}

error dummy_handle::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error dummy_handle::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error dummy_handle::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
{
    void *p = nullptr;
    switch(desc)
    {
        case HWSTUB_DT_VERSION: p = &m_desc_version; break;
        case HWSTUB_DT_LAYOUT: p = &m_desc_layout; break;
        case HWSTUB_DT_TARGET: p = &m_desc_target; break;
        default: break;
    }
    if(p == nullptr)
        return error::ERROR;
    /* size is in the bLength field of the descriptor */
    size_t desc_sz = *(uint8_t *)p;
    buf_sz = std::min(buf_sz, desc_sz);
    memcpy(buf, p, buf_sz);
    return error::SUCCESS;
}

error dummy_handle::get_dev_log(void *buf, size_t& buf_sz)
{
    (void) buf;
    (void) buf_sz;
    return error::DUMMY;
}

error dummy_handle::exec_dev(uint32_t addr, uint16_t flags)
{
    (void) addr;
    (void) flags;
    return error::DUMMY;
}

error dummy_handle::cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS],
    const void *out_data, size_t out_size, void *in_data, size_t *in_size)
{
    (void) op;
    (void) args;
    (void) out_data;
    (void) out_size;
    (void) in_data;
    (void) in_size;
    return error::DUMMY;
}

error dummy_handle::status() const
{
    error err = handle::status();
    return err == error::SUCCESS ? error::DUMMY : err;
}

size_t dummy_handle::get_buffer_size()
{
    return 1;
}

} // namespace hwstub
