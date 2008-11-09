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
#define IDLE            0
#define READY           1
#define IDENT           2
#define STBY            3
#define TRAN            4
#define DATA            5
#define RCV             6
#define PRG             7
#define DIS             8

/* SD Commands */
#define GO_IDLE_STATE         0
#define ALL_SEND_CID          2
#define SEND_RELATIVE_ADDR    3
#define SET_DSR               4
#define SWITCH_FUNC           6
#define SELECT_CARD           7
#define DESELECT_CARD         7
#define SEND_IF_COND          8
#define SEND_CSD              9
#define SEND_CID             10
#define STOP_TRANSMISSION    12
#define SEND_STATUS          13
#define GO_INACTIVE_STATE    15
#define SET_BLOCKLEN         16
#define READ_SINGLE_BLOCK    17
#define READ_MULTIPLE_BLOCK  18
#define SEND_NUM_WR_BLOCKS   22
#define WRITE_BLOCK          24
#define WRITE_MULTIPLE_BLOCK 25
#define ERASE_WR_BLK_START   32
#define ERASE_WR_BLK_END     33
#define ERASE                38
#define APP_CMD              55

/* Application Specific commands */
#define SET_BUS_WIDTH   6
#define SD_APP_OP_COND  41

#endif
