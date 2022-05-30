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
#include "screendump.h"
#include "nand-x1000.h"
#include "sfc-x1000.h"

/* Set to true if a SYS_USB_CONNECTED event is seen
 * Set to false if a SYS_USB_DISCONNECTED event is seen
 * Handled by the gui code since that's how events are delivered
 * TODO: this is an ugly kludge */
bool is_usb_connected = false;

static bool screenshot_enabled = false;

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

void screenshot(void)
{
#ifdef HAVE_SCREENDUMP
    if(!screenshot_enabled || check_disk(false) != DISK_PRESENT)
        return;

    screen_dump();
#endif
}

void screenshot_enable(void)
{
#ifdef HAVE_SCREENDUMP
    splashf(3*HZ, "Screenshots enabled\nPress %s for screenshot",
            BL_SCREENSHOT_NAME);
    screenshot_enabled = true;
#endif
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
        splashf(5*HZ, "Cannot load uImage (%d)\n%s", handle, filename);
        return -3;
    }

    return handle;
}

struct nand_reader_data
{
    nand_drv* ndrv;
    nand_page_t page;
    nand_page_t end_page;
    unsigned offset;
    uint32_t count;
};

static int uimage_nand_reader_init(struct nand_reader_data* d, nand_drv* ndrv,
                                   uint32_t addr, uint32_t length)
{
    unsigned pg_size = ndrv->chip->page_size;

    /* must start at a block address */
    if(addr % (pg_size << ndrv->chip->log2_ppb))
        return -1;

    d->ndrv = ndrv;
    d->page = addr / ndrv->chip->page_size;
    d->end_page = d->page + (length + pg_size - 1) / pg_size;
    d->offset = 0;
    d->count = length;

    if(d->end_page > ndrv->chip->nr_blocks << ndrv->chip->log2_ppb)
        return -2;

    return 0;
}

static ssize_t uimage_nand_reader(void* buf, size_t count, void* rctx)
{
    struct nand_reader_data* d = rctx;
    nand_drv* ndrv = d->ndrv;
    unsigned pg_size = ndrv->chip->page_size;
    size_t read_count = 0;
    int rc;

    /* truncate overlong reads */
    if(count > d->count)
        count = d->count;

    while(d->page < d->end_page && read_count < count) {
        rc = nand_page_read(ndrv, d->page, ndrv->page_buf);
        if(rc < 0)
            return -1;

        /* Check the first page of a block for the bad block marker.
         * Any bad blocks are silently skipped. */
        if(!(d->page & (ndrv->ppb - 1))) {
            if(ndrv->page_buf[ndrv->chip->bbm_pos] != 0xff) {
                if(d->offset != 0)
                    return -1; /* shouldn't happen but just in case... */
                d->page += ndrv->ppb;
                continue;
            }
        }

        size_t copy_len = MIN(count - read_count, pg_size - d->offset);
        memcpy(buf, &ndrv->page_buf[d->offset], copy_len);

        /* this seems like an excessive amount of arithmetic... */
        buf += copy_len;
        read_count += copy_len;
        d->count -= copy_len;

        d->offset += copy_len;
        if(d->offset >= pg_size) {
            d->offset -= pg_size;
            d->page++;
        }
    }

    return read_count;
}

int load_uimage_flash(uint32_t addr, uint32_t length,
                      struct uimage_header* uh, size_t* sizep)
{
    nand_drv* ndrv = nand_init();
    nand_lock(ndrv);
    if(nand_open(ndrv) != NAND_SUCCESS) {
        splashf(5*HZ, "NAND open failed");
        nand_unlock(ndrv);
        return -1;
    }

    struct nand_reader_data n;
    int ret = uimage_nand_reader_init(&n, ndrv, addr, length);
    if(ret != 0) {
        splashf(5*HZ, "Bad image params\nAddr: %08lx\nLength: %lu", addr, length);
        ret = -2;
        goto out;
    }

    int handle = uimage_load(uh, sizep, uimage_nand_reader, &n);
    if(handle <= 0) {
        splashf(5*HZ, "uImage load failed (%d)", handle);
        ret = -3;
        goto out;
    }

    ret = handle;

  out:
    nand_close(ndrv);
    nand_unlock(ndrv);
    return ret;
}

int dump_flash(int fd, uint32_t addr, uint32_t length)
{
    static char buf[8192];
    int ret = 0;

    nand_drv* ndrv = nand_init();
    nand_lock(ndrv);

    ret = nand_open(ndrv);
    if(ret != NAND_SUCCESS) {
        splashf(5*HZ, "NAND open failed\n");
        nand_unlock(ndrv);
        return ret;
    }

    while(length > 0) {
        uint32_t count = MIN(length, sizeof(buf));
        ret = nand_read_bytes(ndrv, addr, count, buf);
        if(ret != NAND_SUCCESS) {
            splashf(5*HZ, "Dump failed\nNAND I/O error");
            goto out;
        }

        if(write(fd, buf, count) != (ssize_t)count) {
            splashf(5*HZ, "Dump failed\nFile I/O error");
            ret = -1;
            goto out;
        }

        length -= count;
        addr += count;
    }

  out:
    nand_close(ndrv);
    nand_unlock(ndrv);
    return ret;
}

