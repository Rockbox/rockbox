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
#include "hwstub_net.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstddef>

namespace hwstub {
namespace net {

/**
 * Context
 */
context::context()
    :m_state(state::HELLO), m_error(error::SUCCESS)
{
}

context::~context()
{
}

std::shared_ptr<context> context::create_socket(int socket_fd)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<socket_context>(new socket_context(socket_fd));
}

std::shared_ptr<context> context::create_tcp(const std::string& domain,
    const std::string& port)
{
    (void) domain;
    (void) port;
    return std::shared_ptr<context>();
}

namespace
{
    /* len is the total length, including a 0 character if any */
    std::shared_ptr<context> create_unix_low(bool abstract, const char *path, size_t len)
    {
        struct sockaddr_un address;
        if(len > sizeof(address.sun_path))
            return std::shared_ptr<context>();
        int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if(socket_fd < 0)
            return std::shared_ptr<context>();
        memset(&address, 0, sizeof(struct sockaddr_un));
        address.sun_family = AF_UNIX;
        /* NOTE memcpy, we don't want to add a extra 0 at the end */
        memcpy(address.sun_path, path, len);
        /* for abstract name, replace first character by 0 */
        if(abstract)
            address.sun_path[0] = 0;
        /* NOTE sun_path is the last field of the structure */
        size_t sz = offsetof(struct sockaddr_un, sun_path) + len;
        /* NOTE don't give sizeof(address) because for abstract names it would contain
         * extra garbage */
        if(connect(socket_fd, (struct sockaddr *)&address,  sz) != 0)
        {
            close(socket_fd);
            return std::shared_ptr<context>();
        }
        return context::create_socket(socket_fd);
    }
}

std::shared_ptr<context> context::create_unix(const std::string& path)
{
    return create_unix_low(false, path.c_str(), path.size() + 1);
}

std::shared_ptr<context> context::create_unix_abstract(const std::string& path)
{
    std::string fake_path = "#" + path; /* the # will be overriden by 0 */
    return create_unix_low(true, fake_path.c_str(), fake_path.size());
}

uint32_t context::from_ctx_dev(ctx_dev_t dev)
{
    return (uint32_t)(uintptr_t)dev; /* NOTE safe because it was originally a 32-bit int */
}

hwstub::context::ctx_dev_t context::to_ctx_dev(uint32_t dev)
{
    return (ctx_dev_t)(uintptr_t)dev; /* NOTE assume that sizeof(void *)>=sizeof(uint32_t) */
}

error context::fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr)
{
    (void) list;
    (void) ptr;
    delayed_init();
    if(m_state == state::DEAD)
        return m_error;

    (void)ptr;
    list.clear();
    return error::SUCCESS;
}

void context::destroy_device_list(void *ptr)
{
    (void)ptr;
}

error context::create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev)
{
    (void) dev;
    (void) hwdev;
    return error::ERROR;
}

bool context::match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev)
{
    (void) dev;
    (void) hwdev;
    return false;
}

error context::send_cmd(uint32_t cmd, uint32_t args[HWSTUB_NET_ARGS], uint8_t *send_data,
    size_t send_size, uint8_t **recv_data, size_t *recv_size)
{
    (void) cmd;
    (void) args;
    (void) send_data;
    (void) send_size;
    (void) recv_data;
    (void) recv_size;
    return error::ERROR;
}

void context::delayed_init()
{
    /* only do HELLO if we haven't do it yet */
    if(m_state != state::HELLO)
        return;
    debug() << "[net::ctx] --> HELLO " << HWSTUB_VERSION_MAJOR << "."
        << HWSTUB_VERSION_MINOR << "\n";
    /* send HELLO with our version and see what the server is up to */
    uint32_t args[HWSTUB_NET_ARGS] = {0};
    args[0] = HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR;
    error err = send_cmd(HWSERVER_HELLO, args, nullptr, 0, nullptr, nullptr);
    if(err != error::SUCCESS)
    {
        debug() << "[net::ctx] HELLO failed: " << error_string(err) << "\n";
        m_state = state::DEAD;
        m_error = err;
        return;
    }
    /* check the server is running the same version */
    debug() << "[net::ctx] <-- HELLO " << std::hex << ((args[0] & 0xff00) >> 8) << "." << (args[0] & 0xff) << "";
    if(args[0] != (HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR))
    {
        debug() << " (mismatch)\n";
        m_state = state::DEAD;
        m_error = error::SERVER_MISMATCH;
    }
    debug() << " (good)\n";
    /* good, we can now send commands */
    m_state = state::IDLE;
}

/**
 * Socket context
 */
socket_context::socket_context(int socket_fd)
    :m_socketfd(socket_fd)
{
}

socket_context::~socket_context()
{
    close(m_socketfd);
}

error socket_context::send(void *buffer, size_t sz)
{
    (void) buffer;
    (void) sz;
    return error::NET_ERROR;
}

error socket_context::recv(void *buffer, size_t& sz)
{
    (void) buffer;
    (void) sz;
    return error::NET_ERROR;
}

/**
 * Server
 */
server::server(std::shared_ptr<hwstub::context> ctx)
    :m_context(ctx)
{
    clear_debug();
}

server::~server()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* ask all client threads to stop */
    for(auto& cl : m_client)
        cl.exit = true;
    /* wait for each thread to stop */
    for(auto& cl : m_client)
        cl.future.wait();
}

void server::set_debug(std::ostream& os)
{
    m_debug = &os;
}

std::ostream& server::debug()
{
    return *m_debug;
}

server::client_state::client_state(srv_client_t cl, std::future<void>&& f)
    :client(cl), future(std::move(f)), exit(false)
{
}

void server::client_thread2(server *s, client_state *cs)
{
    s->client_thread(cs);
}

void server::client_thread(client_state *state)
{
    while(!state->exit)
    {
        /* do stuff */
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

error server::client_arrived(srv_client_t client)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* the naive way would be to use a std::thread but this class is annoying
     * because it is impossible to check if a thread has exited, except by calling
     * join() which is blocking. Fortunately, std::packaged_task and std::future
     * provide a way around this */

    std::packaged_task<void(server*, client_state*)> task(&server::client_thread2);
    m_client.emplace_back(client, task.get_future());
    std::thread(std::move(task), this, &m_client.back()).detach();
    return error::SUCCESS;
}

void server::client_left(srv_client_t client)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* find thread and set its exit flag, also cleanup threads that finished */
    for(auto it = m_client.begin(); it != m_client.end();)
    {
        /* check if thread has finished */
        if(it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            it = m_client.erase(it);
            continue;
        }
        /* set exit flag if this our thread */
        if(it->client == client)
            it->exit = true;
        ++it;
    }
}

} // namespace uri
} // namespace net
