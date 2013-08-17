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
#ifndef __MMC_H__
#define __MMC_H__

#include <stdbool.h>
#include "config.h"
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

struct storage_info;

void mmc_enable(bool on);
void mmc_spindown(int seconds);
void mmc_sleep(void);
void mmc_sleepnow(void);
bool mmc_disk_is_active(void);
int mmc_soft_reset(void);
int mmc_init(void) STORAGE_INIT_ATTR;
void mmc_close(void);
int mmc_read_sectors(IF_MD(int drive,) unsigned long start, int count, void* buf);
int mmc_write_sectors(IF_MD(int drive,) unsigned long start, int count, const void* buf);
void mmc_spin(void);
int mmc_spinup_time(void);

#ifdef STORAGE_GET_INFO
void mmc_get_info(IF_MD(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool mmc_removable(IF_MD_NONVOID(int drive));
bool mmc_present(IF_MD_NONVOID(int drive));
#endif

long mmc_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int mmc_num_drives(int first_drive);
#endif

/* MMC States */
#define MMC_IDLE            0
#define MMC_READY           1
#define MMC_IDENT           2
#define MMC_STBY            3
#define MMC_TRAN            4
#define MMC_DATA            5
#define MMC_RCV             6
#define MMC_PRG             7
#define MMC_DIS             8
#define MMC_BTST            9

/* MMC Commands */
#define MMC_GO_IDLE_STATE           0
#define MMC_SEND_OP_COND            1
#define MMC_ALL_SEND_CID            2
#define MMC_SET_RELATIVE_ADDR       3
#define MMC_SET_DSR                 4
#define MMC_SWITCH                  6
#define MMC_SELECT_CARD             7  /* with card's rca  */
#define MMC_DESELECT_CARD           7  /* with rca = 0  */
#define MMC_SEND_EXT_CSD            8
#define MMC_SEND_CSD                9
#define MMC_SEND_CID                10
#define MMC_READ_DAT_UNTIL_STOP     11
#define MMC_STOP_TRANSMISSION       12
#define MMC_SEND_STATUS             13
#define MMC_BUSTEST_R               14
#define MMC_GO_INACTIVE_STATE       15
#define MMC_SET_BLOCKLEN            16
#define MMC_READ_SINGLE_BLOCK       17
#define MMC_READ_MULTIPLE_BLOCK     18
#define MMC_BUSTEST_W               19
#define MMC_WRITE_DAT_UNTIL_STOP    20
#define MMC_SET_BLOCK_COUNT         23
#define MMC_WRITE_BLOCK             24
#define MMC_WRITE_MULTIPLE_BLOCK    25
#define MMC_PROGRAM_CID             26
#define MMC_PROGRAM_CSD             27
#define MMC_SET_WRITE_PROT          28
#define MMC_CLR_WRITE_PROT          29
#define MMC_SEND_WRITE_PROT         30
#define MMC_ERASE_GROUP_START       35
#define MMC_ERASE_GROUP_END         36
#define MMC_ERASE                   38
#define MMC_FAST_IO                 39
#define MMC_GO_IRQ_STATE            40
#define MMC_LOCK_UNLOCK             42
#define MMC_APP_CMD                 55
#define MMC_GEN_CMD                 56

#endif
