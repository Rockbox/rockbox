/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef __SD_H__
#define __SD_H__

#include <stdbool.h>
#include "mv.h" /* for HAVE_MULTIVOLUME or not */

struct storage_info;

void sd_enable(bool on);
void sd_spindown(int seconds);
void sd_sleep(void);
bool sd_disk_is_active(void);
int sd_soft_reset(void);
int sd_init(void);
void sd_close(void);
int sd_read_sectors(IF_MV2(int drive,) unsigned long start, int count, void* buf);
int sd_write_sectors(IF_MV2(int drive,) unsigned long start, int count, const void* buf);
void sd_spin(void);

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MV2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MV_NONVOID(int drive));
bool sd_present(IF_MV_NONVOID(int drive));
#endif

long sd_last_disk_activity(void);

/* SD States */
#define SD_IDLE             0
#define SD_READY            1
#define SD_IDENT            2
#define SD_STBY             3
#define SD_TRAN             4
#define SD_DATA             5
#define SD_RCV              6
#define SD_PRG              7
#define SD_DIS              8

/* SD Commands */
#define SD_GO_IDLE_STATE         0
#define SD_ALL_SEND_CID          2
#define SD_SEND_RELATIVE_ADDR    3
#define SD_SET_DSR               4
#define SD_SWITCH_FUNC           6
#define SD_SELECT_CARD           7
#define SD_DESELECT_CARD         7
#define SD_SEND_IF_COND          8
#define SD_SEND_CSD              9
#define SD_SEND_CID             10
#define SD_STOP_TRANSMISSION    12
#define SD_SEND_STATUS          13
#define SD_GO_INACTIVE_STATE    15
#define SD_SET_BLOCKLEN         16
#define SD_READ_SINGLE_BLOCK    17
#define SD_READ_MULTIPLE_BLOCK  18
#define SD_SEND_NUM_WR_BLOCKS   22
#define SD_WRITE_BLOCK          24
#define SD_WRITE_MULTIPLE_BLOCK 25
#define SD_ERASE_WR_BLK_START   32
#define SD_ERASE_WR_BLK_END     33
#define SD_ERASE                38
#define SD_APP_CMD              55

/* Application Specific commands */
#define SD_SET_BUS_WIDTH    6
#define SD_APP_OP_COND      41

#endif
