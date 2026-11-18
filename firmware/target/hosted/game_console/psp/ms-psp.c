/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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
/* Raw Memory Stick storage driver: Rockbox's own FAT driver, file layer and
 * dircache run on the whole card, msstor0:, which user mode can open, read
 * and write.
 *
 * Exposed as the STORAGE_MS storage type: a single drive with plain
 * sector reads and writes and none of the SD/MMC specifics.
 *
 * Sony's driver (fatms) caches the same filesystem through ms0:, so it must
 * not use the card while Rockbox does: ms0: is detached at init and only
 * reattached briefly for module loads and at exit (lib/sys_storage_psp.c). */

#include <string.h>
#include <pspiofilemgr.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "storage.h"
#include "debug.h"
#include "sys_storage_psp.h"

#define MSSTOR_DEVICE       "msstor0:"
#define SECTOR_SIZE         512
#define WRITE_MAX_SECTORS   128     /* msstor shortens larger writes */
#define BOUNCE_SECTORS      64      /* 32 KB */

static SceUID raw_fd = -1;
static struct mutex raw_mtx;
static long last_activity;

/* the driver needs 64-byte aligned buffers */
static unsigned char bounce[BOUNCE_SECTORS * SECTOR_SIZE]
    __attribute__((aligned(64)));

int ms_read_sectors(IF_MD(int drive,) sector_t start, int count, void *buf)
{
    IF_MD((void)drive;)
    int rc = 0;
    SceOff off = (SceOff)start * SECTOR_SIZE;

    mutex_lock(&raw_mtx);
    last_activity = current_tick;

    if (sceIoLseek(raw_fd, off, PSP_SEEK_SET) != off)
        rc = -1;
    else if (((uintptr_t)buf & 63) == 0) {
        int len = count * SECTOR_SIZE;
        if (sceIoRead(raw_fd, buf, len) != len)
            rc = -2;
    }
    else {
        /* unaligned destination: go through the bounce buffer */
        while (count > 0) {
            int n = MIN(count, BOUNCE_SECTORS);
            int len = n * SECTOR_SIZE;
            if (sceIoRead(raw_fd, bounce, len) != len) {
                rc = -2;
                break;
            }
            memcpy(buf, bounce, len);
            buf = (char *)buf + len;
            count -= n;
        }
    }

    mutex_unlock(&raw_mtx);
    return rc;
}

int ms_write_sectors(IF_MD(int drive,) sector_t start, int count,
                          const void *buf)
{
    IF_MD((void)drive;)
    int rc = 0;
    SceOff off = (SceOff)start * SECTOR_SIZE;

    mutex_lock(&raw_mtx);
    last_activity = current_tick;

    if (sceIoLseek(raw_fd, off, PSP_SEEK_SET) != off)
        rc = -1;

    while (rc == 0 && count > 0) {
        bool unaligned = ((uintptr_t)buf & 63) != 0;
        int n = MIN(count, unaligned ? BOUNCE_SECTORS : WRITE_MAX_SECTORS);
        int len = n * SECTOR_SIZE;
        const void *src = buf;
        if (unaligned) {
            memcpy(bounce, buf, len);
            src = bounce;
        }
        if (sceIoWrite(raw_fd, src, len) != len)
            rc = -2;
        buf = (const char *)buf + len;
        count -= n;
    }

    mutex_unlock(&raw_mtx);
    return rc;
}

int ms_init(void)
{
    mutex_init(&raw_mtx);
    raw_fd = sceIoOpen(MSSTOR_DEVICE, PSP_O_RDWR, 0);
    if (raw_fd < 0) {
        DEBUGF("%s open failed 0x%x\n", MSSTOR_DEVICE, raw_fd);
        return -1;
    }
    sys_storage_detach();       /* Rockbox is now the only user of the card */
    return 0;
}

void ms_close(void)
{
    if (raw_fd >= 0)
        sceIoClose(raw_fd);
    raw_fd = -1;
}
#ifdef STORAGE_GET_INFO
void ms_get_info(IF_MD(int drive,) struct storage_info *info)
{
    IF_MD((void)drive;)
    SceOff end = raw_fd >= 0 ? sceIoLseek(raw_fd, 0, PSP_SEEK_END) : 0;
    info->sector_size = SECTOR_SIZE;
    info->num_sectors = end / SECTOR_SIZE;
    info->vendor = "Sony";
    info->product = "Memory Stick";
    info->revision = "msstor0";
}
#endif

void ms_enable(bool on) { (void)on; }
void ms_spindown(int seconds) { (void)seconds; }
bool ms_disk_is_active(void) { return false; }
int ms_soft_reset(void) { return 0; }
void ms_spin(void) {}
void ms_sleepnow(void) {}
int ms_spinup_time(void) { return 0; }
long ms_last_disk_activity(void) { return last_activity; }

int ms_event(long id, intptr_t data)
{
    return storage_event_default_handler(id, data, last_activity,
                                         STORAGE_MS);
}

#ifdef HAVE_HOTSWAP
bool ms_removable(IF_MD_NONVOID(int drive)) { IF_MD((void)drive;) return false; }
bool ms_present(IF_MD_NONVOID(int drive)) { IF_MD((void)drive;) return true; }
#endif
