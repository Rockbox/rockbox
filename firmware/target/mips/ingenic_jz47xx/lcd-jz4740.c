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

static volatile bool _lcd_on = false;
static volatile bool lcd_poweroff = false;
static struct mutex lcd_mtx;

/* LCD init */
void lcd_init_device(void)
{
    lcd_init_controller();
    _lcd_on = true;
    mutex_init(&lcd_mtx);
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
    
    _lcd_on = state;
}

bool lcd_enabled(void)
{
    return _lcd_on;
}

/* Don't switch threads when in interrupt mode! */
static void lcd_lock(void)
{
    if(LIKELY(!in_interrupt_mode()))
        mutex_lock(&lcd_mtx);
    else
        while( !(REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_TT));
}

static void lcd_unlock(void)
{
    if(LIKELY(!in_interrupt_mode()))
        mutex_unlock(&lcd_mtx);
    else
        while( !(REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_TT));
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    lcd_lock();
    
    lcd_set_target(x, y, width, height);
    
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) = 0;
    REG_DMAC_DRSR(DMA_LCD_CHANNEL)  = DMAC_DRSR_RS_SLCD; /* source = SLCD */
    REG_DMAC_DSAR(DMA_LCD_CHANNEL)  = ((unsigned int)&lcd_framebuffer[y][x]) & 0x1FFFFFFF;
    REG_DMAC_DTAR(DMA_LCD_CHANNEL)  = 0x130500B0; /* SLCD_FIFO */
    REG_DMAC_DTCR(DMA_LCD_CHANNEL)  = width*height;
    
    REG_DMAC_DCMD(DMA_LCD_CHANNEL)  = ( DMAC_DCMD_SAI     | DMAC_DCMD_RDIL_IGN | DMAC_DCMD_SWDH_32
                                      | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_16BIT );
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) = DMAC_DCCSR_NDES;
    
    __dcache_writeback_all(); /* Size of framebuffer is way bigger than cache size;
                                     we need to find a way to make the framebuffer uncached, so this statement can get removed. */
    
    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    
    REG_SLCD_CTRL |= SLCD_CTRL_DMA_EN;
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) |= DMAC_DCCSR_EN;

    if(LIKELY(!in_interrupt_mode()))
    {
        while( !(REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_TT) )
            yield();
    }
    else
        while( !(REG_DMAC_DCCSR(DMA_LCD_CHANNEL) & DMAC_DCCSR_TT));
    
    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_EN;
    
    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    
    REG_SLCD_CTRL &= ~SLCD_CTRL_DMA_EN;
    
    lcd_unlock();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!_lcd_on)
        return;
    
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
