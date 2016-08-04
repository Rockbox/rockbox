/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "hwstub_virtual.hpp"
#include <algorithm>
#include <cstring>

namespace hwstub {
namespace virt {

/**
 * Context
 */
context::context()
{
}

context::~context()
{
}

std::shared_ptr<context> context::create()
{
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<context>(new context());
}

std::shared_ptr<context> context::create_spec(const std::string& spec, std::string *error)
{
    std::shared_ptr<context> ctx = create();
    /* parse spec */
    std::string str = spec;
    while(!str.empty())
    {
        /* find next separator, if any */
        std::string dev_spec;
        size_t dev_sep = str.find(';');
        if(dev_sep == std::string::npos)
        {
            dev_spec = str;
            str.clear();
        }
        else
        {
            dev_spec = str.substr(0, dev_sep);
            str = str.substr(dev_sep + 1);
        }
        /* handle dev spec: find ( and )*/
        size_t lparen = dev_spec.find('(');
        if(lparen == std::string::npos)
        {
            if(error)
                *error = "invalid device spec '" + dev_spec + "': missing (";
            return std::shared_ptr<context>();
        }
        if(dev_spec.back() != ')')
        {
            if(error)
                *error = "invalid device spec '" + dev_spec + "': missing )";
            return std::shared_ptr<context>();
        }
        std::string args = dev_spec.substr(lparen + 1, dev_spec.size() - lparen - 2);
        std::string type = dev_spec.substr(0, lparen);
        if(type == "dummy")
        {
            ctx->connect(std::make_shared<dummy_hardware>(args));
        }
        else
        {
            if(error)
                *error = "invalid device spec '" + dev_spec + "': unknown device type";
            return std::shared_ptr<context>();
        }
    }
    return ctx;
}

bool context::connect(std::shared_ptr<hardware> hw)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if(std::find(m_hwlist.begin(), m_hwlist.end(), hw) != m_hwlist.end())
        return false;
    m_hwlist.push_back(hw);
    return true;
}

bool context::disconnect(std::shared_ptr<hardware> hw)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    auto it = std::find(m_hwlist.begin(), m_hwlist.end(), hw);
    if(it == m_hwlist.end())
        return false;
    m_hwlist.erase(it);
    return true;
}

std::shared_ptr<hardware> context::from_ctx_dev(ctx_dev_t dev)
{
    return ((hardware *)dev)->shared_from_this();
}

hwstub::context::ctx_dev_t context::to_ctx_dev(std::shared_ptr<hardware>& dev)
{
   return (ctx_dev_t)dev.get();
}

error context::fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr)
{
    (void) ptr;
    list.resize(m_hwlist.size());
    for(size_t i = 0; i < m_hwlist.size(); i++)
        list[i] = to_ctx_dev(m_hwlist[i]);
    return error::SUCCESS;
}

void context::destroy_device_list(void *ptr)
{
    (void) ptr;
}

error context::create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    hwdev.reset(new device(shared_from_this(), from_ctx_dev(dev)));
    return error::SUCCESS;
}

bool context::match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev)
{
    device *udev = dynamic_cast<device*>(hwdev.get());
    return udev != nullptr && udev->native_device().get() == from_ctx_dev(dev).get();
}

/**
 * Device
 */
device::device(std::shared_ptr<hwstub::context> ctx, std::shared_ptr<hardware> dev)
    :hwstub::device(ctx), m_hwdev(dev)
{
}

device::~device()
{
}

std::shared_ptr<hardware> device::native_device()
{
    return m_hwdev.lock();
}

error device::open_dev(std::shared_ptr<hwstub::handle>& h)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    h.reset(new handle(shared_from_this()));
    return error::SUCCESS;
}

bool device::has_multiple_open() const
{
    return true;
}

/**
 * Handle
 */
handle::handle(std::shared_ptr<hwstub::device> dev)
    :hwstub::handle(dev)
{
    m_hwdev = dynamic_cast<device*>(dev.get())->native_device();
}

handle::~handle()
{
}


error handle::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    auto p = m_hwdev.lock();
    return p ? p->read_dev(addr, buf, sz, atomic) : error::DISCONNECTED;
}

error handle::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    auto p = m_hwdev.lock();
    return p ? p->write_dev(addr, buf, sz, atomic) : error::DISCONNECTED;
}

error handle::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
{
    auto p = m_hwdev.lock();
    return p ? p->get_dev_desc(desc, buf, buf_sz) : error::DISCONNECTED;
}

error handle::get_dev_log(void *buf, size_t& buf_sz)
{
    auto p = m_hwdev.lock();
    return p ? p->get_dev_log(buf, buf_sz) : error::DISCONNECTED;
}

error handle::exec_dev(uint32_t addr, uint16_t flags)
{
    auto p = m_hwdev.lock();
    return p ? p->exec_dev(addr, flags) : error::DISCONNECTED;
}

error handle::cop_dev(uint8_t op, uint8_t args[HWSTUB_COP_ARGS],
    const void *out_data, size_t out_size, void *in_data, size_t *in_size)
{
    (void) op;
    (void) args;
    (void) out_data;
    (void) out_size;
    (void) in_data;
    (void) in_size;
    return error::UNSUPPORTED;
}

error handle::status() const
{
    return hwstub::handle::status();
}

size_t handle::get_buffer_size()
{
    auto p = m_hwdev.lock();
    return p ? p->get_buffer_size() : 1;
}

/**
 * Hardware
 */
hardware::hardware()
{
}

hardware::~hardware()
{
}

/**
 * Dummy hardware
 */
dummy_hardware::dummy_hardware(const std::string& name)
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
    strncpy(m_desc_target.bName, name.c_str(), sizeof(m_desc_target.bName));
    m_desc_target.bName[sizeof(m_desc_target.bName) - 1] = 0;
}

dummy_hardware::~dummy_hardware()
{
}

error dummy_hardware::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error dummy_hardware::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error dummy_hardware::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
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

error dummy_hardware::get_dev_log(void *buf, size_t& buf_sz)
{
    (void) buf;
    (void) buf_sz;
    return error::DUMMY;
}

error dummy_hardware::exec_dev(uint32_t addr, uint16_t flags)
{
    (void) addr;
    (void) flags;
    return error::DUMMY;
}

size_t dummy_hardware::get_buffer_size()
{
    return 1;
}

} // namespace virt
} // namespace net

