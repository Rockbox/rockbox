/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __MSC_X1000_H__
#define __MSC_X1000_H__

#include "kernel.h"
#include "sdmmc.h"
#include <stdbool.h>

/* Number of MSC controllers */
#define MSC_COUNT 2

/* Media types */
#define MSC_TYPE_SD  0
#define MSC_TYPE_MMC 1
#define MSC_TYPE_ATA 2
#define MSC_TYPE_ANY 3

/* Clock modes */
#define MSC_CLK_MANUAL    0
#define MSC_CLK_AUTOMATIC 1

/* Clock status bits */
#define MSC_CLKST_ENABLE (1 << 0)
#define MSC_CLKST_AUTO   (1 << 1)

/* Driver flags */
#define MSC_DF_ERRSTATE (1 << 0)
#define MSC_DF_READY    (1 << 1)
#define MSC_DF_HCS_CARD (1 << 2)
#define MSC_DF_V2_CARD  (1 << 3)

/* Request status codes */
#define MSC_REQ_SUCCESS     0
#define MSC_REQ_CRC_ERR     1
#define MSC_REQ_CARD_ERR    2
#define MSC_REQ_TIMEOUT     3
#define MSC_REQ_EXTRACTED   4
#define MSC_REQ_LOCKUP      5
#define MSC_REQ_ERROR       6
#define MSC_REQ_INCOMPLETE  (-1)

/* Response types */
#define MSC_RESP_NONE   0
#define MSC_RESP_BUSY   (1 << 7)
#define MSC_RESP_R1     1
#define MSC_RESP_R1B    (MSC_RESP_R1|MSC_RESP_BUSY)
#define MSC_RESP_R2     2
#define MSC_RESP_R3     3
#define MSC_RESP_R6     6
#define MSC_RESP_R7     7

/* Request flags */
#define MSC_RF_INIT         (1 << 0)
#define MSC_RF_ERR_CMD12    (1 << 1)
#define MSC_RF_AUTO_CMD12   (1 << 2)
#define MSC_RF_PROG         (1 << 3)
#define MSC_RF_DATA         (1 << 4)
#define MSC_RF_WRITE        (1 << 5)
#define MSC_RF_ABORT        (1 << 6)

/* Clock speeds */
#define MSC_SPEED_INIT  400000
#define MSC_SPEED_FAST  25000000
#define MSC_SPEED_HIGH  50000000

typedef struct msc_config {
    int msc_nr;
    int msc_type;
    int bus_width;
    const char* label;
    int cd_gpio;
    int cd_active_level;
} msc_config;

typedef struct msc_req {
    /* Filled by caller */
    int command;
    unsigned argument;
    int resptype;
    int flags;
    void* data;
    unsigned nr_blocks;
    unsigned block_len;

    /* Filled by driver */
    volatile unsigned response[4];
    volatile int status;
} msc_req;

struct sd_dma_desc {
    unsigned nda;
    unsigned mem;
    unsigned len;
    unsigned cmd;
} __attribute__((aligned(16)));

typedef struct msc_drv {
    int msc_nr;
    int drive_nr;
    const msc_config* config;

    int driver_flags;
    int clk_status;
    unsigned cmdat_def;
    msc_req* req;
    unsigned iflag_done;

    volatile int req_running;
    volatile int card_present; /* Debounced status */
    volatile int card_present_last; /* Status when we last polled it */

    struct mutex lock;
    struct semaphore cmd_done;
    struct timeout cmd_tmo;
    struct timeout cd_tmo;
    struct sd_dma_desc dma_desc;

    tCardInfo cardinfo;
} msc_drv;

/* Driver initialization, etc */
extern void msc_init(void);
extern msc_drv* msc_get(int type, int index);
extern msc_drv* msc_get_by_drive(int drive_nr);

extern void msc_lock(msc_drv* d);
extern void msc_unlock(msc_drv* d);
extern void msc_full_reset(msc_drv* d);
extern bool msc_card_detect(msc_drv* d);

extern void msc_led_trigger(void);

/* Controller API */
extern void msc_ctl_reset(msc_drv* d);
extern void msc_set_clock_mode(msc_drv* d, int mode);
extern void msc_enable_clock(msc_drv* d, bool enable);
extern void msc_set_speed(msc_drv* d, int rate);
extern void msc_set_width(msc_drv* d, int width);

/* Request API */
extern void msc_async_start(msc_drv* d, msc_req* r);
extern void msc_async_abort(msc_drv* d, int status);
extern int  msc_async_wait(msc_drv* d, int timeout);
extern int  msc_request(msc_drv* d, msc_req* r);

/* Command helpers; note these are written with SD in mind
 * and should be reviewed before using them for MMC / CE-ATA
 */
extern int msc_cmd_exec(msc_drv* d, msc_req* r);
extern int msc_app_cmd_exec(msc_drv* d, msc_req* r);
extern int msc_cmd_go_idle_state(msc_drv* d);
extern int msc_cmd_send_if_cond(msc_drv* d);
extern int msc_cmd_app_op_cond(msc_drv* d);
extern int msc_cmd_all_send_cid(msc_drv* d);
extern int msc_cmd_send_rca(msc_drv* d);
extern int msc_cmd_send_csd(msc_drv* d);
extern int msc_cmd_select_card(msc_drv* d);
extern int msc_cmd_set_bus_width(msc_drv* d, int width);
extern int msc_cmd_set_clr_card_detect(msc_drv* d, int arg);
extern int msc_cmd_switch_freq(msc_drv* d);
extern int msc_cmd_send_status(msc_drv* d);
extern int msc_cmd_set_block_len(msc_drv* d, unsigned len);

#endif /* __MSC_X1000_H__ */
