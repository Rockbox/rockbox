/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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

/********
 * APHB *
 ********/

#define HW_APBH_BASE        0x80004000

/* APHB channels */
#define HW_APBH_SSP(ssp)    ssp
#define HW_APBH_NAND(dev)   (4 + (ssp))

#define HW_APBH_CTRL0       (*(volatile uint32_t *)(HW_APBH_BASE + 0x0))
#define HW_APBH_CTRL0__FREEZE_CHANNEL(i)    (1 << (i))
#define HW_APBH_CTRL0__CLKGATE_CHANNEL(i)   (1 << ((i) + 8))
#define HW_APBH_CTRL0__RESET_CHANNEL(i)     (1 << ((i) + 16))
#define HW_APBH_CTRL0__APB_BURST4_EN        (1 << 28)
#define HW_APBH_CTRL0__APB_BURST8_EN        (1 << 29)

#define HW_APBH_CTRL1       (*(volatile uint32_t *)(HW_APBH_BASE + 0x10))
#define HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ(i)      (1 << (i))
#define HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ_EN(i)   (1 << ((i) + 16))

#define HW_APBH_CTRL2       (*(volatile uint32_t *)(HW_APBH_BASE + 0x20))
#define HW_APBH_CTRL2__CHx_ERROR_IRQ(i)         (1 << (i))
#define HW_APBH_CTRL2__CHx_ERROR_STATUS(i)      (1 << ((i) + 16))

#define HW_APBH_CHx_CURCMDAR(i) (*(volatile uint32_t *)(HW_APBH_BASE + 0x40 + 0x70 * (i)))

#define HW_APBH_CHx_NXTCMDAR(i) (*(volatile uint32_t *)(HW_APBH_BASE + 0x50 + 0x70 * (i)))

#define HW_APBH_CHx_CMD(i)      (*(volatile uint32_t *)(HW_APBH_BASE + 0x60 + 0x70 * (i)))

#define HW_APBH_CHx_BAR(i)      (*(volatile uint32_t *)(HW_APBH_BASE + 0x70 + 0x70 * (i)))

#define HW_APBH_CHx_SEMA(i)     (*(volatile uint32_t *)(HW_APBH_BASE + 0x80 + 0x70 * (i)))

#define HW_APBH_CHx_DEBUG1(i)   (*(volatile uint32_t *)(HW_APBH_BASE + 0x90 + 0x70 * (i)))

#define HW_APBH_CHx_DEBUG2(i)   (*(volatile uint32_t *)(HW_APBH_BASE + 0xa0 + 0x70 * (i)))
#define HW_APBH_CHx_DEBUG2__AHB_BYTES_BP    0
#define HW_APBH_CHx_DEBUG2__AHB_BYTES_BM    0xffff
#define HW_APBH_CHx_DEBUG2__APB_BYTES_BP    16
#define HW_APBH_CHx_DEBUG2__APB_BYTES_BM    0xffff0000

/********
 * APHX *
 ********/
 
/* APHX channels */
#define HW_APBX_AUDIO_ADC   0
#define HW_APBX_AUDIO_DAC   1
#define HW_APBX_I2C         3

#define HW_APBX_BASE        0x80024000

#define HW_APBX_CTRL0       (*(volatile uint32_t *)(HW_APBX_BASE + 0x0))

#define HW_APBX_CTRL1       (*(volatile uint32_t *)(HW_APBX_BASE + 0x10))
#define HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ(i)      (1 << (i))
#define HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ_EN(i)   (1 << ((i) + 16))

#define HW_APBX_CTRL2       (*(volatile uint32_t *)(HW_APBX_BASE + 0x20))
#define HW_APBX_CTRL2__CHx_ERROR_IRQ(i)         (1 << (i))
#define HW_APBX_CTRL2__CHx_ERROR_STATUS(i)      (1 << ((i) + 16))

#define HW_APBX_CHANNEL_CTRL    (*(volatile uint32_t *)(HW_APBX_BASE + 0x30))
#define HW_APBX_CHANNEL_CTRL__FREEZE_CHANNEL(i) (1 << (i))
#define HW_APBX_CHANNEL_CTRL__RESET_CHANNEL(i)  (1 << ((i) + 16))

#define HW_APBX_CHx_CURCMDAR(i) (*(volatile uint32_t *)(HW_APBX_BASE + 0x100 + (i) * 0x70))

#define HW_APBX_CHx_NXTCMDAR(i) (*(volatile uint32_t *)(HW_APBX_BASE + 0x110 + (i) * 0x70))

#define HW_APBX_CHx_CMD(i)      (*(volatile uint32_t *)(HW_APBX_BASE + 0x120 + (i) * 0x70))

