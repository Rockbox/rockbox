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

#include "hwstub.h"

static volatile int exit_flag = 0;
struct hwstub_version_desc_t hwdev_ver;
static libusb_hotplug_callback_handle hotplug_handle;
static bool hotplug_support = false;

struct dev_info_t {
    int32_t dev_ref;
    struct hwstub_target_desc_t hwdesc;
};

struct dev_node_t {
    struct libusb_device *dev;
    struct hwstub_device_t *hwdev;
    struct dev_info_t devinfo;
    int32_t ref_cnt;
};

/* (L)inked (L)ist (D)double
 *
 * Format:
 *
 *      0<-XX<->YY<->ZZ->0
 * head----^         ^
 * tail--------------+
 */
struct lld_head
{
    struct lld_node *head; /* First list item */
    struct lld_node *tail; /* Last list item (to make insert_last O(1)) */
    size_t cnt;            /* Number of nodes */
};

struct lld_node
{
    struct lld_node *next; /* Next list item */
    struct lld_node *prev; /* Previous list item */
    void *data;
};

static struct lld_head dev_list = { .head = NULL, .tail = NULL, .cnt = 0 };

void signal_handler(int signum)
{
    (void)signum;
    exit_flag = 1;
}

static int hwstub_tcp_send(int connfd, uint32_t cmd, void *buf, uint32_t len)
{
    int n;
    struct hwstub_tcp_cmd_t p = {
        .cmd = cmd,
        .addr = 0,
        .len = len
    };

    n = send(connfd, &p, sizeof(p), 0);
    if (n > 0 && buf && len)
        n = send(connfd, buf, len, 0);

    return n;
}

/**
 * Adds a node to a doubly-linked list using "insert last"
 */
void lld_insert_last(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *tail = list->tail;

    list->tail = node;

    if (tail == NULL)
        list->head = node;
    else
        tail->next = node;

    node->next = NULL;
    node->prev = tail;
    list->cnt++;
}

/**
 * Removes a node from a doubly-linked list
 */
void lld_remove(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *next = node->next;
    struct lld_node *prev = node->prev;

    if (node == list->head)
        list->head = next;

    if (node == list->tail)
        list->tail = prev;

    if (prev != NULL)
        prev->next = next;

    if (next != NULL)
        next->prev = prev;

    list->cnt--;
}

void lld_remove_free(struct lld_head *list, struct lld_node *node)
{
    lld_remove(list, node);
    if (node->data) free(node->data);
    if (node) free(node);
}

static struct lld_node *find_node_by_dev(struct lld_head *list, struct libusb_device *dev)
{
    if (list->head)
    {
        struct lld_node *n = list->head;

        do
        {
            struct dev_node_t *devnode = (struct dev_node_t *)(n->data);
            if (devnode->dev == dev)
                return n;
        } while (n->next);
    }

    return NULL;

}

static struct lld_node *find_node_by_ref(struct lld_head *list, int ref)
{
    if (list->head)
    {
        struct lld_node *n = list->head;

        do
        {
            struct dev_node_t *devnode = (struct dev_node_t *)(n->data);
            if (devnode->devinfo.dev_ref == ref)
                return n;
        } while (n->next);
    }

    return NULL;
}

static bool opened_ref(struct lld_head *list, int32_t ref)
{
    return (find_node_by_ref(list, ref) != NULL);
}

