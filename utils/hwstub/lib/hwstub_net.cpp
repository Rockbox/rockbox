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
#include <arpa/inet.h>

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
    int create_unix_low(bool abstract, const char *path, size_t len, bool conn,
        std::string *error)
    {
        struct sockaddr_un address;
        if(len > sizeof(address.sun_path))
        {
            if(error)
                *error = "unix path is too long";
            return -1;
        }
        int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if(socket_fd < 0)
        {
            if(error)
                *error = "socket() failed";
            return -1;
        }
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
        if(conn)
        {
            if(connect(socket_fd, (struct sockaddr *)&address,  sz) != 0)
            {
                close(socket_fd);
                if(error)
                    *error = "connect() failed";
                return -1;
            }
            else
                return socket_fd;
        }
        else
        {
            if(bind(socket_fd, (struct sockaddr *)&address,  sz) != 0)
            {
                close(socket_fd);
                if(error)
                    *error = "bind() failed";
                return -1;
            }
            else
                return socket_fd;
        }
    }
}

std::shared_ptr<context> context::create_unix(const std::string& path, std::string *error)
{
    int fd = create_unix_low(false, path.c_str(), path.size() + 1, true, error);
    if(fd >= 0)
        return context::create_socket(fd);
    else
        return std::shared_ptr<context>();
}

std::shared_ptr<context> context::create_unix_abstract(const std::string& path, std::string *error)
{
    std::string fake_path = "#" + path; /* the # will be overriden by 0 */
    int fd = create_unix_low(true, fake_path.c_str(), fake_path.size(), true, error);
    if(fd >= 0)
        return context::create_socket(fd);
    else
        return std::shared_ptr<context>();
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
    (void) ptr;
    delayed_init();
    if(m_state == state::DEAD)
        return m_error;
    uint32_t args[HWSTUB_NET_ARGS] = {0};
    uint8_t *data = nullptr;
    size_t data_sz = 0;
    debug() << "[net::ctx] --> GET_DEV_LIST\n";
    error err = send_cmd(HWSERVER_GET_DEV_LIST, args, nullptr, 0, &data, &data_sz);
    debug() << "[net::ctx] <-- GET_DEV_LIST ";
    if(err != error::SUCCESS)
    {
        debug() << "failed: " << error_string(err) << "\n";
        return err;
    }
    /* sanity check on size */
    if(data_sz % 4)
    {
        debug() << "failed: invalid list size\n";
        delete data;
        return error::PROTOCOL_ERROR;
    }
    debug() << "\n";
    list.clear();
    /* each entry is a 32-bit ID in network order size */
    uint32_t *data_list = (uint32_t *)data;
    for(size_t i = 0; i < data_sz / 4; i++)
        list.push_back(to_ctx_dev(from_net_order(data_list[i])));
    delete data;
    return error::SUCCESS;
}

void context::destroy_device_list(void *ptr)
{
    (void)ptr;
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
    return udev != nullptr && udev->device_id() == from_ctx_dev(dev);
}

uint32_t context::to_net_order(uint32_t u)
{
    return htonl(u);
}

uint32_t context::from_net_order(uint32_t u)
{
    return ntohl(u);
}

