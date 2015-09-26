/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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
#ifndef __NOR_TARGET_H__
#define __NOR_TARGET_H__

void bootflash_init(int port);
void bootflash_read(int port, uint32_t addr, uint32_t size, void* buf);
void bootflash_write(int port, int offset, void* addr, int size);
int bootflash_compare(int port, int offset, void* addr, int size);
void bootflash_erase_blocks(int port, int first, int n);
void bootflash_close(int port);

unsigned bootflash_fs_tail(int port);

#endif /* __NOR_TARGET_H__ */
