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
#ifndef __RB_SCSI_H__
#define __RB_SCSI_H__

#ifdef __cplusplus
extern "C" {
#endif

struct rb_scsi_device_t;
typedef struct rb_scsi_device_t *rb_scsi_device_t;

typedef void (*rb_scsi_printf_t)(void *user, const char *fmt, ...);

/* flags for rb_scsi_open */
#define RB_SCSI_READ_ONLY   (1 << 0)
#define RB_SCSI_DEBUG       (1 << 1)

/* transfer direction */
#define RB_SCSI_NONE        0
#define RB_SCSI_READ        1
#define RB_SCSI_WRITE       2

/* most common status */
#define RB_SCSI_GOOD                0
#define RB_SCSI_CHECK_CONDITION     2
#define RB_SCSI_COMMAND_TERMINATED  0x22

/* return codes */
#define RB_SCSI_OK          0 /* Everything worked */
#define RB_SCSI_STATUS      1 /* Device returned an error in status */
#define RB_SCSI_SENSE       2 /* Device returned sense data */
#define RB_SCSI_OS_ERROR    3 /* Transfer failed, got OS error */
#define RB_SCSI_ERROR       4 /* Transfer failed, got transfer/host error */

/* structure for raw transfers */
struct rb_scsi_raw_cmd_t
{
    int dir; /* direction: none, read or write */
    int cdb_len; /* command buffer length */
    void *cdb; /* command buffer */
    int buf_len; /* data buffer length (will be overwritten with actual count) */
    void *buf; /* buffer */
    int sense_len; /* sense buffer length (will be overwritten with actual count) */
    void *sense; /* sense buffer */
    int tmo; /* timeout (in seconds) */
    int status; /* status returned by device (STATUS) or errno (OS_ERROR) or other error (ERROR) */
};

/* open a device, returns a handle or NULL on error
 * the caller can optionally provide an error printing function */
rb_scsi_device_t rb_scsi_open(const char *path, unsigned flags, void *user,
    rb_scsi_printf_t printf);
/* performs a raw transfer, returns !=0 on error */
int rb_scsi_raw_xfer(rb_scsi_device_t dev, struct rb_scsi_raw_cmd_t *raw);
/* close a device */
void rb_scsi_close(rb_scsi_device_t dev);

#ifdef __cplusplus
}
#endif

#endif /* __RB_SCSI_H__ */