error context::send_cmd(uint32_t cmd, uint32_t args[HWSTUB_NET_ARGS], uint8_t *send_data,
    size_t send_size, uint8_t **recv_data, size_t *recv_size)
{
    if(m_state == state::DEAD)
        return m_error;
    /* do a delayed init, unless with are doing a HELLO */
    if(m_state == state::HELLO && cmd != HWSERVER_HELLO)
        delayed_init();
    /* build header */
    struct hwstub_net_hdr_t hdr;
    hdr.magic = to_net_order(HWSERVER_MAGIC);
    hdr.cmd = to_net_order(cmd);
    for(size_t i = 0; i < HWSTUB_NET_ARGS; i++)
        hdr.args[i] = to_net_order(args[i]);
    hdr.length = to_net_order((uint32_t)send_size);
    /* send header */
    size_t sz = sizeof(hdr);
    error err = send((void *)&hdr, sz);
    if(err != error::SUCCESS)
    {
        m_state = state::DEAD;
        m_error = err;
        return err;
    }
    if(sz != sizeof(hdr))
    {
        m_state = state::DEAD;
        m_error = error::PROTOCOL_ERROR;
    }
    /* send data */
    if(send_size > 0)
    {
        sz = send_size;
        err = send((void *)send_data, sz);
        if(err != error::SUCCESS)
        {
            m_state = state::DEAD;
            m_error = err;
            return err;
        }
        if(sz != send_size)
        {
            m_state = state::DEAD;
            m_error = error::PROTOCOL_ERROR;
        }
    }
    /* receive header */
    sz = sizeof(hdr);
    err = recv((void *)&hdr, sz);
    if(err != error::SUCCESS)
    {
        m_state = state::DEAD;
        m_error = err;
        return err;
    }
    if(sz != sizeof(hdr))
    {
        m_state = state::DEAD;
        m_error = error::PROTOCOL_ERROR;
        return m_error;
    }
    /* correct byte order */
    hdr.magic = from_net_order(hdr.magic);
    hdr.cmd = from_net_order(hdr.cmd);
    hdr.length = from_net_order(hdr.length);
    /* copy arguments */
    for(size_t i = 0; i < HWSTUB_NET_ARGS; i++)
        args[i] = from_net_order(hdr.args[i]);
    /* check header */
    if(hdr.magic != HWSERVER_MAGIC)
    {
        m_state = state::DEAD;
        m_error = error::PROTOCOL_ERROR;
        return m_error;
    }
    /* check NACK */
    if(hdr.cmd == HWSERVER_NACK(cmd))
    {
        /* translate error */
        switch(args[0])
        {
            case HWERR_FAIL: err = error::ERROR; break;
            case HWERR_INVALID_ID: err = error::ERROR; break; /* should not happen */
            case HWERR_DISCONNECTED: err = error::DISCONNECTED; break;
        }
        return err;
    }
    /* check not ACK */
    if(hdr.cmd != HWSERVER_ACK(cmd))
    {
        m_state = state::DEAD;
        m_error = error::PROTOCOL_ERROR;
        return m_error;
    }
    /* receive additional data
     * FIXME should have a limit on the size */
    uint8_t *data = nullptr;
    if(hdr.length > 0)
    {
        new uint8_t[hdr.length];
        sz = hdr.length;
        err = recv((void *)data, sz);
        if(err != error::SUCCESS)
        {
            m_state = state::DEAD;
            m_error = err;
            return err;
        }
        if(sz != hdr.length)
        {
            m_state = state::DEAD;
            m_error = error::PROTOCOL_ERROR;
            return m_error;
        }
    }
    /* copy data if user want it */
    if(recv_data)
    {
        *recv_data = data;
        *recv_size = hdr.length;
    }
    /* throw it away otherwise */
    else
    {
        delete data;
    }
    return error::SUCCESS;
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
        debug() << "[net::ctx] <-- HELLO failed: " << error_string(err) << "\n";
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

void context::stop_context()
{
    /* make sure with have the lock, this function might be call asynchronously */
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* if dead, don't do anything */
    if(m_state == state::DEAD)
        return;
    /* only send BYE if we are initialized */
    if(m_state == state::IDLE)
    {
        debug() << "[net::ctx] --> BYE\n";
        /* send BYE */
        uint32_t args[HWSTUB_NET_ARGS] = {0};
        error err = send_cmd(HWSERVER_BYE, args, nullptr, 0, nullptr, nullptr);
        if(err != error::SUCCESS)
        {
            debug() << "[net::ctx] <-- BYE failed: " << error_string(err) << "\n";
            m_state = state::DEAD;
            m_error = err;
            return;
        }
        debug() << "[net::ctx] <-- BYE\n";
    }
    /* now we are dead */
    m_state = state::DEAD;
    m_error = error::SERVER_DISCONNECTED;
}

/**
 * Socket context
 */
socket_context::socket_context(int socket_fd)
    :m_socketfd(socket_fd)
{
    set_timeout(std::chrono::milliseconds(1000));
}

socket_context::~socket_context()
{
    stop_context();
    close(m_socketfd);
}

void socket_context::set_timeout(std::chrono::milliseconds ms)
{
    struct timeval tv;
    tv.tv_usec = 1000 * (ms.count() % 1000);
    tv.tv_sec = ms.count() / 1000;
     /* set timeout for the client operations */
    setsockopt(m_socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(m_socketfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
}

error socket_context::send(void *buffer, size_t& sz)
{
    debug() << "[net::ctx::sock] send(" << sz << "): ";
    int ret = ::send(m_socketfd, buffer, sz, MSG_NOSIGNAL);
    if(ret >= 0)
    {
        debug() << "good(" << ret << ")\n";
        sz = (size_t)ret;
        return error::SUCCESS;
    }
    /* convert some errors */
    debug() << "fail(" << errno << ")\n";
    switch(errno)
    {
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
#endif
        case EWOULDBLOCK: return error::TIMEOUT;
        case ECONNRESET: case EPIPE: return error::SERVER_DISCONNECTED;
        default: return error::NET_ERROR;
    }
}

error socket_context::recv(void *buffer, size_t& sz)
{
    debug() << "[net::ctx::sock] recv(" << sz << "): ";
    int ret = ::recv(m_socketfd, buffer, sz, MSG_WAITALL);
    if(ret > 0)
    {
        debug() << "good(" << ret << ")\n";
        sz = (size_t)ret;
        return error::SUCCESS;
    }
    if(ret == 0)
    {
        debug() << "disconnected\n";
        return error::SERVER_DISCONNECTED;
    }
    debug() << "fail(" << errno << ")\n";
    switch(errno)
    {
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
#endif
        case EWOULDBLOCK: return error::TIMEOUT;
        default: return error::NET_ERROR;
    }
}

/**
 * Device
 */
device::device(std::shared_ptr<hwstub::context> ctx, uint32_t devid)
    :hwstub::device(ctx), m_device_id(devid)
{
}

device::~device()
{
}

uint32_t device::device_id()
{
    return m_device_id;
}

error device::open_dev(std::shared_ptr<hwstub::handle>& handle)
{
    (void) handle;
    return error::ERROR;
}

bool device::has_multiple_open() const
{
    return false;
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
}

void server::stop_server()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    /* ask all client threads to stop */
    for(auto& cl : m_client)
        cl.exit = true;
    /* wait for each thread to stop */
    for(auto& cl : m_client)
        cl.future.wait();
}

std::shared_ptr<server> server::create_unix(std::shared_ptr<hwstub::context> ctx,
    const std::string& path, std::string *error)
{
    int fd = create_unix_low(false, path.c_str(), path.size() + 1, false, error);
    if(fd >= 0)
        return socket_server::create_socket(ctx, fd);
    else
        return std::shared_ptr<server>();
}

std::shared_ptr<server> server::create_unix_abstract(std::shared_ptr<hwstub::context> ctx,
    const std::string& path, std::string *error)
{
    std::string fake_path = "#" + path; /* the # will be overriden by 0 */
    int fd = create_unix_low(true, fake_path.c_str(), fake_path.size(), false, error);
    if(fd >= 0)
        return socket_server::create_socket(ctx, fd);
    else
        return std::shared_ptr<server>();
}

std::shared_ptr<server> server::create_socket(std::shared_ptr<hwstub::context> ctx,
    int socket_fd)
{
    return socket_server::create(ctx, socket_fd);
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

uint32_t server::to_net_order(uint32_t u)
{
    return htonl(u);
}

uint32_t server::from_net_order(uint32_t u)
{
    return ntohl(u);
}

void server::client_thread(client_state *state)
{
    debug() << "[net::srv::client] start: " << state->client << "\n";
    while(!state->exit)
    {
        /* wait for some header */
        struct hwstub_net_hdr_t hdr;
        size_t sz = sizeof(hdr);
        error err;
        /* wait for some command, or exit flag */
        do
            err = recv(state->client, (void *)&hdr, sz);
        while(err == error::TIMEOUT && !state->exit);
        if(state->exit || err != error::SUCCESS || sz != sizeof(hdr))
            break;
        /* convert to host order */
        hdr.magic = from_net_order(hdr.magic);
        hdr.cmd = from_net_order(hdr.cmd);
        hdr.length = from_net_order(hdr.length);
        /* copy arguments */
        for(size_t i = 0; i < HWSTUB_NET_ARGS; i++)
            hdr.args[i] = from_net_order(hdr.args[i]);
        /* check header */
        if(hdr.magic != HWSERVER_MAGIC)
            break;
        /* receive data
         * FIXME check length here */
        uint8_t *data = nullptr;
        if(hdr.length > 0)
        {
            data = new uint8_t[hdr.length];
            sz = hdr.length;
            /* wait for some command, or exit flag */
            do
                err = recv(state->client, (void *)data, sz);
            while(err == error::TIMEOUT && !state->exit);
            if(state->exit || err != error::SUCCESS || sz != hdr.length)
            {
                delete data;
                break;
            }
        }
        /* hande command */
        uint8_t *send_data = nullptr;
        size_t send_size = 0;
        err = handle_cmd(hdr.cmd, hdr.args, data, hdr.length, send_data, send_size);
        /* free data */
        delete data;
        /* construct header */
        if(err != error::SUCCESS)
        {
            hdr.magic = to_net_order(HWSERVER_MAGIC);
            hdr.cmd = to_net_order(HWSERVER_NACK(hdr.cmd));
            hdr.length = to_net_order(0);
            hdr.args[0] = to_net_order(HWERR_FAIL);
            for(size_t i = 1; i < HWSTUB_NET_ARGS; i++)
                hdr.args[i] = to_net_order(0);
            send_size = 0;
        }
        else
        {
            hdr.magic = to_net_order(HWSERVER_MAGIC);
            hdr.cmd = to_net_order(HWSERVER_ACK(hdr.cmd));
            hdr.length = to_net_order(send_size);
            for(size_t i = 0; i < HWSTUB_NET_ARGS; i++)
                hdr.args[i] = to_net_order(hdr.args[i]);
        }
        /* send header */
        do
            err = send(state->client, (void *)&hdr, sz);
        while(err == error::TIMEOUT && !state->exit);
        if(state->exit || err != error::SUCCESS || sz != sizeof(hdr))
        {
            delete send_data;
            break;
        }
        /* send data if there is some */
        if(send_size > 0)
        {
            sz = send_size;
            do
                err = send(state->client, (void *)send_data, sz);
            while(err == error::TIMEOUT && !state->exit);
            delete send_data;
            if(state->exit || err != error::SUCCESS || sz != send_size)
                break;
        }
    }
    debug() << "[net::srv::client] stop: " << state->client << "\n";
    terminate_client(state->client);
}

void server::client_arrived(srv_client_t client)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    debug() << "[net::srv] client arrived: " << client << "\n";
    /* the naive way would be to use a std::thread but this class is annoying
     * because it is impossible to check if a thread has exited, except by calling
     * join() which is blocking. Fortunately, std::packaged_task and std::future
     * provide a way around this */

    std::packaged_task<void(server*, client_state*)> task(&server::client_thread2);
    m_client.emplace_back(client, task.get_future());
    std::thread(std::move(task), this, &m_client.back()).detach();
}

void server::client_left(srv_client_t client)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    debug() << "[net::srv] client left: " << client << "\n";
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

error server::handle_cmd(uint32_t cmd, uint32_t args[HWSTUB_NET_ARGS],
    uint8_t *recv_data, size_t recv_size, uint8_t*& send_data, size_t& send_size)
{
    send_data = nullptr;
    send_size = 0;
    (void) recv_data;
    (void) recv_size;

    if(cmd == HWSERVER_HELLO)
    {
        debug() << "[net::srv::cmd] --> HELLO " << ((args[0] & 0xff00) >> 8)
            << "." << (args[0] & 0xff);
        if(args[0] != (HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR))
        {
            debug() << " (mismatch)\n";
            return error::ERROR;
        }
        debug() << " (good)\n";
        debug() << "[net::srv::cmd] <-- HELLO " << HWSTUB_VERSION_MAJOR << "."
            << HWSTUB_VERSION_MINOR << "\n";
        /* send HELLO with our version */
        args[0] = HWSTUB_VERSION_MAJOR << 8 | HWSTUB_VERSION_MINOR;
        return error::SUCCESS;
    }
    else if(cmd == HWSERVER_BYE)
    {
        debug() << "[net::srv::cmd] --> BYE\n";
        debug() << "[net::srv::cmd] <-- BYE\n";
        return error::SUCCESS;
    }
    else
        return error::ERROR;
}

/*
 * Socket server
 */
socket_server::socket_server(std::shared_ptr<hwstub::context> contex, int socket_fd)
    :server(contex), m_socketfd(socket_fd)
{
    m_discovery_exit = false;
    set_timeout(std::chrono::milliseconds(1000));
    m_discovery_thread = std::thread(&socket_server::discovery_thread1, this);
}

socket_server::~socket_server()
{
    /* first stop discovery thread to make sure no more clients are created */
    m_discovery_exit = true;
    m_discovery_thread.join();
    close(m_socketfd);
    /* ask server to do a clean stop */
    stop_server();
}

std::shared_ptr<server> socket_server::create(std::shared_ptr<hwstub::context> ctx,
    int socket_fd)
{
    // NOTE: can't use make_shared() because of the protected ctor */
    return std::shared_ptr<socket_server>(new socket_server(ctx, socket_fd));
}

void socket_server::set_timeout(std::chrono::milliseconds ms)
{
    m_timeout.tv_usec = 1000 * (ms.count() % 1000);
    m_timeout.tv_sec = ms.count() / 1000;
}

int socket_server::from_srv_client(srv_client_t cli)
{
    return (int)(intptr_t)cli;
}

socket_server::srv_client_t socket_server::to_srv_client(int fd)
{
    return (srv_client_t)(intptr_t)fd;
}

void socket_server::discovery_thread1(socket_server *s)
{
    s->discovery_thread();
}

void socket_server::terminate_client(srv_client_t client)
{
    debug() << "[net::srv::sock] terminate client: " << client << "\n";
    /* simply close connection */
    close(from_srv_client(client));
}

error socket_server::send(srv_client_t client, void *buffer, size_t& sz)
{
    debug() << "[net::ctx::sock] send(" << client << ", " << sz << "): ";
    int ret = ::send(from_srv_client(client), buffer, sz, MSG_NOSIGNAL);
    if(ret >= 0)
    {
        debug() << "good(" << ret << ")\n";
        sz = (size_t)ret;
        return error::SUCCESS;
    }
    /* convert some errors */
    debug() << "fail(" << errno << ")\n";
    switch(errno)
    {
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
#endif
        case EWOULDBLOCK: return error::TIMEOUT;
        case ECONNRESET: case EPIPE: return error::SERVER_DISCONNECTED;
        default: return error::NET_ERROR;
    }
}

error socket_server::recv(srv_client_t client, void *buffer, size_t& sz)
{
    debug() << "[net::ctx::sock] recv(" << client << ", " << sz << "): ";
    int ret = ::recv(from_srv_client(client), buffer, sz, MSG_WAITALL);
    if(ret > 0)
    {
        debug() << "good(" << ret << ")\n";
        sz = (size_t)ret;
        return error::SUCCESS;
    }
    if(ret == 0)
    {
        debug() << "disconnected\n";
        return error::SERVER_DISCONNECTED;
    }
    debug() << "fail(" << errno << ")\n";
    switch(errno)
    {
#if EAGAIN != EWOULDBLOCK
        case EAGAIN:
#endif
        case EWOULDBLOCK: return error::TIMEOUT;
        default: return error::NET_ERROR;
    }
}

void socket_server::discovery_thread()
{
    debug() << "[net::srv:sock::discovery] start\n";
    /* begin listening to incoming connections */
    if(listen(m_socketfd, LISTEN_QUEUE_SIZE) < 0)
    {
        debug() << "[net::srv::sock::discovery] listen() failed: " << errno << "\n";
        return;
    }

    /* handle connections */
    while(!m_discovery_exit)
    {
        /* since accept() is blocking, use select to ensure a timeout */
        struct timeval tmo = m_timeout; /* NOTE select() can overwrite timeout */
        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_socketfd, &set);
        /* wait for some activity */
        int ret = select(m_socketfd + 1, &set, nullptr, nullptr, &tmo);
        if(ret < 0 || !FD_ISSET(m_socketfd, &set))
            continue;
        int clifd = accept(m_socketfd, nullptr, nullptr);
        if(clifd >= 0)
        {
            debug() << "[net::srv::sock::discovery] new client\n";
            /* set timeout for the client operations */
            setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, (char *)&m_timeout, sizeof(m_timeout));
            setsockopt(clifd, SOL_SOCKET, SO_SNDTIMEO, (char *)&m_timeout, sizeof(m_timeout));
            client_arrived(to_srv_client(clifd));
        }
    }
    debug() << "[net::srv:sock::discovery] stop\n";
}

} // namespace uri
} // namespace net
