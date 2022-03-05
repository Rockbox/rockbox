/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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
#include "system.h"
#include "kernel.h"
#include "power.h"
#include "file.h"
#include "linuxboot.h"
#include "boot-x1000.h"
#include <ctype.h>
#include <sys/types.h>

void boot_rockbox(void)
{
    size_t length;
    int handle = load_rockbox(BOOTFILE, &length);
    if(handle < 0)
        return;

    gui_shutdown();
    x1000_boot_rockbox(core_get_data(handle), length);
}

void shutdown(void)
{
    splash(HZ, "Shutting down");
    power_off();
    while(1);
}

void reboot(void)
{
    splash(HZ, "Rebooting");
    system_reboot();
    while(1);
}

/*
 * boot_linux() is intended for mainline kernels, and as such it
 * should not need any major target-specific modifications.
 */

static int read_linux_args(const char* filename)
{
    if(check_disk(false) != DISK_PRESENT)
        return -1;

    int ret;

    size_t max_size;
    int handle = core_alloc_maximum("args", &max_size, &buflib_ops_locked);
    if(handle <= 0) {
        splash(5*HZ, "Out of memory");
        return -2;
    }

    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        splash2(5*HZ, "Can't open args file", filename);
        ret = -3;
        goto err_free;
    }

    /* this isn't 100% correct but will be good enough */
    off_t fsize = filesize(fd);
    if(fsize < 0 || fsize+1 > (off_t)max_size) {
        splash(5*HZ, "Arguments too long");
        ret = -4;
        goto err_close;
    }

    char* buf = core_get_data(handle);
    core_shrink(handle, buf, fsize+1);

    ssize_t rdres = read(fd, buf, fsize);
    close(fd);

    if(rdres != (ssize_t)fsize) {
        splash(5*HZ, "Can't read args file");
        ret = -5;
        goto err_free;
    }

    /* append a null terminator */
    char* end = buf + fsize;
    *end = 0;

    /* change all newlines, etc, to spaces */
    for(; buf != end; ++buf)
        if(isspace(*buf))
            *buf = ' ';

    return handle;

  err_close:
    close(fd);
  err_free:
    core_free(handle);
    return ret;
}

/*
 * Provisional linux loading function: kernel is at "/uImage",
 * contents of "/linux_cmdline.txt" are used as kernel arguments.
 */
void boot_linux(void)
{
    struct uimage_header uh;
    size_t img_length;
    int handle = load_uimage_file("/uImage", &uh, &img_length);
    if(handle < 0)
        return;

    int args_handle = read_linux_args("/linux_cmdline.txt");
    if(args_handle < 0) {
        core_free(handle);
        return;
    }

    x1000_boot_linux(core_get_data(handle), img_length,
                     (void*)uimage_get_load(&uh),
                     (void*)uimage_get_ep(&uh),
                     core_get_data(args_handle));
}

/*
 * WARNING: Original firmware can be finicky.
 * Be careful when modifying this code.
 */

static __attribute__((unused))
void boot_of_helper(uint32_t addr, uint32_t flash_size, const char* args)
{
    struct uimage_header uh;
    size_t img_length;
    int handle = load_uimage_flash(addr, flash_size, &uh, &img_length);
    if(handle < 0)
        return;

    gui_shutdown();

    x1000_dualboot_load_pdma_fw();
    x1000_dualboot_cleanup();
    x1000_dualboot_init_clocktree();
    x1000_dualboot_init_uart2();

    x1000_boot_linux(core_get_data(handle), img_length,
                     (void*)uimage_get_load(&uh),
                     (void*)uimage_get_ep(&uh), args);
}

void boot_of_player(void)
{
#if defined(OF_PLAYER_ADDR)
    boot_of_helper(OF_PLAYER_ADDR, OF_PLAYER_LENGTH, OF_PLAYER_ARGS);
#else
    splash(HZ, "Not supported");
#endif
}

void boot_of_recovery(void)
{
#if defined(OF_RECOVERY_ADDR)
    boot_of_helper(OF_RECOVERY_ADDR, OF_RECOVERY_LENGTH, OF_RECOVERY_ARGS);
#else
    splash(HZ, "Not supported");
#endif
}
