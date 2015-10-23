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

/**
 * Context
 */

hwstub_context::hwstub_context()
{
}

hwstub_context::~hwstub_context()
{
}

hwstub_error hwstub_context::get_device_list(std::vector<std::shared_ptr<hwstub_device>>& list)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    hwstub_error err = update_list();
    if(err != HWSTUB_SUCCESS)
        return err;
    list.resize(m_devlist.size());
    for(size_t i = 0; i < m_devlist.size(); i++)
        list[i] = m_devlist[i];
    return HWSTUB_SUCCESS;
}

void hwstub_context::notify_device(bool arrived, std::shared_ptr<hwstub_device> dev)
{
    for(auto fn : m_callbacks)
        fn(shared_from_this(), arrived, dev);
}

void hwstub_context::change_device(bool arrived, std::shared_ptr<hwstub_device> dev)
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

hwstub_error hwstub_context::update_list()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* fetch new list */
    std::vector<hwstub_ctx_dev_t> new_list;
    void* ptr;
    hwstub_error err = fetch_device_list(new_list, ptr);
    if(err != HWSTUB_SUCCESS)
        return err;
    /* determine devices that have left */
    std::vector<std::shared_ptr<hwstub_device>> to_del;
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
    std::vector<hwstub_ctx_dev_t> to_add;
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
        std::shared_ptr<hwstub_device> new_dev;
        err = create_device(dev, new_dev);
        if(err == HWSTUB_SUCCESS)
            change_device(true, new_dev);
    }
    /* destroy list */
    destroy_device_list(ptr);
    return HWSTUB_SUCCESS;
}

void hwstub_context::register_callback(const notification_callback_t& fn)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_callbacks.push_back(fn);
}

void hwstub_context::start_polling(std::chrono::milliseconds interval)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* create poller on demand */
    if(!m_poller)
        m_poller.reset(new hwstub_context_poller(shared_from_this(), interval));
    else
        m_poller->set_interval(interval);
    m_poller->start();
}

void hwstub_context::stop_polling()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_poller->stop();
}

/**
 * Context Poller
 */

hwstub_context_poller::hwstub_context_poller(std::weak_ptr<hwstub_context> ctx, std::chrono::milliseconds interval)
    :m_ctx(ctx), m_running(false), m_exit(false), m_interval(interval)
{
    m_thread = std::thread(hwstub_context_poller::thread, this);
}

hwstub_context_poller::~hwstub_context_poller()
{
    /* set exit flag, wakeup thread, wait for exit */
    m_mutex.lock();
    m_exit = true;
    m_mutex.unlock();
    m_cond.notify_one();
    m_thread.join();
}

void hwstub_context_poller::set_interval(std::chrono::milliseconds interval)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change interval, wakeup thread to take new interval into account */
    m_interval = interval;
    m_cond.notify_one();
}

void hwstub_context_poller::start()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change running flag, wakeup thread to start polling */
    m_running = true;
    m_cond.notify_one();
}

void hwstub_context_poller::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    /* change running flag, wakeup thread to stop polling */
    m_running = false;
    m_cond.notify_one();
}

void hwstub_context_poller::poll()
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
            std::shared_ptr<hwstub_context> ctx = m_ctx.lock();
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

void hwstub_context_poller::thread(hwstub_context_poller *poller)
{
    poller->poll();
}

/**
 * Device
 */
hwstub_device::hwstub_device(std::shared_ptr<hwstub_context> ctx)
    :m_ctx(ctx), m_connected(true)
{
}

hwstub_device::~hwstub_device()
{
}

hwstub_error hwstub_device::open(std::shared_ptr<hwstub_handle>& handle)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* do not even try if device is disconnected */
    if(!connected())
        return HWSTUB_DISCONNECTED;
    /* NOTE at the moment hwstub_handle is state-less which means that we can
     * safely give the same handle each time open() is called without callers
     * interfering with each other. If hwstub_handle eventually get a state,
     * one will need to create a proxy class to encapsulate the state */
    handle = m_handle.lock();
    if(has_multiple_open() || !handle)
    {
        hwstub_error err = open_dev(handle);
        m_handle = handle;
        return err;
    }
    else
        return HWSTUB_SUCCESS;
}

void hwstub_device::disconnect()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_connected = false;
}

bool hwstub_device::connected()
{
    return m_connected;
}

