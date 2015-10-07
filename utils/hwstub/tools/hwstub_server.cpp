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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <vector>
#include <stdexcept>
    
#include "hwstub.h"

volatile int exit_flag = 0;
libusb_hotplug_callback_handle hotplug_handle;
bool hotplug_support = false;

std::vector < pthread_t * > g_threadlist;
pthread_mutex_t threadlist_mutex = PTHREAD_MUTEX_INITIALIZER;

/* A device node represents an available device.
 * Schematically, there are two types of devices:
 * - connected devices: those are connected and may or may not be in use
 * - disconnected devices: those stay in the list until all the handles to it
 *   have been released, device will reject any operations.
 * As long as a dev_node exists, it maintins a positive (>=1) reference count
 * on the libusb_device given to the constructor.
 */
class dev_node {
private:
    /* Unique device id */
    static int32_t uid;

    /* Once a device is disconnected, this variable will be set to false and
     * the dev_node cannot be used anymore
     */
    bool connected;

    /* Number of times the dev_node has been opened.
     * A dev_node exists as long as the following two conditions are satisfies:
     * - device is connected
     * - ref_cnt is nonzero
     * In particular, a disconnected device will continue to exist until
     * ref_cnt is 0 (ie all users have closed it)
     */
    int ref_cnt;

public:
    struct hwstub_device_t *hwdev;
    struct libusb_device *dev;
    struct dev_info_t devinfo;

    /* Creates a dev_node and adds a reference to dev.
     * Note that the constructor will probe the device to get some
     * information about. If it fails to do so, the device will be marked
     * as disconnected.
     */
    dev_node(struct libusb_device *dev);
    ~dev_node(void);

    /* Add a reference to the device, opening it if necessary.
     * If it returns true then hwdev handle can be used to send requests to the
     * device and stays valid until close() is called.
     * If the device fails to open, the function returns false and no reference
     * is added.
     */
    bool open(void);

    /* Remove a reference to the device, close it if no one is using it
     * anymore.
     */
    void close(void);

    /* Check if the device is still connected. */
    bool is_connected(void);


    /* Marks the device as disconnected. */
    void disconnect(void);

    /* returns true if anyone still has a reference to the device
     * (ie open()'d but not close()'d)
     */
    bool is_referenced(void);
};

int32_t dev_node::uid = 0;

/* Returns true if there is at least one client keeping this device opened.
 * It is client duty to call close() when it doesn't want to use this device
 * anymore.
 */
bool dev_node::is_referenced(void)
{
    return ref_cnt ? true : false;
}

/* Opens hwstub device (if not opened already) and increments reference
 * count for this device.
 * NOTE: caller needs to aquire the lock
 */
bool dev_node::open(void)
{
    if (hwdev == NULL)
    {
        /* not opened by any client */
        hwdev = hwstub_open_usb(dev);
    }

    if (hwdev)
    {
        ref_cnt++;
        return true;
    }

    return false;
}

/* Decrements reference count for this device. If after this device is not
 * referenced by any other client, releases hwstub device.
 * NOTE: caller needs to aquire the lock
 */
void dev_node::close(void)
{
    /* decrement ref counter */
    ref_cnt--;

    /* release dev if not in use */
    if (ref_cnt <= 0 && hwdev)
    {
        hwstub_release(hwdev);
        hwdev = NULL;
    }
}

/* Marks device as disconnected. */
void dev_node::disconnect(void)
{
    connected = false;
}

/* Returns true is device is connected */
bool dev_node::is_connected(void)
{
    return connected;
}

dev_node::dev_node(struct libusb_device *_dev) :
    connected(false), ref_cnt(0), hwdev(NULL), dev(libusb_ref_device(_dev)),
    devinfo()
{
    struct hwstub_device_t *hwstub_dev = hwstub_open_usb(dev);
    if (hwstub_dev)
    {
        /* get target descriptor */
        int sz = hwstub_get_desc(hwstub_dev, HWSTUB_DT_TARGET,
                                 &(this->devinfo.hwdesc),
                                 sizeof(struct hwstub_target_desc_t));
        /* release the device */
        hwstub_release(hwstub_dev);

        if (sz == sizeof(struct hwstub_target_desc_t))
        {
            devinfo.id = uid++;
            connected = true;
        }
    }
}

dev_node::~dev_node()
{
    if(ref_cnt != 0)
        throw std::runtime_error("dev_node destroyed with references on it");

    if (hwdev)
        hwstub_release(hwdev);

    libusb_unref_device(dev);
}

/* Global list of devices, protected by mutex */
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
*find_node_by_id(const std::vector< dev_node * > &list, int32_t id)
{
    for (size_t i=0; i<list.size(); i++)
    {
        if (list[i]->devinfo.id == id)
            return list[i];
    }

    return NULL;
}

