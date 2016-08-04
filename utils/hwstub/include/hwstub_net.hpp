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
#ifndef __HWSTUB_NET_HPP__
#define __HWSTUB_NET_HPP__

#include "hwstub.hpp"
#include <map>
#include <thread>
#include <future>
#include <list>

namespace hwstub {
namespace net {

/** Net context
 *
 * A socket context provides access to another context through a network. This
 * is particularly useful to have another program create a USB context and provide
 * access to it via some network. The two most useful types of network are TCP
 * and Unix domains */
class context : public hwstub::context
{
    friend class device;
    friend class handle;
protected:
    context();
public:
    virtual ~context();
    /** Create a socket context with an existing file descriptor. Note that the
     * file descriptor will be closed when the context will be destroyed. */
    static std::shared_ptr<context> create_socket(int socket_fd);
    /** Create a TCP socket context with a domain name and a port. If port is empty,
     * a default port is used. */
    static std::shared_ptr<context> create_tcp(const std::string& domain,
        const std::string& port, std::string *error = nullptr);
    /** Create a UNIX socket context with a file system path (see man for details) */
    static std::shared_ptr<context> create_unix(const std::string& path,
        std::string *error = nullptr);
    /** Create a UNIX socket context with an abstract name (see man for details) */
    static std::shared_ptr<context> create_unix_abstract(const std::string& path,
        std::string *error = nullptr);
    /** Useful functions for network byte order conversion */
    uint32_t to_net_order(uint32_t u);
    uint32_t from_net_order(uint32_t u);

    /** Default parameters */
    static std::string default_unix_path();
    static std::string default_tcp_domain();
    static std::string default_tcp_port();

protected:
    /** Send a message to the server. Context will always serialize calls to send()
     * so there is no need to worry about concurrency issues. */
    virtual error send(void *buffer, size_t& sz) = 0;
    /** Receive a message from the server, sz is updated with the received size.
     * Context will always serialize calls to recv() so there is no need to
     * worry about concurrency issues. */
    virtual error recv(void *buffer, size_t& sz) = 0;
    /** Perform a standard command: send a header with optional data and wait for
     * an answer. In case of an underlying network error, the corresponding error
     * code will be reported. If the server responds correctly, the argument array
     * is overwritten with the servers's response. If the requests has been NACK'ed
     * the error code will be parsed and returned as a standard error code (see details below)
     * (note that the original error code can still be found in args[0]). No data
     * is transmitted in case of NACK.
     * If the server ACKs the request, this function will also perform reception of
     * the data. In recv_data is not NULL, the receive data will be put there and the
     * size will be written in in_size. There are two cases: either *recv_data is NULL
     * and the function will allocate the memory based on much data is sent by the server.
     * Or *recv_data is not NULL, in which case the function NOT allocate memory
     * and put the data at *recv_data; in this case, *recv_size should be the set
     * to the size of the buffer and will be updated to the received size. If the
     * server sents more data than the buffer size, OVERFLOW will be returned.
     * If no data was received but recv_data is not null, *recv_size will be set to
     * zero. It is the caller's responsability to delete *recv_data. Note that if
     * server sends data but recv_data is null, the data will still be received and
     * thrown away.
     * This function takes care of network byte order for cmd and arguments
     * but not for data. */
    error send_cmd(uint32_t cmd, uint32_t args[HWSTUB_NET_ARGS], uint8_t *send_data,
        size_t send_size, uint8_t **recv_data, size_t *recv_size);
    /** Ask the context to stop any communication with the server and do a clean
     * shutdown if possible. This is a blocking call. When this function returns,
     * there will no more calls to the underlying communication functions.
     * This function should be called in the destructor to prevent the context from
     * calling children functions after the object has been deconstructed. */
    void stop_context();

