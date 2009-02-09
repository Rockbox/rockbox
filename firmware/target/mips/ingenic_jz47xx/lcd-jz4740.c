/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
 
#include "config.h"
#include "jz4740.h"
#include "lcd.h"
#include "lcd-target.h"
#include "system.h"
#include "kernel.h"
#include "backlight-target.h"

/*
   Warning: code behaviour is unpredictable when threads get switched in IRQ mode!
   So don't update the LCD in an interrupt handler!
 */

static volatile bool lcd_is_on = false;
static struct mutex lcd_mtx;
static struct wakeup lcd_wkup;

/* LCD init */
void lcd_init_device(void)
{
    __cpm_start_lcd();
    lcd_init_controller();
    __cpm_stop_lcd();
    lcd_is_on = true;
    mutex_init(&lcd_mtx);
    wakeup_init(&lcd_wkup);
    system_enable_irq(DMA_IRQ(DMA_LCD_CHANNEL));
}

void lcd_enable(bool state)
{
    if(state)
    {
        lcd_on();
        lcd_call_enable_hook();
    }
    else
        lcd_off();
    
    lcd_is_on = state;
}

bool lcd_enabled(void)
{
    return lcd_is_on;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
x=0;y=0;width=LCD_WIDTH;height=LCD_HEIGHT;
    mutex_lock(&lcd_mtx);
    
    __cpm_start_lcd();
    
    lcd_set_target(x, y, width, height);
    
    dma_enable();
    
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_LCD_CHANNEL)  = PHYSADDR((unsigned long)&lcd_framebuffer[y][x]);
    REG_DMAC_DRSR(DMA_LCD_CHANNEL)  = DMAC_DRSR_RS_SLCD; /* source = SLCD */
    REG_DMAC_DTAR(DMA_LCD_CHANNEL)  = PHYSADDR(SLCD_FIFO);
    REG_DMAC_DTCR(DMA_LCD_CHANNEL)  = width*height;
    
    REG_DMAC_DCMD(DMA_LCD_CHANNEL)  = ( DMAC_DCMD_SAI     | DMAC_DCMD_RDIL_IGN | DMAC_DCMD_SWDH_32
                                      | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_16BIT );
    
    __dcache_writeback_all(); /* Size of framebuffer is way bigger than cache size.
                                 We need to find a way to make the framebuffer uncached, so this statement can get removed. */
    
    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    REG_SLCD_CTRL |= SLCD_CTRL_DMA_EN; /* Enable SLCD DMA support */
    
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) |= DMAC_DCCSR_EN; /* Enable DMA channel */
    REG_DMAC_DCMD(DMA_LCD_CHANNEL) |= DMAC_DCMD_TIE; /* Enable DMA interrupt */

    wakeup_wait(&lcd_wkup, TIMEOUT_BLOCK); /* Sleeping in lcd_update() should be safe */
    
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_EN; /* Disable DMA channel */
    
    dma_disable();
    
    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    REG_SLCD_CTRL &= ~SLCD_CTRL_DMA_EN; /* Disable SLCD DMA support */
    
    __cpm_stop_lcd();
    
    mutex_unlock(&lcd_mtx);
}

void DMA_CALLBACK(DMA_LCD_CHANNEL)(void)
{
    if (REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_HLT)
        REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_HLT;

    if (REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_AR)
        REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_AR;

    if (REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_CT)
        REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_CT;

    if (REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_TT)
        REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_TT;
    
    wakeup_signal(&lcd_wkup);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_is_on || !backlight_enabled())
        return;
    
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    (void)src;
    (void)src_x;
    (void)src_y;
    (void)stride;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