int hwstub_tcp_send(int connfd, int32_t id, uint32_t cmd,
                    void *buf, uint32_t len)
{
    int n;
    struct hwstub_tcp_cmd_t p = {
        .id = id,
        .cmd = cmd,
        .addr = 0,
        .len = len
    };

    n = send(connfd, &p, sizeof(p), 0);
    if (n > 0 && buf && len)
        n = send(connfd, buf, len, 0);

    return n;
}

/* Remove any device from the global list if they meet the deletion criteria:
 * no references and disconnected.
 * NOTE: use protected with mutex!!!
 */
void hwstub_cleanup_devices(void)
{
    size_t i = 0;
    while(i < g_devlist.size())
    {
        if(!g_devlist[i]->is_connected() && !g_devlist[i]->is_referenced())
        {
            delete g_devlist[i];
            g_devlist.erase(g_devlist.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

/* Close every device in this list and clear the list.
 * This function will cleanup the global list if necessary.
 * NOTE: will try to lock the global list AND unlock afterwards */
void hwstub_tcp_handler_cleanup(void *_list)
{
    std::vector< dev_node *> *list = (std::vector< dev_node *> *)_list;

    pthread_mutex_trylock(&devlist_mutex);
    for (size_t i=0; i<list->size(); i++)
    {
        (*list)[i]->close();
    }
    list->clear();

    hwstub_cleanup_devices();
    pthread_mutex_unlock(&devlist_mutex);

    pthread_mutex_lock(&threadlist_mutex);
    for (size_t i=0; i<g_threadlist.size(); i++)
    {
        if (pthread_equal(*(g_threadlist[i]), pthread_self()))
        {
            /* Prevent thread from leaking memory */
            pthread_detach(pthread_self());

            /* Remove from global thread list */
            delete g_threadlist[i];
            g_threadlist.erase(g_threadlist.begin() + i);
            break;
        }
    }
    pthread_mutex_unlock(&threadlist_mutex);
}

void *hwstub_tcp_handler(void *arg)
{
    int connfd = *(int *)arg;
    struct hwstub_tcp_cmd_t p;
    ssize_t n;
    int status = HWERR_OK;
    void *buf = NULL;

    /* per client list of devices which are 'opened' */
    std::vector< dev_node * > reflist;
    dev_node *refdev;

    struct timeval tv;
    tv.tv_sec = 30;  /* 30 secs timeout */
    tv.tv_usec = 0;  /* init this field to avoid strange errors */
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    /* avoid race condition when creating new client thread */
    if (arg) delete (int *)arg;

    /* register cleanup function */
    pthread_cleanup_push(&hwstub_tcp_handler_cleanup, (void *)&reflist);

    printf("hwstub_tcp_handler: %d\n", pthread_self());
    while (1)
    {
        /* give thread a chance to be canceled by main thread */
        pthread_testcancel();

        n = recv(connfd, &p, sizeof(p), MSG_WAITALL);
        if (n <= 0)
        {
            if (n == -1)
                continue; /* recv timeout */
            else
            {
                pthread_exit(NULL); /* error or client disconnect */
            }
        }

        printf("got cmd: 0x%08x addr: 0x%08x len: 0x%08x\n", p.cmd, p.addr, p.len);

        status = HWERR_OK;
        n = 0;
        buf = NULL;

        pthread_mutex_lock(&devlist_mutex);
        refdev = find_node_by_id(reflist, p.id);
        if (refdev == NULL)
        {
            if (p.cmd != HWSERVER_DEV_OPEN && p.cmd != HWSERVER_GET_DEV_LIST)
            {
                status = HWERR_NOT_OPEN;
                goto Lstatus;
            }
        }
        else
        {
            if(!refdev->is_connected())
            {
                if (p.cmd != HWSERVER_DEV_CLOSE)
                {
                    status = HWERR_DISCONNECTED;
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
                        if (buf) free(buf);
                        pthread_exit(NULL); /* error or client disconnect */
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

            /* server level requests */
            case HWSERVER_GET_DEV_LIST:
            {
                printf("HWSERVER_GET_DEV_LIST\n");
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
                                printf("HWSERVER_GET_DEV_LIST: %d:%s\n",
                                       g_devlist[i]->devinfo.id,
                                       g_devlist[i]->devinfo.hwdesc.bName);
                                memcpy(ptr++, &g_devlist[i]->devinfo, sizeof(struct dev_info_t));
                                n += sizeof(struct dev_info_t);
                            }
                        }
                    }
                }
            }
            break;

            case HWSERVER_DEV_OPEN:
            {
                if (refdev == NULL)
                {
                    dev_node *dev = find_node_by_id(g_devlist, p.id);
                    if (dev == NULL)
                    {
                        /* bogus id */
                        status = HWERR_INVALID_ID;
                    }
                    else
                    {
                        if (!dev->open())
                        {
                            pthread_mutex_unlock(&devlist_mutex);
                            status = HWERR_FAIL;
                            break;
                        }

                        /* add to reflist */
                        reflist.push_back(dev);
                    }
                }
            }
            break;

            case HWSERVER_DEV_CLOSE:
            {
                refdev->close();

                /* remove from reflist */
                for (size_t i=0; i<reflist.size(); i++)
                {
                    if (reflist[i] == refdev)
                    {
                        reflist.erase(reflist.begin() + i);
                        break;
                    }
                }

                hwstub_cleanup_devices();
            }
            break;

            default:
            break;
        } /* switch (p.cmd) */
Lstatus:
            pthread_mutex_unlock(&devlist_mutex);

            /* send response */
            hwstub_tcp_send(connfd, p.id,
                            status ? HWSTUB_NACK(p.cmd) : HWSTUB_ACK(p.cmd),
                            status ? NULL : (buf ? buf : NULL), status ? status : n);

            /* cleanup */
            if (buf)
                free(buf);

    } /* while */

    /* Needed as pthread_cleanup_push() and pthread_cleanup_pop()
     * may be implemented as paired macros */
    pthread_cleanup_pop(1);
    return NULL;
}

static void add_to_devlist(struct libusb_device *dev)
{
    dev_node *node = new dev_node(dev);
    if (node->is_connected())
    {
        pthread_mutex_lock(&devlist_mutex);
        g_devlist.push_back(node);
        printf("Add device to g_devlist: %d:%s\n", node->devinfo.id, node->devinfo.hwdesc.bName);
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
                node->disconnect();

            hwstub_cleanup_devices();
            pthread_mutex_unlock(&devlist_mutex);
        }
        break;
    }

    return 0;
}

/* We need to call libusb_handle_events_completed() in order to pass down the
 * road usb events so connect/disconnect can be picked up by registered
 * callback. This looks like a bug/feature of libusb which is not documented
 * anywhere.
 */
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

/* Checks if system has hotplug support in libusb and if so
 * registers callback to track connection/disconnection of
 * usb devices on the fly.
 */
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
            exit(EXIT_FAILURE);

        if (pthread_create(&usb_ev_poll_thread_id, NULL, &usb_ev_poll, NULL) != 0)
        {
            libusb_hotplug_deregister_callback(NULL, hotplug_handle);
            exit(EXIT_FAILURE);
        }
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
    pthread_join(usb_ev_poll_thread_id, NULL);
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

/* Cleanup function called at process termination */
void cleanup(void)
{
    /* Cancel worker threads */
    while (g_threadlist.size())
    {
        pthread_t tid = *(g_threadlist[0]);
        pthread_cancel(tid);
    }

    /* Probably redundand but harmless */
    g_threadlist.clear();

    device_search_fini();
    if (listenfd > 0) close(listenfd);
}

void demonize(void (*fn)(void *), void *arg)
{
    /* Our process ID and Session ID */
    pid_t pid, sid;
    
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
            
    /* Open any logs here */        
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }
    
    /* Change the current working directory */
    if ((chdir("/")) < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }
    
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* redirect stdin, stdout, and stderr to /dev/null */
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);    

    /* Call daemon worker function */
    (*fn)(arg);    
    exit(EXIT_SUCCESS);
}