    /** Perform delayed init (aka HELLO stage), do nothing is not needed */
    void delayed_init();
    /* NOTE ctx_dev_t = uint32_t (device id) */
    uint32_t from_ctx_dev(ctx_dev_t dev);
    ctx_dev_t to_ctx_dev(uint32_t dev);
    virtual error fetch_device_list(std::vector<ctx_dev_t>& list, void*& ptr);
    virtual void destroy_device_list(void *ptr);
    virtual error create_device(ctx_dev_t dev, std::shared_ptr<hwstub::device>& hwdev);
    virtual bool match_device(ctx_dev_t dev, std::shared_ptr<hwstub::device> hwdev);

    enum class state
    {
        HELLO, /* client is initialising, server has not been contacted yet */
        IDLE, /* not doing anything */
        DEAD, /* died on unrecoverable error */
    };

    state m_state; /* client state */
    error m_error; /* error state for DEAD */
};

/** Socket based net context
 *
 * Don't use this class directly, use context::create_* calls. This class
 * provides send()/recv() for any socket based network. */
class socket_context : public context
{
    friend class context;
protected:
    socket_context(int socket_fd);
public:
    virtual ~socket_context();
    /** set operation timeout */
    void set_timeout(std::chrono::milliseconds ms);

protected:
    virtual error send(void *buffer, size_t& sz);
    virtual error recv(void *buffer, size_t& sz);

    int m_socketfd; /* socket file descriptor */
};


/** Net device
 *
 * Device accessed through a network */
class device : public hwstub::device
{
    friend class context; /* for ctor */
protected:
    device(std::shared_ptr<hwstub::context> ctx, uint32_t devid);
public:
    virtual ~device();

protected:
    /** Return device ID */
    uint32_t device_id();
    virtual error open_dev(std::shared_ptr<hwstub::handle>& handle);
    virtual bool has_multiple_open() const;

    int32_t m_device_id; /* device id */
};

/** Net handle
 *
 * Handle used to talk to a distant device. */
class handle : public hwstub::handle
{
    friend class device;
protected:
    handle(std::shared_ptr<hwstub::device> dev, uint32_t hid);
public:
    virtual ~handle();

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

    uint32_t m_handle_id; /* handle id */
};

/** Net server
 *
 * A server that forwards requests from net clients to a context */
class server
{
protected:
    server(std::shared_ptr<hwstub::context> contex);
public:
    virtual ~server();

    /** Create a socket server with an existing file descriptor. Note that the
     * file descriptor will be closed when the context will be destroyed. */
    static std::shared_ptr<server> create_socket(std::shared_ptr<hwstub::context> contex,
        int socket_fd);
    /** Create a TCP socket server with a domain name and a port. If port is empty,
     * a default port is used. */
    static std::shared_ptr<server> create_tcp(std::shared_ptr<hwstub::context> contex,
        const std::string& domain, const std::string& port, std::string *error = nullptr);
    /** Create a UNIX socket server with a file system path (see man for details) */
    static std::shared_ptr<server> create_unix(std::shared_ptr<hwstub::context> contex,
        const std::string& path, std::string *error = nullptr);
    /** Create a UNIX socket server with an abstract name (see man for details) */
    static std::shared_ptr<server> create_unix_abstract(
        std::shared_ptr<hwstub::context> contex, const std::string& path,
        std::string *error = nullptr);
    /** Useful functions for network byte order conversion */
    uint32_t to_net_order(uint32_t u);
    uint32_t from_net_order(uint32_t u);

