/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "rbscsi.h"

static void misc_std_printf(void *user, const char *fmt, ...)
{
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#if defined(_WIN32) || defined(__WIN32__)
#define RB_SCSI_WINDOWS
#include <windows.h>
typedef HANDLE rb_scsi_handle_t;
#elif defined(__linux) || defined(__linux__) || defined(linux)
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/sg_lib.h>
#include <scsi/sg_io_linux.h>
#define RB_SCSI_LINUX
typedef int rb_scsi_handle_t;
#else
typedef void *rb_scsi_handle_t;
#endif

struct rb_scsi_device_t
{
    rb_scsi_handle_t handle;
    void *user;
    rb_scsi_printf_t printf;
    unsigned flags;
};

#ifdef RB_SCSI_LINUX
rb_scsi_device_t rb_scsi_open(const char *path, unsigned flags, void *user,
    rb_scsi_printf_t printf)
{
    if(printf == NULL)
        printf = misc_std_printf;
    int fd = open(path, (flags & RB_SCSI_READ_ONLY) ? O_RDONLY : O_RDWR);
    if(fd < 0)
    {
        if(flags & RB_SCSI_DEBUG)
            printf(user, "rb_scsi: open failed: %s\n", strerror(errno));
        return NULL;
    }
    rb_scsi_device_t dev = malloc(sizeof(struct rb_scsi_device_t));
    dev->flags = flags;
    dev->handle = fd;
    dev->user = user;
    dev->printf = printf ? printf : misc_std_printf;
    return dev;
}

int rb_scsi_raw_xfer(rb_scsi_device_t dev, struct rb_scsi_raw_cmd_t *raw)
{
#define rb_printf(...) \
    do{ if(dev->flags & RB_SCSI_DEBUG) dev->printf(dev->user, __VA_ARGS__); }while(0)
    sg_io_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    /* prepare transfer descriptor */
    hdr.interface_id = 'S';
    switch(raw->dir)
    {
        case RB_SCSI_NONE: hdr.dxfer_direction = SG_DXFER_NONE; break;
        case RB_SCSI_READ: hdr.dxfer_direction = SG_DXFER_FROM_DEV; break;
        case RB_SCSI_WRITE: hdr.dxfer_direction = SG_DXFER_TO_DEV; break;
        default: rb_printf("rb_scsi: invalid direction"); return -1;
    }
    hdr.cmd_len = raw->cdb_len;
    hdr.mx_sb_len = raw->sense_len;
    hdr.dxfer_len = raw->buf_len;
    hdr.dxferp = raw->buf;
    hdr.cmdp = raw->cdb;
    hdr.sbp = raw->sense;
    hdr.timeout = raw->tmo * 1000;
    hdr.flags = SG_FLAG_LUN_INHIBIT;
    /* do raw command */
    if(ioctl(dev->handle, SG_IO, &hdr) < 0)
    {
        raw->status = errno;
        rb_printf("rb_scsi: ioctl failed: %s\n", strerror(raw->status));
        return RB_SCSI_OS_ERROR;
    }
    /* error handling is weird: host status has the priority */
    if(hdr.host_status)
    {
        rb_printf("rb_scsi: host error: %d\n", hdr.host_status);
        raw->status = hdr.host_status;
        return RB_SCSI_ERROR;
    }
    raw->status = hdr.status & 0x7e;
    raw->buf_len -= hdr.resid;
    raw->sense_len = hdr.sb_len_wr;
    /* driver error can report sense */
    if((hdr.driver_status & 0xf) == DRIVER_SENSE)
    {
        rb_printf("rb_scsi: driver status reported sense\n");
        return RB_SCSI_SENSE;
    }
    /* otherwise errors */
    if(hdr.driver_status)
    {
        rb_printf("rb_scsi: driver error: %d\n", hdr.driver_status);
        return RB_SCSI_ERROR;
    }
    /* scsi status can imply sense */
    if(raw->status == SAM_STAT_CHECK_CONDITION || raw->status == SAM_STAT_COMMAND_TERMINATED)
        return RB_SCSI_SENSE;
    return raw->status ? RB_SCSI_STATUS : RB_SCSI_OK;
#undef rb_printf
}

void rb_scsi_close(rb_scsi_device_t dev)
{
    close(dev->handle);
    free(dev);
}

#else /* other targets */
rb_scsi_device_t rb_scsi_open(const char *path, unsigned flags, void *user,
    rb_scsi_printf_t printf)
{
    if(printf == NULL)
        printf = misc_std_printf;
    (void) path;
    if(flags & RB_SCSI_DEBUG)
        printf(user, "rb_scsi: unimplemented on this platform\n");
    return NULL;
}

void rb_scsi_close(rb_scsi_device_t dev)
{
    free(dev);
}
#endif
