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
#ifndef __HWSTUB_GDB_HPP__
#define __HWSTUB_GDB_HPP__

#include "hwstub.hpp"
#include "hwstub_net.hpp"

namespace hwstub {
namespace gdb {


/** GDB context
 *
 * A GDB context connects to a GDB server and has only one device. */
class context : public hwstub::context
{
    friend class device;
    friend class handle;
protected:
    context(int socke_fd);
public:
    virtual ~context();
    /** Create a context from an existing socket */
    static std::shared_ptr<context> create(int socket_fd);
    /** Create a TCP socket context with a domain name and a port. If port is empty,
     * a default port is used. */
    static std::shared_ptr<context> create_tcp(const std::string& domain,
        const std::string& port, std::string *error = nullptr);

protected:
    /** Send a message to the server. Context will always serialize calls to send()
     * so there is no need to worry about concurrency issues. */
    error send(void *buffer, size_t& sz);
    /** Receive a message from the server, sz is updated with the received size.
     * Context will always serialize calls to recv() so there is no need to
     * worry about concurrency issues. */
    error recv(void *buffer, size_t& sz);
    /* NOTE ctx_dev_t is always 0 */
    virtual error fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual error create_device(ctx_dev_t dev, std::shared_ptr<device>& hwdev);
    virtual bool match_device(ctx_dev_t dev, std::shared_ptr<device> hwdev);

    hwstub::net::socket_connection m_conn;
};

/** GDB device
 *
 * Device based on gdb context. */
class device : public hwstub::device
{
    friend class context; /* for ctor */
protected:
    device(std::shared_ptr<hwstub::context> ctx);
public:
    virtual ~device();

protected:
    virtual error open_dev(std::shared_ptr<handle>& handle);
    virtual bool has_multiple_open() const;
};

/** GDB handle
 *
 * Handle based on gdb device. */
class handle : public hwstub::handle
{
    friend class device; /* for ctor */
protected:
    handle(std::shared_ptr<hwstub::device> dev);
public:
    virtual ~handle();

protected:
    virtual error read_dev(uint32_t addr, void *buf, size_t& sz, bool atomic);
    virtual error write_dev(uint32_t addr, const void *buf, size_t& sz, bool atomic);
    virtual error get_dev_desc(uint16_t desc, void *buf, size_t& buf_sz);
    virtual error get_dev_log(void *buf, size_t& buf_sz);
    virtual error exec_dev(uint32_t addr, uint16_t flags);
    virtual error status() const;
    virtual size_t get_buffer_size();

protected:
    struct hwstub_version_desc_t m_desc_version;
    struct hwstub_layout_desc_t m_desc_layout;
    struct hwstub_target_desc_t m_desc_target;
};

} // namespace virt
} // namespace hwstub

#endif /* __HWSTUB_GDB_HPP__ */
 
