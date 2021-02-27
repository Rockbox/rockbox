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
#include "kernel.h"
#include "lcd-target.h"
#include "lcd-x1000.h"
#include "x1000/lcd.h"
#include "x1000/cpm.h"

static int lcd_state = LCD_STATE_OFF;

static void lcd_send(unsigned d)
{
    long deadline = current_tick + HZ/4;
    while(jz_readf(LCD_MSTATE, BUSY) && current_tick < deadline);
    REG_LCD_MDATA = d;
}

static void lcd_init_reg_mode(void)
{
    /* Set MCFG/MCFG_NEW according to target interface settings */
    unsigned mcfg = 0, mcfg_new = 0;

    switch(LCD_TGT_CMD_WIDTH) {
    case 8:
        jz_vwritef(mcfg, LCD_MCFG, CWIDTH_V(8BIT));
        jz_vwritef(mcfg, LCD_MCFG_NEW, CMD_9BIT(0));
        break;
    case 9:
        jz_vwritef(mcfg, LCD_MCFG, CWIDTH_V(16BIT_OR_9BIT));
        jz_vwritef(mcfg, LCD_MCFG_NEW, CMD_9BIT(1));
        break;
    case 16:
        jz_vwritef(mcfg, LCD_MCFG, CWIDTH_V(16BIT_OR_9BIT));
        jz_vwritef(mcfg, LCD_MCFG_NEW, CMD_9BIT(0));
        break;
    case 18:
        jz_vwritef(mcfg, LCD_MCFG, CWIDTH_V(18BIT));
        jz_vwritef(mcfg, LCD_MCFG_NEW, CMD_9BIT(0));
        break;
    case 24:
        jz_vwritef(mcfg, LCD_MCFG, CWIDTH_V(24BIT));
        jz_vwritef(mcfg, LCD_MCFG_NEW, CMD_9BIT(0));
        break;
    default:
        break;
    }

    switch(LCD_TGT_BUS_WIDTH) {
    case 8:
        jz_vwritef(mcfg_new, LCD_MCFG_NEW, DWIDTH_V(8BIT));
        break;
    case 9:
        jz_vwritef(mcfg_new, LCD_MCFG_NEW, DWIDTH_V(9BIT));
        break;
    case 16:
        jz_vwritef(mcfg_new, LCD_MCFG_NEW, DWIDTH_V(16BIT));
        break;
    case 18:
        jz_vwritef(mcfg_new, LCD_MCFG_NEW, DWIDTH_V(18BIT));
        break;
    case 24:
        jz_vwritef(mcfg_new, LCD_MCFG_NEW, DWIDTH_V(24BIT));
        break;
    default:
        break;
    }

    if(LCD_TGT_SERIAL) {
        jz_vwritef(mcfg_new, LCD_MCFG_NEW,
                   DTYPE_V(SERIAL), CTYPE_V(SERIAL));
    } else {
        jz_vwritef(mcfg_new, LCD_MCFG_NEW,
                   DTYPE_V(PARALLEL), CTYPE_V(PARALLEL));
    }

    jz_vwritef(mcfg_new, LCD_MCFG_NEW,
               6800_MODE(LCD_TGT_6800_MODE),
               CSPLY(LCD_TGT_DC_POLARITY),
               RSPLY(LCD_TGT_WR_POLARITY),
               CLKPLY(LCD_TGT_CLK_POLARITY));

    /* Program the configuration */
    jz_write(LCD_MCFG, mcfg);
    jz_write(LCD_MCFG_NEW, mcfg_new);
    jz_writef(LCD_MCTRL,
              NARROW_TE(0),
              TE_INV(0),
              NOT_USE_TE(1),
              DCSI_SEL(0),
              MIPI_SLCD(0),
              FAST_MODE(1),
              GATE_MASK(0),
              DMA_MODE(1),
              DMA_START(0),
              DMA_TX_EN(0));
    jz_writef(LCD_WTIME,
              DHTIME(0), DLTIME(0),
              CHTIME(0), CLTIME(0));
    jz_writef(LCD_TASH,
              TAH(0), TAS(0));
    jz_write(LCD_SMWT, 0);
}

int lcd_get_state(void)
{
    return lcd_state;
}

void lcd_set_clock(unsigned freq)
{
    unsigned in_freq = clk_get_sclk_a();
    unsigned div = 1;
    while(div < 256 && in_freq/div > freq)
        div += 1;

    jz_overwritef(CPM_LPCDR, CE(1), CLKSRC_V(SCLK_A), CLKDIV(div - 1));
    long deadline = current_tick + HZ/4;
    while(jz_readf(CPM_LPCDR, BUSY) && current_tick < deadline);
    jz_writef(CPM_LPCDR, CE(0));

    clk_notify_change(X1000_CLK_LCD);
}

void lcd_exec_commands(const unsigned* cmdseq)
{
    while(*cmdseq != LCD_INSTR_END) {
        unsigned instr = *cmdseq++;
        unsigned d = 0;
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
    lcd_tgt_power(true);
    lcd_init_reg_mode();
    lcd_tgt_ctl_on(true);
    lcd_state = LCD_STATE_RUN;
}

#ifdef HAVE_LCD_SHUTDOWN
void lcd_shutdown(void)
{
    lcd_state = LCD_STATE_OFF;
    lcd_tgt_ctl_on(false);
    lcd_tgt_power(false);
    jz_writef(CPM_CLKGR, LCD(1));
}
#endif

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

static int lcd_clip_coord(int c, int min, int max)
{
    if(c < min)  return min;
    if(c >= max) return max;
    return c;
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if(lcd_state != LCD_STATE_RUN)
        return;

    /* Clip the rectangle */
    int x1 = lcd_clip_coord(x, 0, LCD_WIDTH - 1);
    int y1 = lcd_clip_coord(y, 0, LCD_HEIGHT - 1);
    int x2 = lcd_clip_coord(x + width, 0, LCD_WIDTH - 1);
    int y2 = lcd_clip_coord(y + height, 0, LCD_HEIGHT - 1);

    /* Discard degenerates */
    if(x1 >= x2 || y1 >= y2)
        return;

    /* Send over the pixel data */
    lcd_tgt_set_fb_addr(x1, y1, x2, y2);
    for(int j = y1; j <= y2; ++j)
        for(int i = x1; i <= x2; ++i)
            lcd_send(*FBADDR(i, j));
}

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return lcd_state == LCD_STATE_RUN;
}

void lcd_enable(bool en)
{
    /* no recovery from power off */
    if(lcd_state == LCD_STATE_OFF)
        return;

    if(en) {
        bool need_update = false;
        if(lcd_state == LCD_STATE_SLEEP) {
            lcd_tgt_ctl_sleep(false);
            lcd_state = LCD_STATE_IDLE;
            need_update = true;
        }

        if(lcd_state == LCD_STATE_IDLE) {
            /* TODO: enable DMA */
            lcd_state = LCD_STATE_RUN;
        }

        if(need_update)
            lcd_update();
    } else if(!en && lcd_state == LCD_STATE_RUN) {
        /* TODO: disable DMA */
        lcd_state = LCD_STATE_IDLE;
    }
}
#endif

#ifdef HAVE_LCD_SLEEP
void lcd_sleep(void)
{
    switch(lcd_state) {
    case LCD_STATE_RUN:
        lcd_enable(false);
        /* fallthrough */
    case LCD_STATE_IDLE:
        lcd_tgt_ctl_sleep(true);
        lcd_state = LCD_STATE_SLEEP;
        break;
    default:
        /* either already asleep or powered off */
        break;
    }
}
#endif
