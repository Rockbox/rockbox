/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __DMA_IMX233_H__
#define __DMA_IMX233_H__

#include "cpu.h"
#include "system.h"
#include "system-target.h"

/************
 * CHANNELS *
 ************/

#define APBH_DMA_CHANNEL(i)     i
#define APBX_DMA_CHANNEL(i)     ((i) | 0x10)
#define APB_IS_APBX_CHANNEL(x)  ((x) & 0x10)
#define APB_GET_DMA_CHANNEL(x)  ((x) & 0xf)

#if IMX233_SUBTARGET >= 3700
// NOTE: although undocumented, the iMX233 channel 0 is actually the LCDIF one
#define APB_LCDIF           APBH_DMA_CHANNEL(0)
#else
#define APB_LCDIF           APBX_DMA_CHANNEL(4)
#endif

#define APB_SSP(ssp)        APBH_DMA_CHANNEL(ssp)
#define APB_GPMI(dev)       APBH_DMA_CHANNEL(4 + (dev))

#define APB_AUDIO_ADC       APBX_DMA_CHANNEL(0)
#define APB_AUDIO_DAC       APBX_DMA_CHANNEL(1)
#define APB_I2C             APBX_DMA_CHANNEL(3)
// NOTE: although undocumented, the IMX233 channel 5 is actually the DRI one
#define APB_DRI             APBX_DMA_CHANNEL(5)

/**********
 * COMMON *
 **********/

/* DMA structures should be cache aligned and be padded so that their size
 * is a multiple of a cache line size. Otherwise some nasty side effects
 * could occur with adjacents data fields.
 * The same apply to DMA buffers for the same reasons */
struct apb_dma_command_t
{
    struct apb_dma_command_t *next;
    uint32_t cmd;
    void *buffer;
    /* PIO words follow */
} __attribute__((packed));

#define DMA_INFO_CURCMDADDR (1 << 0)
#define DMA_INFO_NXTCMDADDR (1 << 1)
#define DMA_INFO_CMD        (1 << 2)
#define DMA_INFO_BAR        (1 << 3)
#define DMA_INFO_APB_BYTES  (1 << 4)
#define DMA_INFO_AHB_BYTES  (1 << 5)
#define DMA_INFO_FROZEN     (1 << 6)
#define DMA_INFO_GATED      (1 << 7)
#define DMA_INFO_INTERRUPT  (1 << 8)
#define DMA_INFO_ALL        0x1ff

struct imx233_dma_info_t
{
    unsigned long cur_cmd_addr;
    unsigned long nxt_cmd_addr;
    unsigned long cmd;
    unsigned long bar;
    unsigned apb_bytes;
    unsigned ahb_bytes;
    bool frozen;
    bool gated;
    bool int_enabled;
    bool int_cmdcomplt;
    bool int_error;
    int nr_unaligned;
};

#define BM_APB_CHx_CMD_COMMAND          0x3
#define BP_APB_CHx_CMD_COMMAND          0
#define BF_APB_CHx_CMD_COMMAND(v)       ((v) & 0x3)
#define BF_APB_CHx_CMD_COMMAND_V(v)     BF_APB_CHx_CMD_COMMAND(BV_APB_CHx_CMD_COMMAND__##v)
#define BV_APB_CHx_CMD_COMMAND__NO_XFER 0
#define BV_APB_CHx_CMD_COMMAND__WRITE   1
#define BV_APB_CHx_CMD_COMMAND__READ    2
#define BV_APB_CHx_CMD_COMMAND__SENSE   3
#define BM_APB_CHx_CMD_CHAIN            (1 << 2)
#define BP_APB_CHx_CMD_CHAIN            2
#define BF_APB_CHx_CMD_CHAIN(v)         (((v) & 1) << 2)
#define BM_APB_CHx_CMD_IRQONCMPLT       (1 << 3)
#define BP_APB_CHx_CMD_IRQONCMPLT       3
#define BF_APB_CHx_CMD_IRQONCMPLT(v)    (((v) & 1) << 3)
/* those two are only available on APHB */
#define BM_APBH_CHx_CMD_NANDLOCK        (1 << 4)
#define BP_APBH_CHx_CMD_NANDLOCK        4
#define BF_APBH_CHx_CMD_NANDLOCK(v)     (((v) & 1) << 4)
#define BM_APBH_CHx_CMD_NANDWAIT4READY  (1 << 5)
#define BP_APBH_CHx_CMD_NANDWAIT4READY  5
#define BF_APBH_CHx_CMD_NANDWAIT4READY(v)   (((v) & 1) << 5)

