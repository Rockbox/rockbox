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
#include <signal.h>

#include "hwstub.h"

static volatile int exit_flag = 0;
struct hwstub_device_t *hwdev;
struct hwstub_version_desc_t hwdev_ver;

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

static void doprocessing(int connfd)
{
     struct hwstub_tcp_cmd_t p;
     ssize_t n;
     void *buf;

     while ((n = recv(connfd, &p, sizeof(p), MSG_WAITALL)) > 0)
     {
         printf("got cmd: 0x%08x addr: 0x%08x len: 0x%08x\n", p.cmd, p.addr, p.len);
         switch (p.cmd)
         {
             case HWSTUB_GET_LOG:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 if ((n =  hwstub_get_log(hwdev, buf, p.len)) >= 0)
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_GET_LOG_NACK, buf, 0);

                 free(buf);                 
             break;

             case HWSTUB_READ2:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 if ((n = hwstub_rw_mem(hwdev, 1, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_READ2_NACK, buf, n);

                 free(buf);
             break;

             case HWSTUB_WRITE:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 n = recv(connfd, buf, p.len, MSG_WAITALL);
                 if ((n = hwstub_rw_mem(hwdev, 0, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ACK, NULL, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_NACK, NULL, n);

                 free(buf);
             break;

             case HWSTUB_EXEC:
                 if (hwstub_exec(hwdev, p.addr, p.len))
                     hwstub_tcp_send(connfd, HWSTUB_EXEC_NACK, NULL, 0);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_EXEC_ACK, NULL, 0);
             break;

             case HWSTUB_READ2_ATOMIC:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 if ((n = hwstub_rw_mem_atomic(hwdev, 1, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ATOMIC_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_READ2_ATOMIC_NACK, buf, n);

                 free(buf);
             break;

             case HWSTUB_WRITE_ATOMIC:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 n = recv(connfd, buf, p.len, MSG_WAITALL);
                 if ((n = hwstub_rw_mem_atomic(hwdev, 0, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ATOMIC_ACK, NULL, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_WRITE_ATOMIC_NACK, NULL, n);

                 free(buf);
             break;

             case HWSTUB_GET_DESC:
                 buf = malloc(p.len);

                 if (buf == NULL)
                     exit(EXIT_FAILURE);

                 if ((n = hwstub_get_desc(hwdev, p.addr, buf, p.len)) == p.len)
                     hwstub_tcp_send(connfd, HWSTUB_GET_DESC_ACK, buf, n);
                 else
                     hwstub_tcp_send(connfd, HWSTUB_GET_DESC_NACK, NULL, 0);

                 free(buf);
             break;

             default:
             break;
         }
     }
}

int main (int argc, char **argv)
{
    int listenfd = -1, connfd = -1;
    struct sockaddr_in serv_addr;
    struct sigaction act;
    pid_t pid;

    // create usb context
    libusb_context *ctx;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    // look for device
    printf("Looking for device %#04x:%#04x...\n", HWSTUB_USB_VID, HWSTUB_USB_PID);

    libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx,
        HWSTUB_USB_VID, HWSTUB_USB_PID);
    if(handle == NULL)
    {
        fprintf(stderr, "No device found\n");
        return 1;
    }

    // admin stuff
    libusb_device *mydev = libusb_get_device(handle);
    printf("device found at %d:%d\n",
           libusb_get_bus_number(mydev),
           libusb_get_device_address(mydev));

    hwdev = hwstub_open(handle);
    if(hwdev == NULL)
    {
        fprintf(stderr, "Cannot probe device!\n");
        return 1;
    }

    // get hwstub information
    int ret = hwstub_get_desc(hwdev, HWSTUB_DT_VERSION, &hwdev_ver, sizeof(hwdev_ver));
    if(ret != sizeof(hwdev_ver))
    {
        fprintf(stderr, "Cannot get version!\n");
        goto Lerr;
    }
    if(hwdev_ver.bMajor != HWSTUB_VERSION_MAJOR || hwdev_ver.bMinor < HWSTUB_VERSION_MINOR)
    {
        printf("Warning: this tool is possibly incompatible with your device:\n");
        printf("Device version: %d.%d.%d\n", hwdev_ver.bMajor, hwdev_ver.bMinor, hwdev_ver.bRevision);
        printf("Host version: %d.%d\n", HWSTUB_VERSION_MAJOR, HWSTUB_VERSION_MINOR);
    }

    // tcp stuff
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("DBG: socket() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8888);

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
            doprocessing(connfd);
            printf("DBG: Client disconnect\n");
            close(connfd);
            exit(EXIT_SUCCESS); 
        }
        else
        {
            close(connfd);
        }
    }

    printf("Hwstub server close\n");
    Lerr:
    // display log if handled
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

    if (hwdev) hwstub_release(hwdev);
    if (listenfd > 0) close(listenfd);
    return 0;
}
