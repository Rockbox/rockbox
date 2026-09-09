/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
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

#ifndef __DPU_X1600_H__
#define __DPU_X1600_H__

/* Needed by the declarations below, not just by the ones further down: the
 * usbstage payloads include this header without pulling these in first. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The SoC driver handles DPU timing and scanout. The board supplies
 * lcd_tgt_config, lcd_tgt_enable() and lcd_tgt_sleep() for its panel. */

#include "clk-x1600.h"

/* Panel timing in fb_videomode units. The driver converts it to DPU
 * start/end values for LCD_WIDTH by LCD_HEIGHT. Lane order and sync
 * polarities are currently fixed in dpu-x1600.c. */
struct lcd_tgt_config {
    /* Requested pixel clock in Hz; the source PLL and integer divider
     * determine the actual rate. */
    uint32_t pixclock;

    /* Horizontal timing, in dot clocks. */
    uint16_t hsync_len;         /* HSYNC pulse width          */
    uint16_t left_margin;       /* horizontal back porch      */
    uint16_t right_margin;      /* horizontal front porch     */

    /* Vertical timing, in lines. */
    uint16_t vsync_len;         /* VSYNC pulse width          */
    uint16_t upper_margin;      /* vertical back porch        */
    uint16_t lower_margin;      /* vertical front porch       */
};

/* Static configuration for the target's panel; defined by the target. */
extern const struct lcd_tgt_config lcd_tgt_config;

/* Configure CPM_LPCDR from SCLK_A, MPLL or EPLL. Invalid requests leave
 * the pixel clock unchanged. */
extern void lcd_set_clock(x1600_clk_t clksrc, uint32_t freq);

/* Board hook to power and initialise the panel, including lcd_set_clock(). */
extern void lcd_tgt_enable(bool on);

/* Board hook to enter or leave the panel low-power state. */
extern void lcd_tgt_sleep(bool sleep);

/* Debug interface */

/* Physical address the DPU is currently scanning out of.  Zero before
 * lcd_init_device() has run. */
extern uint32_t dpu_get_scanout_addr(void);

/* Return DC_ST, or zero when the DPU is unpowered. */
extern uint32_t dpu_get_status(void);

/* Debug snapshot indices:
 * 0 DC_ST, 1 DISP_COM, 2 SRD_CHAIN_ADDR, 3-6 TFT timings, 7 TFT_TRAN_CFG,
 * 8-10 DMA/stop counters, 11 zero-frequency request flag,
 * 12 source clock at the last zero-frequency request,
 * 13 framebuffer address, 14 source pixel, 15 destination pixel,
 * 16 descriptor framebuffer address, 17 scanout-buffer address.
 * Returns 0xDEAD0000|which when dpu_powered is false. */
uint32_t dpu_debug_snapshot(int which);

/* The framebuffer base lcd_update() actually copies from, latched at init.
 * Exposed so a bring-up build can compare it against a live FBADDR(0,0). */
void* lcd_fbbase_debug(void);

/* Bring-up only: run lcd_update()'s two steps with a sample between them. */
void lcd_update_debug(uint16_t* after_commit, uint16_t* after_copy);

/* Bring-up only: 0=fbcopy runs, 1=fbcopy timeouts, 2=DMA_CHN_CS, 3=CLKGR.PDMA */
uint32_t lcd_dma_debug(int which);

/* SRD descriptor layout: five words in memory. */
#define DPU_SRD_DESC_WORDS  5
#define DPU_SRD_NEXT        0
#define DPU_SRD_FBADDR      1
#define DPU_SRD_STRIDE      2
#define BP_DPU_SRD_STRIDE_STRIDE    0
#define BM_DPU_SRD_STRIDE_STRIDE    0x1fff
#define DPU_SRD_FORMAT      3
#define BP_DPU_SRD_FORMAT_FORMAT        19
#define BM_DPU_SRD_FORMAT_FORMAT        0x780000
#define BV_DPU_SRD_FORMAT_FORMAT__RGB555 0x0
#define BV_DPU_SRD_FORMAT_FORMAT__RGB565 0x2
#define BV_DPU_SRD_FORMAT_FORMAT__RGB888 0x4
#define BP_DPU_SRD_FORMAT_COLOR         16
#define BM_DPU_SRD_FORMAT_COLOR         0x70000
#define BV_DPU_SRD_FORMAT_COLOR__RGB    0x0
#define BV_DPU_SRD_FORMAT_COLOR__RBG    0x1
#define BV_DPU_SRD_FORMAT_COLOR__GRB    0x2
#define BV_DPU_SRD_FORMAT_COLOR__GBR    0x3
#define BV_DPU_SRD_FORMAT_COLOR__BRG    0x4
#define BV_DPU_SRD_FORMAT_COLOR__BGR    0x5
#define BP_DPU_SRD_FORMAT_CHAIN_END     0
#define BM_DPU_SRD_FORMAT_CHAIN_END     0x1
#define DPU_SRD_INTMASK     4
#define BM_DPU_SRD_INTMASK_EOD_MSK      0x20000     /* DISP_END   */
#define BM_DPU_SRD_INTMASK_SOS_MSK      0x4         /* SRD_START  */
#define BM_DPU_SRD_INTMASK_EOS_MSK      0x2         /* SRD_END    */

#define BF_DPU_SRD_STRIDE_STRIDE(v) (((v) & 0x1fff) << 0)
#define BF_DPU_SRD_FORMAT_FORMAT(v)     (((v) & 0xf) << 19)
#define BF_DPU_SRD_FORMAT_COLOR(v)      (((v) & 0x7) << 16)
#define BF_DPU_SRD_FORMAT_CHAIN_END(v)  (((v) & 0x1) << 0)

#endif /* __DPU_X1600_H__ */
