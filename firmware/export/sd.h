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
#include "mv.h" /* for HAVE_MULTIDRIVE or not */

#define SD_BLOCK_SIZE 512 /* XXX : support other sizes ? */

struct storage_info;

void sd_enable(bool on);
void sd_spindown(int seconds);
void sd_sleep(void);
void sd_sleepnow(void);
bool sd_disk_is_active(void);
int  sd_soft_reset(void);
int  sd_init(void);
void sd_close(void);
int  sd_read_sectors(IF_MD2(int drive,) unsigned long start, int count, void* buf);
int  sd_write_sectors(IF_MD2(int drive,) unsigned long start, int count, const void* buf);
void sd_spin(void);
int  sd_spinup_time(void); /* ticks */

#ifdef STORAGE_GET_INFO
void sd_get_info(IF_MD2(int drive,) struct storage_info *info);
#endif
#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MV_NONVOID(int drive));
bool sd_present(IF_MV_NONVOID(int drive));
void card_enable_monitoring_target(bool on);
#endif

bool card_detect_target(void);

long sd_last_disk_activity(void);

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive);
#endif


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
#define SD_GO_IDLE_STATE           0
#define SD_ALL_SEND_CID            2
#define SD_SEND_RELATIVE_ADDR      3
#define SD_SET_DSR                 4
#define SD_SWITCH_FUNC             6
#define SD_SET_BUS_WIDTH           6  /* acmd6 */
#define SD_SELECT_CARD             7  /* with card's rca  */
#define SD_DESELECT_CARD           7  /* with rca = 0  */
#define SD_SEND_IF_COND            8
#define SD_SEND_CSD                9
#define SD_SEND_CID               10
#define SD_STOP_TRANSMISSION      12
#define SD_SEND_STATUS            13
#define SD_SD_STATUS              13  /* acmd13 */
#define SD_GO_INACTIVE_STATE      15
#define SD_SET_BLOCKLEN           16
#define SD_READ_SINGLE_BLOCK      17
#define SD_READ_MULTIPLE_BLOCK    18
#define SD_SEND_NUM_WR_BLOCKS     22  /* acmd22 */
#define SD_SET_WR_BLK_ERASE_COUNT 23  /* acmd23 */
#define SD_WRITE_BLOCK            24
#define SD_WRITE_MULTIPLE_BLOCK   25
#define SD_PROGRAM_CSD            27
#define SD_ERASE_WR_BLK_START     32
#define SD_ERASE_WR_BLK_END       33
#define SD_ERASE                  38
#define SD_APP_OP_COND            41  /* acmd41 */
#define SD_LOCK_UNLOCK            42
#define SD_SET_CLR_CARD_DETECT    42  /* acmd42 */
#define SD_SEND_SCR               51  /* acmd51 */
#define SD_APP_CMD                55

/*
  SD/MMC status in R1, for native mode (SPI bits are different)
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
    c : clear by read
 */

#define SD_R1_OUT_OF_RANGE         (1 << 31)   /* erx, c */
#define SD_R1_ADDRESS_ERROR        (1 << 30)   /* erx, c */
#define SD_R1_BLOCK_LEN_ERROR      (1 << 29)   /* erx, c */
#define SD_R1_ERASE_SEQ_ERROR      (1 << 28)   /* er,  c */
#define SD_R1_ERASE_PARAM          (1 << 27)   /* exx, c */
#define SD_R1_WP_VIOLATION         (1 << 26)   /* erx, c */
#define SD_R1_CARD_IS_LOCKED       (1 << 25)   /* sx,  a */
#define SD_R1_LOCK_UNLOCK_FAILED   (1 << 24)   /* erx, c */
#define SD_R1_COM_CRC_ERROR        (1 << 23)   /* er,  b */
#define SD_R1_ILLEGAL_COMMAND      (1 << 22)   /* er,  b */
#define SD_R1_CARD_ECC_FAILED      (1 << 21)   /* erx, c */
#define SD_R1_CC_ERROR             (1 << 20)   /* erx, c */
#define SD_R1_ERROR                (1 << 19)   /* erx, c */
#define SD_R1_UNDERRUN             (1 << 18)   /* ex,  c */
#define SD_R1_OVERRUN              (1 << 17)   /* ex,  c */
#define SD_R1_CSD_OVERWRITE        (1 << 16)   /* erx, c */
#define SD_R1_WP_ERASE_SKIP        (1 << 15)   /* erx, c */
#define SD_R1_CARD_ECC_DISABLED    (1 << 14)   /* sx,  a */
#define SD_R1_ERASE_RESET          (1 << 13)   /* sr,  c */
#define SD_R1_STATUS(x)            (x & 0xFFFFE000)
#define SD_R1_CURRENT_STATE(x)     ((x & 0x00001E00) >> 9) /* sx, b (4 bits) */
#define SD_R1_READY_FOR_DATA       (1 << 8)    /* sx,  a */
#define SD_R1_APP_CMD              (1 << 5)    /* sr,  c */
#define SD_R1_AKE_SEQ_ERROR        (1 << 3)    /* er,  c */

/* SD OCR bits */
#define SD_OCR_CARD_CAPACITY_STATUS    (1 << 30)   /* Card Capacity Status */

/*  All R1 Response flags that indicate Card error(vs MCI Controller error) */
#define SD_R1_CARD_ERROR           ( SD_R1_OUT_OF_RANGE           \
                                   | SD_R1_ADDRESS_ERROR          \
                                   | SD_R1_BLOCK_LEN_ERROR        \
                                   | SD_R1_ERASE_SEQ_ERROR        \
                                   | SD_R1_ERASE_PARAM            \
                                   | SD_R1_WP_VIOLATION           \
                                   | SD_R1_LOCK_UNLOCK_FAILED     \
                                   | SD_R1_COM_CRC_ERROR          \
                                   | SD_R1_ILLEGAL_COMMAND        \
                                   | SD_R1_CARD_ECC_FAILED        \
                                   | SD_R1_CC_ERROR               \
                                   | SD_R1_ERROR                  \
                                   | SD_R1_UNDERRUN               \
                                   | SD_R1_CSD_OVERWRITE          \
                                   | SD_R1_WP_ERASE_SKIP          \
                                   | SD_R1_AKE_SEQ_ERROR)


#endif
