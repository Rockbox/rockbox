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
#include <ntddscsi.h>
#include <stdint.h>
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

/* Linux */
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
    if(raw->status == RB_SCSI_CHECK_CONDITION || raw->status == RB_SCSI_COMMAND_TERMINATED)
        return RB_SCSI_SENSE;
    return raw->status ? RB_SCSI_STATUS : RB_SCSI_OK;
#undef rb_printf
}

void rb_scsi_close(rb_scsi_device_t dev)
{
    close(dev->handle);
    free(dev);
}

/* Windpws */
#elif defined(RB_SCSI_WINDOWS)
/* return either path or something allocated with malloc() */
static const char *map_to_physical_drive(const char *path, unsigned flags, void *user,
    rb_scsi_printf_t printf)
{
    /* don't do anything if path starts with '\' */
    if(path[0] == '\\')
        return path;
    /* Convert to UNC path (C: -> \\.\C:) otherwise it won't work) */
    char *unc_path = malloc(strlen(path) + 5);
    sprintf(unc_path, "\\\\.\\%s", path);
    if(flags & RB_SCSI_DEBUG)
        printf(user, "rb_scsi: map to UNC path: %s\n", unc_path);
    return unc_path;
}

rb_scsi_device_t rb_scsi_open(const char *path, unsigned flags, void *user,
    rb_scsi_printf_t printf)
{
    if(printf == NULL)
        printf = misc_std_printf;
    /* magic to auto-detect physical drive */
    const char *open_path = map_to_physical_drive(path, flags, user, printf);
    HANDLE h = CreateFileA(open_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
    /* free path if it was allocated */
    if(open_path != path)
        free((char *)open_path);
    if(h == INVALID_HANDLE_VALUE)
    {
        if(flags & RB_SCSI_DEBUG)
        {
            LPSTR msg = NULL;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
            printf(user, "rb_scsi: open failed: %s\n", msg);
            LocalFree(msg);
        }
        return NULL;
    }
    rb_scsi_device_t dev = malloc(sizeof(struct rb_scsi_device_t));
    dev->flags = flags;
    dev->handle = h;
    dev->user = user;
    dev->printf = printf ? printf : misc_std_printf;
    return dev;
}

int rb_scsi_raw_xfer(rb_scsi_device_t dev, struct rb_scsi_raw_cmd_t *raw)
{
#define rb_printf(...) \
    do{ if(dev->flags & RB_SCSI_DEBUG) dev->printf(dev->user, __VA_ARGS__); }while(0)
    /* we need to allocate memory for SCSI_PASS_THROUGH_WITH_BUFFERS and the
     * data and sense */
    DWORD length = sizeof(SCSI_PASS_THROUGH) + raw->buf_len + raw->sense_len;
    DWORD returned = 0;
    SCSI_PASS_THROUGH *spt = malloc(length);
    memset(spt, 0, length);
    spt->Length = sizeof(SCSI_PASS_THROUGH);
    spt->PathId = 0;
    spt->TargetId = 0;
    spt->Lun = 0;
    spt->CdbLength = raw->cdb_len;
    memcpy(spt->Cdb, raw->cdb, raw->cdb_len);
    spt->SenseInfoLength = raw->sense_len;
    spt->SenseInfoOffset = spt->Length; /* Sense after header */
    switch(raw->dir)
    {
        case RB_SCSI_NONE: spt->DataIn = SCSI_IOCTL_DATA_UNSPECIFIED; break;
        case RB_SCSI_READ: spt->DataIn = SCSI_IOCTL_DATA_IN; break;
        case RB_SCSI_WRITE: spt->DataIn = SCSI_IOCTL_DATA_OUT; break;
        default:
            free(spt);
            rb_printf("rb_scsi: invalid direction"); return -1;
    }
    spt->DataTransferLength = raw->buf_len;
    spt->DataBufferOffset = spt->SenseInfoOffset + spt->SenseInfoLength; /* Data after Sense */
    spt->TimeOutValue = raw->tmo;
    /* on write, copy data to buffer */
    if(raw->dir == RB_SCSI_WRITE)
        memcpy((BYTE *)spt + spt->DataBufferOffset, raw->buf, raw->buf_len);
    BOOL status = DeviceIoControl(dev->handle, IOCTL_SCSI_PASS_THROUGH,
        spt, length, spt, length, &returned, FALSE);
    if(!status)
    {
        if(dev->flags & RB_SCSI_DEBUG)
        {
            LPSTR msg = NULL;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
            printf(dev->user, "rb_scsi: ioctl failed: %s\n", msg);
            LocalFree(msg);
        }
        free(spt);
        return RB_SCSI_OS_ERROR;
    }
    /* on read, copy data from buffer */
    if(raw->dir == RB_SCSI_READ)
    {
        raw->buf_len = spt->DataTransferLength;
        memcpy(raw->buf, (BYTE *)spt + spt->DataBufferOffset, raw->buf_len);
    }
    /* check status */
    raw->status = spt->ScsiStatus;
    if(raw->status == RB_SCSI_CHECK_CONDITION || raw->status == RB_SCSI_COMMAND_TERMINATED)
    {
        memcpy(raw->sense, (BYTE *)spt + spt->SenseInfoOffset, raw->sense_len);
        free(spt);
        return RB_SCSI_SENSE;
    }
    else
        raw->sense_len = 0;
    free(spt);
    return raw->status ? RB_SCSI_STATUS : RB_SCSI_OK;
#undef rb_printf
}

void rb_scsi_close(rb_scsi_device_t dev)
{
    CloseHandle(dev->handle);
    free(dev);
}

/* other targets */
#else
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

int rb_scsi_raw_xfer(rb_scsi_device_t dev, struct rb_scsi_raw_cmd_t *raw)
{
    return RB_SCSI_ERROR;
}

void rb_scsi_close(rb_scsi_device_t dev)
{
    free(dev);
}
#endif

void rb_scsi_decode_sense(rb_scsi_device_t dev, void *_sense, int sense_len)
{
#define rb_printf(...) \
    do{ if(dev->flags & RB_SCSI_DEBUG) dev->printf(dev->user, __VA_ARGS__); }while(0)

    uint8_t *sense = _sense;
    uint8_t type = sense[0] & 0x7f;
    switch(type)
    {
        case 0x70: case 0x73: rb_printf("Current sense: "); break;
        case 0x71: case 0x72: rb_printf("Previous sense: "); break;
        default: rb_printf("Unknown sense\n"); return;
    }
    unsigned key = sense[2] & 0xf;
    switch(key)
    {
        case 0: rb_printf("no sense"); break;
        case 1: rb_printf("recovered error"); break;
        case 2: rb_printf("not ready"); break;
        case 3: rb_printf("medium error"); break;
        case 4: rb_printf("hardware error"); break;
        case 5: rb_printf("illegal request"); break;
        case 6: rb_printf("unit attention"); break;
        case 7: rb_printf("data protect"); break;
        case 8: rb_printf("blank check"); break;
        case 9: rb_printf("vendor specific"); break;
        case 10: rb_printf("copy aborted"); break;
        case 11: rb_printf("aborted command"); break;
        default: rb_printf("unknown key"); break;
    }
    rb_printf("\n");

#undef rb_printf
}
