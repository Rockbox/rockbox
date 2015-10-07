/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>

#include <vector>

#include "hwstub.h"

static volatile int exit_flag = 0;
struct hwstub_version_desc_t hwdev_ver;
static libusb_hotplug_callback_handle hotplug_handle;
static bool hotplug_support = false;

class dev_node {
private:
    static int32_t ref;
    bool connected;
    int ref_cnt;

public:
    struct hwstub_device_t *hwdev;
    struct libusb_device *dev;
    struct dev_info_t devinfo;

    dev_node(struct libusb_device *dev);
    ~dev_node(void);
    bool open(void);
    void close(void);
    bool is_connected(void);
    void disconnect(void);
    bool is_referenced(void);
};

int32_t dev_node::ref = 0;

bool dev_node::is_referenced(void)
{
    return ref_cnt ? true : false;
}

bool dev_node::open(void)
{
    if (hwdev == NULL)
    {
        // not opened by any client
        hwdev = hwstub_open_usb(dev);
    }

    if (hwdev)
    {
        ref_cnt++;
        return true;
    }

    return false;
}

void dev_node::close(void)
{
    // decrement ref counter
    ref_cnt--;

    // release dev if not in use
    if (ref_cnt <= 1 && hwdev)
        hwstub_release(hwdev);
}

void dev_node::disconnect(void)
{
    connected = false;
}

bool dev_node::is_connected(void)
{
    return connected;
}

dev_node::dev_node(struct libusb_device *_dev) :
    connected(false), ref_cnt(0), hwdev(NULL), dev(NULL), devinfo()
{
    struct hwstub_device_t *hwstub_dev = hwstub_open_usb(_dev);
    if (hwstub_dev)
    {
        // get target descriptor
        int sz = hwstub_get_desc(hwstub_dev, HWSTUB_DT_TARGET,
                                 &(this->devinfo.hwdesc),
                                 sizeof(struct hwstub_target_desc_t));
        // release the device
        hwstub_release(hwstub_dev);

        if (sz == sizeof(struct hwstub_target_desc_t))
        {
            dev = libusb_ref_device(_dev);
            ref_cnt = 1;
            devinfo.dev_ref = ref++;
            connected = true;
        }
    }
}

dev_node::~dev_node()
{
    if (hwdev)
        hwstub_release(hwdev);

    libusb_unref_device(dev);
}

std::vector< dev_node * > g_devlist;
pthread_mutex_t devlist_mutex = PTHREAD_MUTEX_INITIALIZER;

void signal_handler(int signum)
{
    (void)signum;
    exit_flag = 1;
}

dev_node
*find_node_by_dev(const std::vector < dev_node * > &list,
                  struct libusb_device *dev)
{
    for (size_t i=0; i<list.size(); i++)
    {
        if (list[i]->dev == dev)
            return list[i];
    }

    return NULL;
}

dev_node
*find_node_by_ref(const std::vector< dev_node * > &list, int32_t ref)
{
    for (size_t i=0; i<list.size(); i++)
    {
        if (list[i]->devinfo.dev_ref == ref)
            return list[i];
    }

    return NULL;
}

int hwstub_tcp_send(int connfd, int32_t ref, uint32_t cmd, void *buf, uint32_t len)
{
    int n;
    struct hwstub_tcp_cmd_t p = {
        .ref = ref,
        .cmd = cmd,
        .addr = 0,
        .len = len
    };

    n = send(connfd, &p, sizeof(p), 0);
    if (n > 0 && buf && len)
        n = send(connfd, buf, len, 0);

    return n;
}

void hwstub_tcp_handler_cleanup(std::vector< dev_node *> &list)
{
    pthread_mutex_lock(&devlist_mutex);
    for (size_t i=0; i<list.size(); i++)
    {
        list[i]->close();
        if (!list[i]->is_referenced())
        {
            for (size_t j=0; j<g_devlist.size(); j++)
            {
                if (g_devlist[j] == list[i])
                {
                   g_devlist.erase(g_devlist.begin() + j);
                   delete list[i];
                   break;
                }
            }
        }
    }
    pthread_mutex_unlock(&devlist_mutex);
    list.clear();
}