    /** Set/clear debug output for this context */
    void set_debug(std::ostream& os);
    inline void clear_debug() { set_debug(cnull); }
    /** Get debug output for this context */
    std::ostream& debug();
protected:
    struct client_state;
    /** Opaque client type */
    typedef void* srv_client_t;
    /** The client discovery implementation must call this function when a new
     * client wants to talk to the server. If the server is unhappy with the
     * request, it will immediately call terminate_client() */
    void client_arrived(srv_client_t client);
    /** The client discovery implementation can notify asychronously about a client
     * that left. Note that the implementation does not need to provide a mechanism,
     * but should in this case return CLIENT_DISCONNECTED when the server performs
     * a send() or recv() on a disconnected client. The server will always call
     * after receiving client_left() but since this call is asychronous, the
     * implementation must be prepared to deal with extra send()/recv() in the mean
     * time. */
    void client_left(srv_client_t client);
    /** The client discovery implementation can ask the server to stop all client
     * threads. This is a blocking call. When this function returns, there will no
     * more calls to the underlying communication functions. Note that the server
     * will normally call terminate_client() on each active client at this point.
     * This function should be called in the destructor to prevent the server from
     * calling children functions after the object has been deconstructed. */
    void stop_server();
    /** Notify that the connection to a client is now finished. After this call, no
     * more send()/recv() will be made to the client and the associated data will
     * be freed. After this call, the implementation is not allowed to call client_left()
     * for this client (assuming it did not previously). The implementation should close
     * the communication channel at this point and free any associated data. */
    virtual void terminate_client(srv_client_t client) = 0;
    /** Send a message to the client. Server will always serialize calls to send()
     * for a given client so there is no need to worry about concurrency issues. */
    virtual error send(srv_client_t client, void *buffer, size_t& sz) = 0;
    /** Receive a message from the client, sz is updated with the received size.
     * Server will always serialize calls to recv() for a given client so there
     * is no need to worry about concurrency issues. See comment about client_left(). */
    virtual error recv(srv_client_t client, void *buffer, size_t& sz) = 0;
    /** handle command: cmd and arguments are in host order, the function should
     * either return an error (command will be NACKed) or must fill the arguments
     * and data for the answer. Note that the data is still in network byte order.
     * If the funtion wants to send data back, it must set *send_data to a valid
     * pointer, this pointer will be freed after the data is sent back. */
    error handle_cmd(client_state *state, uint32_t cmd, uint32_t args[HWSTUB_NET_ARGS],
        uint8_t *recv_data, size_t recv_size, uint8_t*& send_data, size_t& send_size);

    /* complete state of a client */
    struct client_state
    {
        client_state(srv_client_t cl, std::future<void>&& f);
        srv_client_t client; /* client */
        std::future<void> future; /* thread (see .cpp for explaination) */
        volatile bool exit; /* exit flag */
        uint32_t next_dev_id; /* next device ID */
        uint32_t next_handle_id; /* next handle ID */
        /* dev ID <-> hwstub dev map */
        std::map<uint32_t, std::shared_ptr<hwstub::device>> dev_map;
        /* handle ID -> hwstub handle map */
        std::map<uint32_t, std::shared_ptr<hwstub::handle>> handle_map;
    };

    /** Client thread */
    static void client_thread2(server *s, client_state *cs);
    void client_thread(client_state *cs);

    std::shared_ptr<hwstub::context> m_context; /* context to perform operation */
    std::list<client_state> m_client; /* client list */
    std::recursive_mutex m_mutex; /* server mutex */
    std::ostream *m_debug; /* debug stream */
};

/** Socket based net server
 *
 */
class socket_server : public server
{
protected:
    socket_server(std::shared_ptr<hwstub::context> contex, int socket_fd);
public:
    virtual ~socket_server();
    /** create a server */
    static std::shared_ptr<server> create(std::shared_ptr<hwstub::context> contex,
        int socket_fd);
    /** set operation timeout */
    void set_timeout(std::chrono::milliseconds ms);

protected:
    virtual void terminate_client(srv_client_t client);
    virtual error send(srv_client_t client, void *buffer, size_t& sz);
    virtual error recv(srv_client_t, void *buffer, size_t& sz);

    /* NOTE srv_client_t = int (client file descriptor) */
    int from_srv_client(srv_client_t cli);
    srv_client_t to_srv_client(int fd);

    /** Discovery thread */
    static void discovery_thread1(socket_server *s);
    void discovery_thread();

    static const int LISTEN_QUEUE_SIZE = 5;
    struct timeval m_timeout; /* operations timeout */
    int m_socketfd; /* socket file descriptor */
    std::thread m_discovery_thread; /* thread handling client discovery */
    volatile bool m_discovery_exit; /* exit flag */
};

} // namespace net
} // namespace hwstub

#endif /* __HWSTUB_NET_HPP__ */