std::shared_ptr<hwstub_context> hwstub_device::get_context()
{
    return m_ctx.lock();
}

/**
 * Handle
 */

hwstub_handle::hwstub_handle(std::shared_ptr<hwstub_device > dev)
    :m_dev(dev)
{
}

hwstub_handle::~hwstub_handle()
{
}

std::shared_ptr<hwstub_device> hwstub_handle::get_device()
{
    return m_dev;
}

ssize_t hwstub_handle::get_desc(uint16_t desc, void *buf, size_t buf_sz)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = m_dev->get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* ensure valid status */
    hwstub_error err = status();
    if(err != HWSTUB_SUCCESS)
        return err;
    return get_dev_desc(desc, buf, buf_sz);
}

ssize_t hwstub_handle::get_log(void *buf, size_t buf_sz)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = m_dev->get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* ensure valid status */
    hwstub_error err = status();
    if(err != HWSTUB_SUCCESS)
        return err;
    return get_dev_log(buf, buf_sz);
}

ssize_t hwstub_handle::exec(uint32_t addr, uint16_t flags)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = m_dev->get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* ensure valid status */
    hwstub_error err = status();
    if(err != HWSTUB_SUCCESS)
        return err;
    return exec_dev(addr, flags);
}

ssize_t hwstub_handle::read(uint8_t breq, uint32_t addr, void *buf, size_t sz, bool atomic)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = m_dev->get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* ensure valid status */
    hwstub_error err = status();
    if(err != HWSTUB_SUCCESS)
        return err;
    /* split transfer as needed */
    ssize_t cnt = 0;
    while(sz > 0)
    {
        ssize_t xfer = read_dev(breq, addr, buf, std::min(sz, get_buffer_size()), atomic);
        if(xfer <  0)
            return xfer;
        sz -= xfer;
        buf += xfer;
        addr += xfer;
        cnt += xfer;
    }
    return cnt;
}

ssize_t hwstub_handle::write(uint8_t breq, uint32_t addr, const void *buf, size_t sz, bool atomic)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* get a pointer so that it's not destroyed during the runtime of the function,
     * the pointer will be released at the end of the function */
    std::shared_ptr<hwstub_context> ctx = m_dev->get_context();
    if(!ctx)
        return HWSTUB_NO_CONTEXT;
    /* ensure valid status */
    hwstub_error err = status();
    if(err != HWSTUB_SUCCESS)
        return err;
    /* split transfer as needed */
    ssize_t cnt = 0;
    while(sz > 0)
    {
        ssize_t xfer = write_dev(breq, addr, buf, std::min(sz, get_buffer_size()), atomic);
        if(xfer <  0)
            return xfer;
        sz -= xfer;
        buf += xfer;
        addr += xfer;
        cnt += xfer;
    }
    return cnt;
}

hwstub_error hwstub_handle::status() const
{
    /* check context */
    if(!m_dev->get_context())
        return HWSTUB_NO_CONTEXT;
    if(!m_dev->connected())
        return HWSTUB_DISCONNECTED;
    else
        return HWSTUB_SUCCESS;
}

namespace
{
    template<typename T>
    hwstub_error helper_get_desc(hwstub_handle *h, uint8_t type, T& desc)
    {
        ssize_t ret = h->get_desc(type, &desc, sizeof(desc));
        if(ret < 0)
            return (hwstub_error)ret;
        if(ret != (ssize_t)sizeof(desc) || desc.bDescriptorType != type ||
                desc.bLength != sizeof(desc))
            return HWSTUB_ERROR;
        else
            return HWSTUB_SUCCESS;
    }
}

hwstub_error hwstub_handle::get_version_desc(hwstub_version_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_VERSION, desc);
}

hwstub_error hwstub_handle::get_layout_desc(hwstub_layout_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_LAYOUT, desc);
}

hwstub_error hwstub_handle::get_stmp_desc(hwstub_stmp_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_STMP, desc);
}

hwstub_error hwstub_handle::get_pp_desc(hwstub_pp_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_PP, desc);
}

hwstub_error hwstub_handle::get_jz_desc(hwstub_jz_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_JZ, desc);
}

hwstub_error hwstub_handle::get_target_desc(hwstub_target_desc_t& desc)
{
    return helper_get_desc(this, HWSTUB_DT_TARGET, desc);
}