void *hwstub_tcp_handler(void *arg)
{
    int connfd = *(int *)arg;
    struct hwstub_tcp_cmd_t p;
    ssize_t n;
    int status = HWERR_OK;
    void *buf = NULL;

    // per client list of devices which are 'opened'
    std::vector< dev_node * > reflist;
    dev_node *refdev;

    struct timeval tv;
    tv.tv_sec = 30;  /* 30 secs timeout */
    tv.tv_usec = 0;  /* init this field to avoid strange errors */
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    // avoid race condition when creating new client thread
    if (arg) delete (int *)arg;

    while (1)
    {
        n = recv(connfd, &p, sizeof(p), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == -1)
                continue; /* recv timeout */
            else
            {
                hwstub_tcp_handler_cleanup(reflist);
                return NULL; /* error or client disconnect */
            }
        }

        printf("got cmd: 0x%08x addr: 0x%08x len: 0x%08x\n", p.cmd, p.addr, p.len);

        status = HWERR_OK;
        n = 0;

        refdev = find_node_by_ref(reflist, p.ref);
        if (refdev == NULL)
        {
            if (p.cmd != HWSERVER_DEV_OPEN && p.cmd != HWSERVER_GET_DEV_LIST)
            {
                status = HWERR_NOTOPEN_REF;
                goto Lstatus;
            }
        }
        else
        {
            if(!refdev->is_connected())
            {
                if (p.cmd != HWSERVER_DEV_CLOSE)
                {
                    status = HWERR_INVALID_REF;
                    goto Lstatus;
                }
            }
        }

        switch (p.cmd)
        {
            case HWSTUB_GET_LOG:
            {
                buf = malloc(p.len);
     
                if (buf == NULL)
                {
                    status = HWERR_FAIL;
                }
                else
                {
                    if ((n =  hwstub_get_log(refdev->hwdev, buf, p.len)) < 0)
                        status = HWERR_FAIL;
                }
            }
            break;

            case HWSTUB_EXEC:
                if (hwstub_exec(refdev->hwdev, p.addr, p.len))
                    status = HWERR_FAIL;
            break;

            case HWSTUB_READ2:
            case HWSTUB_READ2_ATOMIC:
            {
                buf = malloc(p.len);

                if (buf == NULL)
                {
                    status = HWERR_FAIL;
                }
                else
                {
                    if (p.cmd == HWSTUB_READ2)
                        n = hwstub_rw_mem(refdev->hwdev, 1, p.addr, buf, p.len);
                    else
                        n = hwstub_rw_mem_atomic(refdev->hwdev, 1, p.addr, buf, p.len);

                    if (n != p.len)
                        status = HWERR_FAIL;
                }
            }
            break;

            case HWSTUB_WRITE:
            case HWSTUB_WRITE_ATOMIC:
            {
                buf = malloc(p.len);

                if (buf == NULL)
                {
                    status = HWERR_FAIL;
                }
                else
                {
                    n = recv(connfd, buf, p.len, MSG_WAITALL);
                    if (n <= 0)
                    {
                        hwstub_tcp_handler_cleanup(reflist);
                        return NULL; /*error or client disconnect */
                    }

                    if (p.cmd == HWSTUB_WRITE)
                        n = hwstub_rw_mem(refdev->hwdev, 0, p.addr, buf, p.len);
                    else
                        n = hwstub_rw_mem_atomic(refdev->hwdev, 0, p.addr, buf, p.len);

                    if (n != p.len)
                        status = HWERR_FAIL;
                }
            }
            break;

            case HWSTUB_GET_DESC:
            {
                buf = malloc(p.len);

                if (buf == NULL)
                {
                    status = HWERR_FAIL;
                }
                else
                {
                    if ((n = hwstub_get_desc(refdev->hwdev, p.addr, buf, p.len)) != p.len)
                        status = HWERR_FAIL;
                }
            }
            break;

            // server level requests
            case HWSERVER_GET_DEV_LIST:
            {
                printf("HWSERVER_GET_DEV_LIST\n");
                pthread_mutex_lock(&devlist_mutex);
                //n = devlist.size() * sizeof(struct dev_info_t);
                buf = malloc(p.len);

                if (buf == NULL)
                {
                    status = HWERR_FAIL;
                }
                else
                {
                    struct dev_info_t *ptr = (struct dev_info_t *)buf;
                    for (size_t i=0; i<g_devlist.size(); i++)
                    {
                        if (g_devlist[i]->is_connected())
                        {
                            if (n + sizeof(struct dev_info_t) <= p.len)
                            {
                                printf("HWSERVER_GET_DEV_LIST: %d:%s\n", g_devlist[i]->devinfo.dev_ref, g_devlist[i]->devinfo.hwdesc.bName);
                                memcpy(ptr++, &g_devlist[i]->devinfo, sizeof(struct dev_info_t));
                                n += sizeof(struct dev_info_t);
                            }
                        }
                    }
                }
                pthread_mutex_unlock(&devlist_mutex);
            }
            break;

            case HWSERVER_DEV_OPEN:
            {
                pthread_mutex_lock(&devlist_mutex);
                dev_node *dev = find_node_by_ref(g_devlist, p.ref);
                if (dev == NULL)
                {
                    // bogus ref
                    status = HWERR_INVALID_REF;
                }
                else
                {
                    if (!dev->open())
                    {
                        pthread_mutex_unlock(&devlist_mutex);
                        status = HWERR_FAIL;
                        break;
                    }

                    // add to reflist
                    reflist.push_back(dev);
                }
                pthread_mutex_unlock(&devlist_mutex);
            }
            break;

            case HWSERVER_DEV_CLOSE:
            {
                pthread_mutex_lock(&devlist_mutex);

                refdev->close();

                //remove from reflist
                for (size_t i=0; i<reflist.size(); i++)
                {
                    if (reflist[i] == refdev)
                        reflist.erase(reflist.begin() + i);
                }

                if (!refdev->is_referenced())
                {
                    for (size_t i=0; i<g_devlist.size(); i++)
                    {
                        if (g_devlist[i] == refdev)
                        {
                            g_devlist.erase(g_devlist.begin() + i);
                            delete refdev;
                        }
                    }
                }

                pthread_mutex_unlock(&devlist_mutex);
            }
            break;

            default:
            break;
        } /* switch (p.cmd) */
Lstatus:
            // send response
            hwstub_tcp_send(connfd, p.ref,
                            status ? HWSTUB_NACK(p.cmd) : HWSTUB_ACK(p.cmd),
                            buf ? buf : NULL, status ? status : n);

            // cleanup
            if (buf)
                free(buf);

    } /* while */

    return NULL;
}

