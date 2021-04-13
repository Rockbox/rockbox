/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "config.h"
#include "debug.h"
#include "powermgmt.h"

#include "debug-ibasso.h"


/*
    Without this socket iBasso Vold will not start.
    iBasso Vold uses this to send status messages about storage devices.
*/
static const char VOLD_MONITOR_SOCKET_NAME[] = "UNIX_domain";
static int _vold_monitor_socket_fd           = -1;

static void vold_monitor_open_socket(void)
{
    TRACE;

    unlink(VOLD_MONITOR_SOCKET_NAME);

    _vold_monitor_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(_vold_monitor_socket_fd < 0)
    {
        _vold_monitor_socket_fd = -1;
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VOLD_MONITOR_SOCKET_NAME, sizeof(addr.sun_path) -1);

    if(bind(_vold_monitor_socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return;
    }

    if(listen(_vold_monitor_socket_fd, 1) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return;
    }
}

/* Track state of external SD */
bool extsd_present = true; /* Worst-case is it will show up empty */

/*
    bionic does not have pthread_cancel.
    0: Vold monitor thread stopped/ending.
    1: Vold monitor thread started/running.
*/
static volatile sig_atomic_t _vold_monitor_active = 0;


/*
    1: /mnt/sdcard is unmounting
    0: else
*/
static volatile sig_atomic_t _vold_monitor_forced_close_imminent = 0;


static void* vold_monitor_run(void* nothing)
{
    _vold_monitor_active = 1;

    (void) nothing;

    DEBUGF("DEBUG %s: Thread start.", __func__);

    /* Check to see if external SD is mounted */
//    extsd_present = !system("mountpoint -q /mnt/external_sd");
    extsd_present = !system("mount -o remount,rw /mnt/external_sd");

    vold_monitor_open_socket();
    if(_vold_monitor_socket_fd < 0)
    {
        DEBUGF("ERROR %s: Thread end: No socket.", __func__);

        _vold_monitor_active = 0;
        return 0;
    }

    struct pollfd fds[1];
    fds[0].fd     = _vold_monitor_socket_fd;
    fds[0].events = POLLIN;

    while(_vold_monitor_active == 1)
    {
        poll(fds, 1, 10);
        if(! (fds[0].revents & POLLIN))
        {
            continue;
        }

        int socket_fd = accept(_vold_monitor_socket_fd, NULL, NULL);

        if(socket_fd < 0)
        {
            DEBUGF("ERROR %s: accept failed.", __func__);

            continue;
        }

        while(true)
        {
            char msg[1024];
            memset(msg, 0, sizeof(msg));
            int length = read(socket_fd, msg, sizeof(msg));

            if(length <= 0)
            {
                close(socket_fd);
                break;
            }

            DEBUGF("%s: msg: %s", __func__, msg);

            if(strcmp(msg, "Volume flash /mnt/sdcard state changed from 4 (Mounted) to 5 (Unmounting)") == 0)
            {
                /* We are losing /mnt/sdcard, shutdown Rockbox before it is forced closed. */

                _vold_monitor_forced_close_imminent = 1;
                sys_poweroff();
                _vold_monitor_active = 0;
            }
            else if(strcmp(msg, "Volume sdcard /mnt/external_sd state changed from 4 (Mounted) to 5 (Unmounting)") == 0)
            {
                /* We are loosing the external sdcard, inform Rockbox. */
                extsd_present = false;
            }
            else  if(strcmp(msg, "Volume sdcard /mnt/external_sd state changed from 3 (Checking) to 4 (Mounted)") == 0)
            {
                /* The external sdcard is back, inform Rockbox. */
                extsd_present = true;
            }
        }
    }

    close(_vold_monitor_socket_fd);
    unlink(VOLD_MONITOR_SOCKET_NAME);
    _vold_monitor_socket_fd = -1;

    DEBUGF("%s: Thread end.", __func__);

    _vold_monitor_active = 0;
    return 0;
}


/* Vold monitor thread. */
static pthread_t _vold_monitor_thread;


void vold_monitor_start(void)
{
    TRACE;

    if(_vold_monitor_active == 0)
    {
        pthread_create(&_vold_monitor_thread, NULL, vold_monitor_run, NULL);
    }
}


bool vold_monitor_forced_close_imminent(void)
{
    TRACE;

    return(_vold_monitor_forced_close_imminent == 1);
}
