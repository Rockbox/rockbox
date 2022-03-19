/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "x1000bootloader.h"
#include "core_alloc.h"
#include "storage.h"
#include "button.h"
#include "kernel.h"
#include "usb.h"
#include "file.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "linuxboot.h"
#include "nand-x1000.h"

/* Set to true if a SYS_USB_CONNECTED event is seen
 * Set to false if a SYS_USB_DISCONNECTED event is seen
 * Handled by the gui code since that's how events are delivered
 * TODO: this is an ugly kludge */
bool is_usb_connected = false;

/* this is both incorrect and incredibly racy... */
int check_disk(bool wait)
{
    if(storage_present(IF_MD(0)))
        return DISK_PRESENT;
    if(!wait)
        return DISK_ABSENT;

    while(!storage_present(IF_MD(0))) {
        splashf(0, "Insert SD card\nPress " BL_QUIT_NAME " to cancel");
        if(get_button(HZ/4) == BL_QUIT)
            return DISK_CANCELED;
    }

    /* a lie intended to give time for mounting the disk in the background */
    splashf(HZ, "Scanning disk");

    return DISK_PRESENT;
}

void usb_mode(void)
{
    if(!is_usb_connected)
        splashf(0, "Waiting for USB\nPress " BL_QUIT_NAME " to cancel");

    while(!is_usb_connected)
        if(get_button(TIMEOUT_BLOCK) == BL_QUIT)
            return;

    splashf(0, "USB mode");
    usb_acknowledge(SYS_USB_CONNECTED_ACK);

    while(is_usb_connected)
        get_button(TIMEOUT_BLOCK);

    splashf(3*HZ, "USB disconnected");
}

int load_rockbox(const char* filename, size_t* sizep)
{
    if(check_disk(true) != DISK_PRESENT)
        return -1;

    int handle = core_alloc_maximum("rockbox", sizep, &buflib_ops_locked);
    if(handle < 0) {
        splashf(5*HZ, "Out of memory");
        return -2;
    }

    unsigned char* loadbuffer = core_get_data(handle);
    int rc = load_firmware(loadbuffer, filename, *sizep);
    if(rc <= 0) {
        core_free(handle);
        splashf(5*HZ, "Error loading Rockbox\n%s", loader_strerror(rc));
        return -3;
    }

    core_shrink(handle, loadbuffer, rc);
    *sizep = rc;

    return handle;
}

int load_uimage_file(const char* filename,
                     struct uimage_header* uh, size_t* sizep)
{
    if(check_disk(true) != DISK_PRESENT)
        return -1;

    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        splashf(5*HZ, "Can't open file\n%s", filename);
        return -2;
    }

    int handle = uimage_load(uh, sizep, uimage_fd_reader, (void*)(intptr_t)fd);
    if(handle <= 0) {
        splashf(5*HZ, "Cannot load uImage\n%s", filename);
        return -3;
    }

    return handle;
}

struct nand_reader_data
{
    nand_drv* ndrv;
    uint32_t addr;
    uint32_t end_addr;
};

static ssize_t uimage_nand_reader(void* buf, size_t count, void* rctx)
{
    struct nand_reader_data* d = rctx;

    if(d->addr + count > d->end_addr)
        count = d->end_addr - d->addr;

    int ret = nand_read_bytes(d->ndrv, d->addr, count, buf);
    if(ret != NAND_SUCCESS)
        return -1;

    d->addr += count;
    return count;
}

int load_uimage_flash(uint32_t addr, uint32_t length,
                      struct uimage_header* uh, size_t* sizep)
{
    int handle = -1;

    struct nand_reader_data n;
    n.ndrv = nand_init();
    n.addr = addr;
    n.end_addr = addr + length;

    nand_lock(n.ndrv);
    if(nand_open(n.ndrv) != NAND_SUCCESS) {
        splashf(5*HZ, "NAND open failed");
        nand_unlock(n.ndrv);
        return -1;
    }

    handle = uimage_load(uh, sizep, uimage_nand_reader, &n);

    nand_close(n.ndrv);
    nand_unlock(n.ndrv);

    if(handle <= 0) {
        splashf(5*HZ, "uImage load failed");
        return -2;
    }

    return handle;
}
