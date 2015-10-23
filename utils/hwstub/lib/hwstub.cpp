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
    for(auto d : m_devlist)
        delete d;
}

hwstub_error hwstub_context::get_device_list(std::vector< hwstub_device* >& list, bool notify)
{
    hwstub_error err = update_list(notify);
    if(err != HWSTUB_SUCCESS)
        return err;
    lock();
    list.resize(m_devlist.size());
    for(size_t i = 0; i < m_devlist.size(); i++)
    {
        list[i] = m_devlist[i];
        m_devlist[i]->ref();
    }
    unlock();
    return HWSTUB_SUCCESS;
}

void hwstub_context::lock()
{
    m_mutex.lock();
}

void hwstub_context::unlock()
{
    m_mutex.unlock();
}

void hwstub_context::notify_device(bool arrived, hwstub_device *dev)
{
}

void hwstub_context::change_device(bool arrived, hwstub_device *dev, bool notify)
{
    lock();
    if(arrived)
    {
        /* add a reference to the device */
        dev->ref();
        /* add to the list (assumed it's not already there) */
        m_devlist.push_back(dev);
        /* notify */
        if(notify)
            notify_device(arrived, dev);
    }
    else
    {
        /* notify first */
        if(notify)
            notify_device(arrived, dev);
        auto it = std::find(m_devlist.begin(), m_devlist.end(), dev);
        if(it != m_devlist.end())
            m_devlist.erase(it);
        /* remove reference */
        dev->unref();
    }
    unlock();
}

hwstub_error hwstub_context::update_list(bool notify)
{
    lock();
    /* fetch new list */
    std::vector< hwstub_ctx_dev_t > new_list;
    void* ptr;
    hwstub_error err = fetch_device_list(new_list, ptr);
    if(err != HWSTUB_SUCCESS)
    {
        unlock();
        return err;
    }
    /* determine devices that have left */
    std::vector< hwstub_device* > to_del;
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
        change_device(false, dev, notify);
    /* determine new devices */
    std::vector< hwstub_ctx_dev_t > to_add;
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
        hwstub_device *new_dev = nullptr;
        err = create_device(dev, new_dev);
        if(err == HWSTUB_SUCCESS)
            change_device(true, new_dev, notify);
    }
    /* destroy list */
    destroy_device_list(ptr);
    unlock();
    return HWSTUB_SUCCESS;
}

/**
 * Device
 */
hwstub_device::hwstub_device(hwstub_context *ctx)
    :m_ctx(ctx), m_refcnt(0)
{
}

hwstub_device::~hwstub_device()
{
}

hwstub_error hwstub_device::open(hwstub_handle *& handle)
{
    return HWSTUB_ERROR;
}

void hwstub_device::ref()
{
    m_refcnt++; /* atomic operation thanks to atomic<> */
}

void hwstub_device::unref()
{
    /* atomic operation thanks to atomic<> */
    if(--m_refcnt == 0)
        delete this;
}

void hwstub_device::lock()
{
    m_mutex.lock();
}

void hwstub_device::unlock()
{
    m_mutex.unlock();
}

void hwstub_device::disconnect()
{
    lock();
    m_connected = false;
    m_ctx->change_device(false, this);
    unlock();
}

bool hwstub_device::connected()
{
    return m_connected;
}