static int hwstub_tcp_handler(int connfd)
{
     struct hwstub_tcp_cmd_t p;
     ssize_t n;
     void *buf;

     struct lld_head reflist = {.head = NULL, .tail = NULL, .cnt = 0};
     struct lld_node *node;
     struct dev_node_t *dev;

     while (1)
     {
         n = recv(connfd, &p, sizeof(p), MSG_WAITALL);
         if (n <= 0)
         {
             if (n == -1)
                 continue; /* recv timeout */
             else
                 return 10*n; /* error or client disconnect */
         }

         printf("got cmd: 0x%08x addr: 0x%08x len: 0x%08x\n", p.cmd, p.addr, p.len);

         switch (p.cmd)
         {
             case HWSTUB_GET_LOG:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);
                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 if ((n =  hwstub_get_log(dev->hwdev, buf, p.len)) >= 0)
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_FAIL);

                 free(buf);
             break;

             case HWSTUB_READ2:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);

                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 if ((n = hwstub_rw_mem(dev->hwdev, 1, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_READ2_NACK, buf, n);

                 free(buf);
             break;

             case HWSTUB_WRITE:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);

                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 n = recv(connfd, buf, p.len, MSG_WAITALL);
                 if (n <= 0)
                     return -3; /* error or client disconnect */

                 if ((n = hwstub_rw_mem(dev->hwdev, 0, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ACK, NULL, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_NACK, NULL, n);

                 free(buf);
             break;

             case HWSTUB_EXEC:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);

                 if (hwstub_exec(dev->hwdev, p.addr, p.len))
                     hwstub_tcp_send(connfd, HWSTUB_EXEC_NACK, NULL, 0);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_EXEC_ACK, NULL, 0);
             break;

             case HWSTUB_READ2_ATOMIC:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);
                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 if ((n = hwstub_rw_mem_atomic(dev->hwdev, 1, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ATOMIC_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ATOMIC_NACK, buf, n);

                 free(buf);
             break;

             case HWSTUB_WRITE_ATOMIC:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);
                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 n = recv(connfd, buf, p.len, MSG_WAITALL);
                 if (n <= 0)
                     return -3; /*error or client disconnect */

                 if ((n = hwstub_rw_mem_atomic(dev->hwdev, 0, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ATOMIC_ACK, NULL, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ATOMIC_NACK, NULL, n);

                 free(buf);
             break;

             case HWSTUB_GET_DESC:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (!node)
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_INVALID_REF);
                     break;
                 }

                 if (!opened_ref(&reflist, p.ref));
                 {
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, NULL, HWERR_NOTOPEN_REF);
                     break;
                 }

                 dev = (struct dev_node_t *)(node->data);
                 buf = malloc(p.len);

                 if (buf == NULL)
                     return -2;

                 if ((n = hwstub_get_desc(dev->hwdev, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_GET_DESC_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_GET_DESC_NACK, NULL, 0);

                 free(buf);
             break;

             case HWSERVER_GET_DEV_LIST:
                 n = dev_list.cnt*sizeof(struct dev_info_t);
                 buf = malloc(n);

                 if (buf == NULL)
                     return -2;

                 struct dev_info_t *ptr = (struct dev_info_t *)buf;
                 node = dev_list.head;
                 if (node)
                 {
                     dev = (struct dev_node_t *)(node->data);
                     for(int i=0; i<dev_list.cnt; i++)
                     {
                         memcpy(ptr++, &dev->devinfo, sizeof(struct dev_info_t));
                         node = node->next;
                     }
                 }

                 hwstub_tcp_send(connfd, HWSERVER_GET_DEV_LIST_ACK, buf, n);
                 free(buf);
             break;

             case HWSERVER_DEV_OPEN:
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (node)
                 {
                     struct dev_node_t *dev = (struct dev_node_t *)(node->data);
                     if (dev->hwdev)
                     {
                         if (!opened_ref(&reflist, p.ref))
                         {
                             // FIXME
                             struct lld_node *refnode = malloc(sizeof(struct lld_node));
                             refnode->data = (void *)dev;
                             lld_insert_last(&reflist, refnode);

                             dev->ref_cnt++; // increase global reference counter
                         }

                         hwstub_tcp_send(connfd, HWSERVER_DEV_OPEN_ACK, NULL, n);
                     }
                     else
                     {
                         // open usb device
                         dev->hwdev = hwstub_open_usb(dev->dev);
                         if (dev->hwdev)
                         {
                             // report success
                             lld_insert_last(&reflist, node);
                             dev->ref_cnt++;
                             hwstub_tcp_send(connfd, HWSERVER_DEV_OPEN_ACK, NULL, n);
                         }
                         else
                         {
                             // report failure
                             hwstub_tcp_send(connfd, HWSERVER_DEV_OPEN_NACK, NULL, -1);
                         }
                     }
                 }
                 else
                 {
                     // report no device found
                     hwstub_tcp_send(connfd, HWSERVER_DEV_OPEN_NACK, NULL, -2);
                 }
             break;

             case HWSERVER_DEV_CLOSE:
                 // check if ref is valid
                 node = find_node_by_ref(&dev_list, p.ref);
                 if (node)
                 {
                     // check if we opened this ref
                     struct lld_node *refnode = find_node_by_ref(&reflist, p.ref);
                     if (refnode)
                     {
                         dev = (struct dev_node_t *)(node->data);

                         // prevent feeing global dev_node_t structure
                         refnode->data = NULL;
                         //remove from reflist
                         lld_remove_free(&reflist, refnode);
                         // decrement global ref counter
                         dev->ref_cnt--;

                         // release dev if not in use
                         if (dev->ref_cnt == 0)
                             hwstub_release(dev->hwdev);

                         hwstub_tcp_send(connfd, HWSERVER_DEV_CLOSE_ACK, NULL, 0);
                     }
                     else
                     {
                         // not owned device
                         hwstub_tcp_send(connfd, HWSERVER_DEV_CLOSE_NACK, NULL, -1);
                     }
                 }
                 else
                 {
                     // invalid ref
                     hwstub_tcp_send(connfd, HWSERVER_DEV_CLOSE_NACK, NULL, -2);
                 }
             break;

             default:
             break;
         }

     } /* while */

     return 0;
}


