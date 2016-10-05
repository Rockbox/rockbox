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
#include "hwstub_gdb.hpp"
#include <algorithm>
#include <cstring>

namespace hwstub {
namespace gdb {

/**
 * Context
 */
context::context(int socket_fd)
{
    m_conn.init(socket_fd);
}

context::~context()
{
}

std::shared_ptr<context> context::create(int socket_fd)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<context>(new context(socket_fd));
}

std::shared_ptr<context> context::create_tcp(const std::string& domain,
    const std::string& port, std::string *error)
{
    int fd = hwstub::net::socket_connection::create_tcp(domain, port, error);
    if(fd >= 0)
        return context::create(fd);
    else
        return std::shared_ptr<context>();
}

error context::fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr)
{
    (void) ptr;
    list.resize(1);
    list[0] = 0;
    return error::SUCCESS;
}

void context::destroy_device_list(void *ptr)
{
    (void) ptr;
}

error context::create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev)
{
    /* there is only one device, so we don't use dev */
    (void) dev;
    // NOTE: can't use make_shared() because of the protected ctor */
    hwdev.reset(new device(shared_from_this()));
    return error::SUCCESS;
}

bool context::match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev)
{
    (void) dev;
    (void) hwdev;
    /* there is only one device, so they all match */
    return true;
}

/**
 * Device
 */
device::device(std::shared_ptr<hwstub::context> ctx)
    :hwstub::device(ctx)
{
}

device::~device()
{
}

error device::open_dev(std::shared_ptr<hwstub::handle>& h)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    h.reset(new handle(shared_from_this()));
    return error::SUCCESS;
}

bool device::has_multiple_open() const
{
    /* we want to serialize accesses, otherwise multiple concurrent messages to
     * the server over the same connection will confuse the server */
    return false;
}

/**
 * Handle
 */
handle::handle(std::shared_ptr<hwstub::device> dev)
    :hwstub::handle(dev)
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
    strncpy(m_desc_target.bName, "GDB server", sizeof(m_desc_target.bName));
    m_desc_target.bName[sizeof(m_desc_target.bName) - 1] = 0;
}

handle::~handle()
{
}


error handle::read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic)
{
    std::shared_ptr<hwstub::context> hctx = get_device()->get_context();
    if(!hctx)
        return error::NO_CONTEXT;
    context *ctx = dynamic_cast<context*>(hctx.get());
    ctx->debug() << "[gdb::handle] --> READ(0x" << std::hex
        << addr << "," << sz << "," << atomic << ")\n";
    ctx->debug() << "[gdb::handle] <-- READ unimplemented\n";
    /* implement read here using ctx->send() and ctx->recv() */
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error handle::write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic)
{
    std::shared_ptr<hwstub::context> hctx = get_device()->get_context();
    if(!hctx)
        return error::NO_CONTEXT;
    context *ctx = dynamic_cast<context*>(hctx.get());
    ctx->debug() << "[gdb::handle] --> WRITE(0x" << std::hex
        << addr << "," << sz << "," << atomic << ")\n";
    ctx->debug() << "[gdb::handle] <-- WRITE unimplemented\n";
    /* implement read here using ctx.send() and ctx.recv() */
    (void) addr;
    (void) buf;
    (void) sz;
    (void) atomic;
    return error::DUMMY;
}

error handle::get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz)
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

error handle::get_dev_log(void *buf, size_t& buf_sz)
{
    (void) buf;
    (void) buf_sz;
    return error::DUMMY;
}

error handle::exec_dev(uint32_t addr, uint16_t flags)
{
    (void) addr;
    (void) flags;
    return error::DUMMY;
}

error handle::status() const
{
    return hwstub::handle::status();
}

size_t handle::get_buffer_size()
{
    return 2048;
}

} // namespace gdb
} // namespace net

