/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Marcin Bukat
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
#include "hwstub_internal.h"

struct hwstub_tcp_device_t
{
    struct hwstub_device_t dev;
    int handle;
    int32_t ref;
};

#define TCP_DEV(to, from) struct hwstub_tcp_device_t *to = (void *)from; 

static void hwstub_tcp_release(struct hwstub_device_t *_dev)
{
    TCP_DEV(dev, _dev)
    close(dev->handle);
    free(dev);
}

static int hwstub_tcp_get_desc(struct hwstub_device_t *_dev, uint16_t desc, void *info, size_t sz)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = HWSTUB_GET_DESC,
        .addr = desc,
        .len = (uint32_t)sz
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSTUB_GET_DESC_ACK && p.len > 0)
    {
        n = recv(dev->handle, info, p.len, MSG_WAITALL);
        return (n == (ssize_t)p.len) ? n : 0;
    }
    else
        return -1;
}

static int hwstub_tcp_get_log(struct hwstub_device_t *_dev, void *buf, size_t sz)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = HWSTUB_GET_LOG,
        .addr = 0,
        .len = (uint32_t)sz
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSTUB_GET_LOG_ACK && p.len > 0)
    {
        n = recv(dev->handle, buf, p.len, MSG_WAITALL);
        return (n == (ssize_t)p.len) ? n : 0;
    }
    else
        return -1;
}

static int hwstub_tcp_read(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    void *buf, size_t sz)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = (uint32_t)breq,
        .addr = addr,
        .len = (uint32_t)sz
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == (HWSTUB_ACK(breq)) && p.len > 0)
    {
        n = read(dev->handle, buf, p.len);
        return n;
    }
    else
        return -1;
}

static int hwstub_tcp_write(struct hwstub_device_t *_dev, uint8_t breq, uint32_t addr,
    const void *buf, size_t sz)
{
    /* FIXME bug */
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = (uint32_t)breq,
        .addr = addr,
        .len = (uint32_t)sz
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* write data */
    n = send(dev->handle, buf, sz, MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && (p.cmd & HWSTUB_ACK_MASK))
        return n;
    else
        return -1;
}

static int hwstub_tcp_exec(struct hwstub_device_t *_dev, uint32_t addr, uint16_t flags)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = HWSTUB_EXEC,
        .addr = addr,
        .len = flags
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSTUB_EXEC_ACK)
        return 0;
    else
        return -1;
}

static struct hwstub_device_vtable_t hwstub_tcp_vtable =
{
    .release = hwstub_tcp_release,
    .get_log = hwstub_tcp_get_log,
    .get_desc = hwstub_tcp_get_desc,
    .read = hwstub_tcp_read,
    .write = hwstub_tcp_write,
    .exec = hwstub_tcp_exec,
};

struct hwstub_device_t *hwstub_tcp_open(const char *host, const char *port)
{
    struct hwstub_tcp_device_t *dev = malloc(sizeof(struct hwstub_tcp_device_t));
    if (dev == NULL)
        goto Lerr;

    memset(dev, 0, sizeof(struct hwstub_tcp_device_t));
    dev->dev.vtable = hwstub_tcp_vtable;
    dev->handle = -1;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    struct addrinfo *result;
    int s = getaddrinfo(host, port, &hints, &result);
    if(s != 0)
        goto Lerr;

    /* getaddrinfo() returns a list of address structures.
     * Try each address until we successfully connect(2).
     * If socket(2) (or connect(2)) fails, we (close the socket
     * and) try the next address. */
    for(struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
    {
        dev->handle = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(dev->handle == -1)
            continue;

        if(connect(dev->handle, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */
        else
            close(dev->handle);
    }
    freeaddrinfo(result);
    if(dev->handle == -1)
        goto Lerr;

    struct timeval tv;
    tv.tv_sec = 30;  /* 30 secs timeout */
    tv.tv_usec = 0;  /* init this field to avoid strange errors */
    setsockopt(dev->handle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
    return (struct hwstub_device_t *)dev;

Lerr:
    if (dev) { free(dev); }
    return NULL;
}

int hwserver_get_dev_list(struct hwstub_device_t *_dev, void *buf, size_t sz)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = dev->ref,
        .cmd = HWSERVER_GET_DEV_LIST,
        .addr = 0,
        .len = (uint32_t)sz
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSERVER_GET_DEV_LIST_ACK && p.len > 0)
    {
        n = recv(dev->handle, buf, p.len, MSG_WAITALL);
        return (n == (ssize_t)p.len) ? n : 0;
    }
    else
        return -1;
}


int hwserver_dev_open(struct hwstub_device_t *_dev, int32_t _ref)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = _ref,
        .cmd = HWSERVER_DEV_OPEN,
        .addr = 0,
        .len = 0
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSERVER_DEV_OPEN_ACK)
    {
        dev->ref = _ref;
        return 0;
    }
    else
        return -1;
}

int hwserver_dev_close(struct hwstub_device_t *_dev, int32_t _ref)
{
    TCP_DEV(dev, _dev)
    struct hwstub_tcp_cmd_t p =
    {
        .ref = _ref,
        .cmd = HWSERVER_DEV_CLOSE,
        .addr = 0,
        .len = 0
    };

    /* write command */
    int n = send(dev->handle, &p, sizeof(p), MSG_NOSIGNAL);
    if(n < 0)
        return -1;

    /* get response */
    n = recv(dev->handle, &p, sizeof(p), MSG_WAITALL);

    if(n == sizeof(p) && p.cmd == HWSERVER_DEV_CLOSE_ACK)
        return 0;
    else
        return -1;
}