void add_dev_to_list(struct lld_head *list, struct libusb_device *dev)
{
    static int ref = 0;

    // try to open the device
    struct hwstub_device_t *hwstub_dev = hwstub_open_usb(dev);
    if (hwstub_dev)
    {
        // allocate memory
        struct lld_node *item = malloc(sizeof(struct lld_node));
        if (item)
        {
            struct dev_node_t *dev_node = malloc(sizeof(struct dev_node_t));
            if (dev_node)
            {
                // get target descriptor
                int sz = hwstub_get_desc(hwstub_dev, HWSTUB_DT_TARGET,
                                         &(dev_node->devinfo.hwdesc),
                                         sizeof(struct hwstub_target_desc_t));

                if (sz != sizeof(struct hwstub_target_desc_t))
                {
                    // error - free memory and close the dev
                    free(item);
                    free(dev_node);
                    hwstub_release(hwstub_dev);
                    return;
                }

                dev_node->dev = dev;
                dev_node->devinfo.dev_ref = ref++;
                dev_node->ref_cnt = 0;
                item->data = dev_node;

                lld_insert_last(list, item);
            }
            else
            {
                free(item);
                hwstub_release(hwstub_dev);
                return;
            }
        }
        hwstub_release(hwstub_dev);
    }
}

void del_dev_from_list(struct lld_head *list, struct libusb_device *dev)
{
    struct lld_node *node = find_node_by_dev(list, dev);
    if (node)
        lld_remove_free(list, node);
}



int hotplug_cb(struct libusb_context *ctx, struct libusb_device *dev,
               libusb_hotplug_event event, void *user_data)
{
    switch(event)
    {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            if (hwstub_probe(dev) >= 0)
                add_dev_to_list(&dev_list, dev);
            break;

        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
                del_dev_from_list(&dev_list, dev);
            break;
    }

    return 0;
}

static void device_search_init(void)
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
                  LIBUSB_HOTPLUG_MATCH_ANY, hotplug_cb, NULL, &hotplug_handle);
    }
}

static void device_search_fini(void)
{
    if (hotplug_support)
        libusb_hotplug_deregister_callback(NULL, hotplug_handle);

    for (int i=0; i<dev_list.cnt; i++)
    {
        struct lld_node *node = dev_list.head;
        if (node)
        {
            struct dev_node_t *dev = (struct dev_node_t *)(node->data);
            if (dev->hwdev)
                hwstub_release(dev->hwdev);

            lld_remove_free(&dev_list, node);
        }
    }

    libusb_exit(NULL);
}

static void usage(void)
{
    printf("hwstub_server compiled with hwstub protocol %d.%d\n",
        HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    printf("\n");
    printf("usage: hwstub_server [options]\n");
    printf("    --help/-h       Display this help\n");
    printf("    --port/-p       TCP port server is listening\n");
    printf("    --dev/-d <uri>  Device URI\n");
    hwstub_usage_uri(stdout);
    exit(1);
}

int main (int argc, char **argv)
{
    int server_port = 8888;
    int listenfd = -1, connfd = -1;
    int c;
    struct sockaddr_in serv_addr;
    struct sigaction act;
    pid_t pid;
    const char *dev_uri = "usb:";

    const struct option long_options[] =
    {
        {"help", no_argument,       0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"dev", required_argument, 0, 'd'},
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

            case 'd':
                dev_uri = optarg;
                break;

            default:
                usage();
                break;
        }
    }

    device_search_init();

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
        /* accept new connection */
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        printf("DBG: Client connect\n");
        /* spawn child process */
        pid = fork();
        if (pid < 0)
        {
            printf("DBG: Cannot spawn child process: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            /* This is the child process */
            close(listenfd);

            struct timeval tv;
            tv.tv_sec = 30;  /* 30 secs timeout */
            tv.tv_usec = 0;  /* init this field to avoid strange errors */
            setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

            int ret = hwstub_tcp_handler(connfd);

            printf("DBG: Client disconnect reason: %d\n", ret);
            close(connfd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(connfd);
        }
    }

    printf("Hwstub server close\n");
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
#endif
    device_search_fini();
    if (listenfd > 0) close(listenfd);
    return 0;
}