#define BM_APB_CHx_CMD_SEMAPHORE        (1 << 6)
#define BP_APB_CHx_CMD_SEMAPHORE        6
#define BF_APB_CHx_CMD_SEMAPHORE(v)     (((v) & 1) << 6)
#define BM_APB_CHx_CMD_WAIT4ENDCMD      (1 << 7)
#define BP_APB_CHx_CMD_WAIT4ENDCMD      7
#define BF_APB_CHx_CMD_WAIT4ENDCMD(v)   (((v) & 1) << 7)
/** WARNING: An errata advise not to use it */
#define BM_APB_CHx_CMD_HALTONTERMINATE (1 << 8)
#define BP_APB_CHx_CMD_HALTONTERMINATE 8
#define BF_APB_CHx_CMD_HALTONTERMINATE(v)   (((v) & 1) << 8)
#define BM_APB_CHx_CMD_CMDWORDS        0xf000
#define BP_APB_CHx_CMD_CMDWORDS        12
#define BF_APB_CHx_CMD_CMDWORDS(v)     (((v) & 0xf) << 12)
#define BM_APB_CHx_CMD_XFER_COUNT      0xffff0000
#define BP_APB_CHx_CMD_XFER_COUNT      16
#define BF_APB_CHx_CMD_XFER_COUNT(v)   (((v) & 0xffff) << 16)
/* For software use */
#define BP_APB_CHx_CMD_UNUSED           8
#define BM_APB_CHx_CMD_UNUSED           (0xf << 8)
#define BF_APB_CHx_CMD_UNUSED(v)        (((v) & 0xf) << 8)
#define BF_APB_CHx_CMD_UNUSED_V(n)      BF_APB_CHx_CMD_UNUSED(BV_APB_CHx_CMD_UNUSED__##n)
#define BFM_APB_CHx_CMD_UNUSED(v)       BM_APB_CHx_CMD_UNUSED
#define BV_APB_CHx_CMD_UNUSED__MAGIC    0xa
#define BFM_APB_CHx_CMD_UNUSED_V(v)     BM_APB_CHx_CMD_UNUSED

/* A single descriptor cannot transfer more than 2^16 bytes but because of the
 * weird 0=64KiB, it's safer to restrict to 2^15 */
#define IMX233_MAX_SINGLE_DMA_XFER_SIZE     (1 << 16)

void imx233_dma_init(void);
void imx233_dma_reset_channel(unsigned chan);
/* only apbh channel have clkgate control */
void imx233_dma_clkgate_channel(unsigned chan, bool enable_clock);

void imx233_dma_freeze_channel(unsigned chan, bool freeze);
void imx233_dma_enable_channel_interrupt(unsigned chan, bool enable);
/* clear both channel complete and error bits */
void imx233_dma_clear_channel_interrupt(unsigned chan);
bool imx233_dma_is_channel_error_irq(unsigned chan);
/* assume no command is in progress */
void imx233_dma_prepare_command(unsigned chan, struct apb_dma_command_t *cmd);
void imx233_dma_set_next_command(unsigned chan, struct apb_dma_command_t *cmd);
void imx233_dma_inc_sema(unsigned chan, unsigned amount);
/* wrapper around prepare_command, set_next_command, inc_sema(1) */
void imx233_dma_start_command(unsigned chan, struct apb_dma_command_t *cmd);
/* return value of the semaphore */
int imx233_dma_wait_completion(unsigned chan, unsigned tmo);
/* get some info
 * WARNING: if channel is not freezed, data might not be coherent ! */
struct imx233_dma_info_t imx233_dma_get_info(unsigned chan, unsigned flags);

#endif // __DMA_IMX233_H__