#define HW_APBX_CHx_BAR(i)      (*(volatile uint32_t *)(HW_APBX_BASE + 0x130 + (i) * 0x70))

#define HW_APBX_CHx_SEMA(i)     (*(volatile uint32_t *)(HW_APBX_BASE + 0x140 + (i) * 0x70))

#define HW_APBX_CHx_DEBUG1(i)   (*(volatile uint32_t *)(HW_APBX_BASE + 0x150 + (i) * 0x70))

#define HW_APBX_CHx_DEBUG2(i)   (*(volatile uint32_t *)(HW_APBX_BASE + 0x160 + (i) * 0x70))
#define HW_APBX_CHx_DEBUG2__AHB_BYTES_BP    0
#define HW_APBX_CHx_DEBUG2__AHB_BYTES_BM    0xffff
#define HW_APBX_CHx_DEBUG2__APB_BYTES_BP    16
#define HW_APBX_CHx_DEBUG2__APB_BYTES_BM    0xffff0000

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
};

#define DMA_INFO_CURCMDADDR (1 << 0)
#define DMA_INFO_NXTCMDADDR (1 << 1)
#define DMA_INFO_CMD        (1 << 2)
#define DMA_INFO_BAR        (1 << 3)
#define DMA_INFO_APB_BYTES  (1 << 4)
#define DMA_INFO_AHB_BYTES  (1 << 5)
#define DMA_INFO_FREEZED    (1 << 6)
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
    bool freezed;
    bool gated;
    bool int_enabled;
    bool int_cmdcomplt;
    bool int_error;
};

#define APBH_DMA_CHANNEL(i)     i
#define APBX_DMA_CHANNEL(i)     ((i) | 0x10)
#define APB_IS_APBX_CHANNEL(x)  ((x) & 0x10)
#define APB_GET_DMA_CHANNEL(x)  ((x) & 0xf)

#define APB_SSP(ssp)        APBH_DMA_CHANNEL(HW_APBH_SSP(ssp))
#define APB_AUDIO_ADC       APBX_DMA_CHANNEL(HW_APBX_AUDIO_ADC)
#define APB_AUDIO_DAC       APBX_DMA_CHANNEL(HW_APBX_AUDIO_DAC)
#define APB_I2C             APBX_DMA_CHANNEL(HW_APBX_I2C)
#define APB_NAND(dev)       APBH_DMA_CHANNEL(HW_APBH_NAND(dev))

#define HW_APB_CHx_CMD__COMMAND_BM         0x3
#define HW_APB_CHx_CMD__COMMAND__NO_XFER   0
#define HW_APB_CHx_CMD__COMMAND__WRITE     1
#define HW_APB_CHx_CMD__COMMAND__READ      2
#define HW_APB_CHx_CMD__COMMAND__SENSE     3
#define HW_APB_CHx_CMD__CHAIN              (1 << 2)
#define HW_APB_CHx_CMD__IRQONCMPLT         (1 << 3)
/* those two are only available on APHB */
#define HW_APBH_CHx_CMD__NANDLOCK          (1 << 4)
#define HW_APBH_CHx_CMD__NANDWAIT4READY    (1 << 5)
#define HW_APB_CHx_CMD__SEMAPHORE          (1 << 6)
#define HW_APB_CHx_CMD__WAIT4ENDCMD        (1 << 7)
/* An errata advise not to use it */
//#define HW_APB_CHx_CMD__HALTONTERMINATE    (1 << 8)
#define HW_APB_CHx_CMD__CMDWORDS_BM         0xf000
#define HW_APB_CHx_CMD__CMDWORDS_BP         12
#define HW_APB_CHx_CMD__XFER_COUNT_BM       0xffff0000
#define HW_APB_CHx_CMD__XFER_COUNT_BP       16
/* For software use */
#define HW_APB_CHx_CMD__UNUSED_BP           8
#define HW_APB_CHx_CMD__UNUSED_BM           (0xf << 8)
#define HW_APB_CHx_CMD__UNUSED_MAGIC        (0xa << 8)

#define HW_APB_CHx_SEMA__PHORE_BM           0xff0000
#define HW_APB_CHx_SEMA__PHORE_BP           16

/* A single descriptor cannot transfer more than 2^16 bytes */
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
void imx233_dma_start_command(unsigned chan, struct apb_dma_command_t *cmd);
void imx233_dma_wait_completion(unsigned chan);
/* get some info
 * WARNING: if channel is not freezed, data might not be coherent ! */
struct imx233_dma_info_t imx233_dma_get_info(unsigned chan, unsigned flags);

#endif // __DMA_IMX233_H__