void main_worker(void *arg)
{
    int *clientfd = NULL;
    int server_port = *(int *)arg;
    struct sockaddr_in serv_addr;
    struct sigaction act;
    pthread_t *tid;

    device_search_init();
    if (atexit(&cleanup) != 0)
    {
        printf("DBG: unable to register atexit handler\n");
        device_search_fini();
        exit(EXIT_FAILURE);
    }

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

    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = &signal_handler;
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        printf("DBG: Cannot register SIGINT handler: %s\n", strerror(errno));
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    while (!exit_flag)
    {
       /* accept new connection */
        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        if (connfd > 0)
        {
            printf("DBG: Client connect %d\n", connfd);
            clientfd = new int(connfd);
            if (clientfd == NULL)
                exit(EXIT_FAILURE);

            tid = new pthread_t();
            if (tid == NULL)
                exit(EXIT_FAILURE);

            if (pthread_create(tid, NULL, &hwstub_tcp_handler, (void *)clientfd) == 0)
            {
                pthread_mutex_lock(&threadlist_mutex);
                g_threadlist.push_back(tid);
                pthread_mutex_unlock(&threadlist_mutex);
            }
        }
    }
    exit(EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
    bool background = false;
    int server_port = 8888;
    int c;

    const struct option long_options[] =
    {
        {"daemon", no_argument,     0, 'd'},
        {"help", no_argument,       0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0,      0,                 0,  0}
    };

    while ((c = getopt_long(argc, argv, "dhp:", long_options, NULL)) != -1)
    {
        switch(c)
        {
            case 'h':
                usage();
                break;

            case 'p':
                server_port = atoi(optarg);
                break;

            case 'd':
                background = true;
                break;

            default:
                usage();
                break;
        }
    }

    if (background)
        demonize(&main_worker, (void *)&server_port);
    else
        main_worker((void *)&server_port);
}