int dump_flash_file(const char* file, uint32_t addr, uint32_t length)
{
    if(check_disk(true) != DISK_PRESENT)
        return -1;

    splashf(0, "Dumping...\n%s\n0x%08lx\n%lu bytes", file, addr, length);

    int fd = open(file, O_WRONLY|O_CREAT|O_TRUNC);
    if(fd < 0) {
        splashf(5*HZ, "Cannot open file\n%s", file);
        return -2;
    }

    int rc = dump_flash(fd, addr, length);
    if(rc < 0) {
        close(fd);
        remove(file);
        return -3;
    }

    splashf(5*HZ, "Dumped\n%s", file);
    close(fd);
    return 0;
}

void dump_of_player(void)
{
#ifdef OF_PLAYER_ADDR
    dump_flash_file("/of_player.img", OF_PLAYER_ADDR, OF_PLAYER_LENGTH);
#endif
}

void dump_of_recovery(void)
{
#ifdef OF_RECOVERY_ADDR
    dump_flash_file("/of_recovery.img", OF_RECOVERY_ADDR, OF_RECOVERY_LENGTH);
#endif
}

void dump_entire_flash(void)
{
#if defined(FIIO_M3K) || defined(SHANLING_Q1) || defined(EROS_QN)
    /* TODO: this should read the real chip size instead of hardcoding it */
    dump_flash_file("/flash.img", 0, 2048 * 64 * 1024);
#endif
}

static void probe_flash(int log_fd)
{
    static uint8_t buffer[CACHEALIGN_UP(32)] CACHEALIGN_ATTR;

    /* Use parameters from maskrom */
    const uint32_t clock_freq = X1000_EXCLK_FREQ; /* a guess */
    const uint32_t dev_conf = jz_orf(SFC_DEV_CONF,
                                     CE_DL(1), HOLD_DL(1), WP_DL(1),
                                     CPHA(0), CPOL(0),
                                     TSH(0), TSETUP(0), THOLD(0),
                                     STA_TYPE_V(1BYTE), CMD_TYPE_V(8BITS),
                                     SMP_DELAY(0));
    const size_t readid_len = 4;

    /* NOTE: This assumes the NAND driver is inactive. If this is not true,
     * this will seriously mess up the NAND driver. */
    sfc_open();
    sfc_set_dev_conf(dev_conf);
    sfc_set_clock(clock_freq);

    /* Issue reset */
    sfc_exec(NANDCMD_RESET, 0, NULL, 0);
    mdelay(10);

    /* Try various read ID commands (cf. Linux's SPI NAND identify routine) */
    sfc_exec(NANDCMD_READID(0, 0), 0, buffer, readid_len|SFC_READ);
    fdprintf(log_fd, "readID opcode  = %02x %02x %02x %02x\n",
             buffer[0], buffer[1], buffer[2], buffer[3]);

    sfc_exec(NANDCMD_READID(1, 0), 0, buffer, readid_len|SFC_READ);
    fdprintf(log_fd, "readID address = %02x %02x %02x %02x\n",
             buffer[0], buffer[1], buffer[2], buffer[3]);

    sfc_exec(NANDCMD_READID(0, 8), 0, buffer, readid_len|SFC_READ);
    fdprintf(log_fd, "readID dummy   = %02x %02x %02x %02x\n",
             buffer[0], buffer[1], buffer[2], buffer[3]);

    /* Try reading Ingenic SFC boot block */
    sfc_exec(NANDCMD_PAGE_READ(3), 0, NULL, 0);
    mdelay(500);
    sfc_exec(NANDCMD_READ_CACHE_SLOW(2), 0, buffer, 16|SFC_READ);

    fdprintf(log_fd, "sfc params0  = %02x %02x %02x %02x\n",
             buffer[ 0], buffer[ 1], buffer[ 2], buffer[ 3]);
    fdprintf(log_fd, "sfc params1  = %02x %02x %02x %02x\n",
             buffer[ 4], buffer[ 5], buffer[ 6], buffer[ 7]);
    fdprintf(log_fd, "sfc params2  = %02x %02x %02x %02x\n",
             buffer[ 8], buffer[ 9], buffer[10], buffer[11]);
    fdprintf(log_fd, "sfc params3  = %02x %02x %02x %02x\n",
             buffer[12], buffer[13], buffer[14], buffer[15]);

    sfc_close();
}

void show_flash_info(void)
{
    if(check_disk(true) != DISK_PRESENT)
        return;

    int fd = open("/flash_info.txt", O_WRONLY|O_CREAT|O_TRUNC);
    if(fd < 0) {
        splashf(5*HZ, "Cannot create log file");
        return;
    }

    splashf(0, "Probing flash...");
    probe_flash(fd);

    close(fd);
    splashf(3*HZ, "Dumped flash info\nSee flash_info.txt");
}
