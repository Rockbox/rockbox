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

#include "lcd.h"
#include "system.h"
#include "kernel.h"
#include "lcd-x1000.h"
#include "dma-x1000.h"
#include "irq-x1000.h"
#include "x1000/lcd.h"
#include "x1000/cpm.h"
#include <stdint.h>
#include <string.h>

#define LCD_DMA_CMD_SOFINT  (1 << 31)
#define LCD_DMA_CMD_EOFINT  (1 << 30)
#define LCD_DMA_CMD_COMMAND (1 << 29)
#define LCD_DMA_CMD_FRM_EN  (1 << 26)

#define LCD_DMA_CNT_BPP_15BIT          ((4 << 27)|(1<<30))
#define LCD_DMA_CNT_BPP_16BIT          (4 << 27)
#define LCD_DMA_CNT_BPP_18BIT_OR_24BIT (5 << 27)

/* Check if running with interrupts disabled (eg: panic screen) */
#define lcd_panic_mode \
    UNLIKELY((read_c0_status() & 1) == 0)

static void lcd_init_controller(const struct lcd_tgt_config* cfg)
{
    /* Set MCFG/MCFG_NEW according to target interface settings */
    unsigned mcfg = 0, mcfg_new = 0;

    switch(cfg->cmd_width) {
    case 8:  mcfg |= BF_LCD_MCFG_CWIDTH_V(8BIT); break;
    case 9:  mcfg |= BF_LCD_MCFG_CWIDTH_V(16BIT_OR_9BIT); break;
    case 16: mcfg |= BF_LCD_MCFG_CWIDTH_V(16BIT_OR_9BIT); break;
    case 18: mcfg |= BF_LCD_MCFG_CWIDTH_V(18BIT); break;
    case 24: mcfg |= BF_LCD_MCFG_CWIDTH_V(24BIT); break;
    default: break;
    }

    if(cfg->cmd_width == 9)
        mcfg_new |= BM_LCD_MCFG_NEW_CMD_9BIT;

    switch(cfg->bus_width) {
    case 8:  mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(8BIT); break;
    case 9:  mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(9BIT); break;
    case 16: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(16BIT); break;
    case 18: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(18BIT); break;
    case 24: mcfg_new |= BF_LCD_MCFG_NEW_DWIDTH_V(24BIT); break;
    default: break;
    }

    if(lcd_tgt_config.use_serial)
        mcfg_new |= jz_orf(LCD_MCFG_NEW, DTYPE_V(SERIAL), CTYPE_V(SERIAL));
    else
        mcfg_new |= jz_orf(LCD_MCFG_NEW, DTYPE_V(PARALLEL), CTYPE_V(PARALLEL));

    jz_vwritef(mcfg_new, LCD_MCFG_NEW,
               6800_MODE(lcd_tgt_config.use_6800_mode),
               CSPLY(lcd_tgt_config.wr_polarity ? 0 : 1),
               RSPLY(lcd_tgt_config.dc_polarity),
               CLKPLY(lcd_tgt_config.clk_polarity));

    /* Program the configuration. Note we cannot enable TE signal at
     * this stage, because the panel will need to be configured first.
     */
    jz_write(LCD_MCFG, mcfg);
    jz_write(LCD_MCFG_NEW, mcfg_new);
    jz_writef(LCD_MCTRL, NARROW_TE(0), TE_INV(0), NOT_USE_TE(1),
              DCSI_SEL(0), MIPI_SLCD(0), FAST_MODE(1), GATE_MASK(0),
              DMA_MODE(1), DMA_START(0), DMA_TX_EN(0));
    jz_writef(LCD_WTIME, DHTIME(0), DLTIME(0), CHTIME(0), CLTIME(0));
    jz_writef(LCD_TASH, TAH(0), TAS(0));
    jz_write(LCD_SMWT, 0);

    /* DMA settings */
    jz_writef(LCD_CTRL, BURST_V(64WORD),
              EOFM(1), SOFM(0), IFUM(0), QDM(0),
              BEDN(0), PEDN(0), ENABLE(0));
    jz_write(LCD_DAH, LCD_WIDTH);
    jz_write(LCD_DAV, LCD_HEIGHT);
}

static void lcd_send(uint32_t d)
{
    while(jz_readf(LCD_MSTATE, BUSY));
    REG_LCD_MDATA = d;
}

void lcd_set_clock(x1000_clk_t clk, uint32_t freq)
{
    uint32_t in_freq = clk_get(clk);
    uint32_t div = clk_calc_div(in_freq, freq);

    jz_writef(CPM_LPCDR, CE(1), CLKDIV(div - 1),
              CLKSRC(clk == X1000_CLK_MPLL ? 1 : 0));
    while(jz_readf(CPM_LPCDR, BUSY));
    jz_writef(CPM_LPCDR, CE(0));
}

void lcd_exec_commands(const uint32_t* cmdseq)
{
    while(*cmdseq != LCD_INSTR_END) {
        uint32_t instr = *cmdseq++;
        uint32_t d = 0;
        switch(instr) {
        case LCD_INSTR_CMD:
            d = jz_orf(LCD_MDATA, TYPE_V(CMD));
            /* fallthrough */

        case LCD_INSTR_DAT:
            d |= *cmdseq++;
            lcd_send(d);
            break;

        case LCD_INSTR_UDELAY:
            udelay(*cmdseq++);
            break;

        default:
            break;
        }
    }
}

void lcd_init_device(void)
{
    jz_writef(CPM_CLKGR, LCD(0));

    lcd_init_controller(&lcd_tgt_config);
    /* lcd_init_descriptors(&lcd_tgt_config); */

    lcd_tgt_enable(true);

    /* lcd_dma_start(); */
}

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return true;
}

void lcd_enable(bool en)
{
    (void)en;
}
#endif

#if defined(HAVE_LCD_SLEEP)
#if defined(LCD_X1000_FASTSLEEP)
# error "Do not define HAVE_LCD_SLEEP if target has LCD_X1000_FASTSLEEP"
#endif

void lcd_sleep(void)
{
}
#endif

void lcd_update(void)
{
    /* disable IRQ to avoid torn updates */
    int i = disable_irq_save();
    lcd_send(jz_orf(LCD_MDATA, TYPE_V(CMD)) | 0x2c);
    for(int y = 0; y < LCD_HEIGHT; ++y)
        for(int x = 0; x < LCD_WIDTH; ++x)
            lcd_send(*FBADDR(x, y));
    restore_irq(i);
}

void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    lcd_update();
}