static void add_to_devlist(struct libusb_device *dev)
{
    dev_node *node = new dev_node(dev);
    if (node->is_connected() && node->is_referenced())
    {
        pthread_mutex_lock(&devlist_mutex);
        g_devlist.push_back(node);
        printf("Add device to g_devlist: %d:%s\n", node->devinfo.dev_ref, node->devinfo.hwdesc.bName);
        pthread_mutex_unlock(&devlist_mutex);
    }
    else
    {
        delete node;
    }
}

int hotplug_cb(struct libusb_context *ctx, struct libusb_device *dev,
               libusb_hotplug_event event, void *user_data)
{
    switch(event)
    {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
        {
            if (!find_node_by_dev(g_devlist, dev))
            {
                if (hwstub_probe(dev) >= 0)
                    add_to_devlist(dev);
            }
        }
        break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
        {
            pthread_mutex_lock(&devlist_mutex);
            dev_node *node = find_node_by_dev(g_devlist, dev);
            if (node)
            {
                node->disconnect();

                if (!node->is_referenced())
                {
                    for (size_t i=0; i<g_devlist.size(); i++)
                    {
                        if (g_devlist[i] == node)
                        {
                            g_devlist.erase(g_devlist.begin() + i);
                            break;
                        }
                    }
                    delete node;
                }
            }
            pthread_mutex_unlock(&devlist_mutex);
        }
        break;
    }

    return 0;
}

