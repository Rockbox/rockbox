/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mktccboot.h - a tool to inject a bootloader into a Telechips 77X/78X firmware
 *               file.
 *
 * Copyright (C) 2009 Tomer Shalev
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

#ifndef _MKTCCBOOT_H_
#define _MKTCCBOOT_H_

/* win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0

#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Injects a bootloader into a Telechips 77X/78X firmware file */
unsigned char *patch_firmware_tcc(unsigned char *of_buf, int of_size,
        unsigned char *boot_buf, int boot_size, int *patched_size);

unsigned char *file_read(char *filename, int *size);

/* Test TCC firmware file for consistency using CRC test */
int test_firmware_tcc(unsigned char* buf, int length);

#ifdef __cplusplus
};
#endif

#endif
