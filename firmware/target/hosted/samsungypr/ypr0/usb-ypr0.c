/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 - 2015 Lorenzo Miori
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
#include "config.h"
#include "stdbool.h"
#include "system.h"
#include "kernel.h"
#include "usb.h"
#include "sc900776.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "sys/ioctl.h"
#include "stdlib.h"

#define LOGF_ENABLE

#include "logf.h"
#include "usb.h"
#include "usb_drv.h"
#include "usb_core.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <asm/byteorder.h>

static sMinivet_t rs = { .addr = 0xA, .value = 0 };
static int minivet_dev = -1;

void minivet_init(void)
{
    minivet_dev = open("/dev/minivet", O_RDWR);
    logf("MINIVET: %d", minivet_dev);
}

void minivet_close(void)
{
    if (minivet_dev >= 0) {
        close(minivet_dev);
    }
}

int minivet_ioctl(int request, sMinivet_t *param) {
    return ioctl(minivet_dev, request, param);
}

void usb_init_device(void)
{
    minivet_init();
    /* Start usb detection... */
    usb_start_monitoring();
}

void usb_attach(void)
{
    logf("USB: attached");
}

// TODO don't care about gotos. they will be dead after the experiments ;)

int usb_detect(void)
{
    /* Check if this register is set...DEVICE_1 */
    if (minivet_ioctl(IOCTL_MINIVET_READ_BYTE, &rs) >= 0) {
        return (rs.value == MINIVET_DEVICETYPE1_USB) ? USB_INSERTED : USB_EXTRACTED;
    }
    return USB_EXTRACTED;
}

void usb_enable(bool on)
{
    if (on)
    {
        logf("USB: connected");
        usb_core_init();
    }
    else
    {
        logf("USB: disconnected");
        usb_core_exit();
    }
}