// We need to call libusb_handle_events_completed()
// in order to pass down the road usb events so
// connect/disconnect can be picked up by registered
// callback. This looks like a bug/feature of libusb
// which is not documented anywhere
void *usb_ev_poll(void *arg)
{
    while(1)
    {
        libusb_handle_events_completed(NULL, NULL);
        usleep(100000);
    }

    return NULL;
}

pthread_t usb_ev_poll_thread_id;
void device_search_init(void)
{
    int ret;

    libusb_init(NULL);

    hotplug_support = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);

    if (hotplug_support)
    {
        int evt = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                  LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT;

        ret = libusb_hotplug_register_callback(
                  NULL, (libusb_hotplug_event)evt, LIBUSB_HOTPLUG_ENUMERATE,
                  LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                  LIBUSB_HOTPLUG_MATCH_ANY, &hotplug_cb, NULL, &hotplug_handle);

        if (ret != LIBUSB_SUCCESS)
            exit(-1);

        if (pthread_create(&usb_ev_poll_thread_id, NULL, &usb_ev_poll, NULL) != 0)
            exit(-1);
    }
}

void device_search_fini(void)
{
    if (hotplug_support)
        libusb_hotplug_deregister_callback(NULL, hotplug_handle);

    pthread_mutex_lock(&devlist_mutex);

    for (size_t i=0; i<g_devlist.size(); i++)
        delete g_devlist[i];

    g_devlist.clear();

    pthread_mutex_unlock(&devlist_mutex);
    pthread_cancel(usb_ev_poll_thread_id);
    libusb_exit(NULL);
}

void usage(void)
{
    printf("hwstub_server compiled with hwstub protocol %d.%d\n",
        HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    printf("\n");
    printf("usage: hwstub_server [options]\n");
    printf("    --help/-h       Display this help\n");
    printf("    --port/-p       TCP port server is listening\n");
    hwstub_usage_uri(stdout);
    exit(1);
}

int listenfd = -1;
void cleanup(void)
{
    device_search_fini();
    if (listenfd > 0) close(listenfd);

    printf("Hwstub server close\n");
}

int main (int argc, char **argv)
{
    int server_port = 8888;
    int *connfd = NULL;
    int c;
    struct sockaddr_in serv_addr;
    struct sigaction act;
    pthread_t thread_id;

    const struct option long_options[] =
    {
        {"help", no_argument,       0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0,      0,                 0,  0}
    };

    while ((c = getopt_long(argc, argv, "hp:d:", long_options, NULL)) != -1)
    {
        switch(c)
        {
            case 'h':
                usage();
                break;

            case 'p':
                server_port = atoi(optarg);
                break;

            default:
                usage();
                break;
        }
    }

    device_search_init();
    atexit(&cleanup);

    /* tcp stuff */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("DBG: socket() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("DBG: Cannot bind listen socket: %s\n", strerror(errno));
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) < 0)
    {
        printf("DBG: listen() failed: %s\n", strerror(errno));
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = &signal_handler;
    if (sigaction(SIGTERM, &act, NULL) < 0)
    {
        printf("DBG: Cannot register SIGTERM handler: %s\n", strerror(errno));
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    while (!exit_flag)
    {
       connfd = new int();

       /* accept new connection */
        *connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("DBG: Client connect\n");

        if (pthread_create(&thread_id, NULL, &hwstub_tcp_handler, (void *)connfd) == 0)
            pthread_detach(thread_id);

    }
    /* display log if handled */
#if 0
    fprintf(stderr, "Device log:\n");
    do
    {
        char buffer[128];
        int length = hwstub_get_log(hwdev, buffer, sizeof(buffer) - 1);
        if(length <= 0)
            break;
        buffer[length] = 0;
        fprintf(stderr, "%s", buffer);
    }while(1);

    device_search_fini();
    if (listenfd > 0) close(listenfd);
#endif
    return 0;
}
