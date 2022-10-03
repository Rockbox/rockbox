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
#include <sys/types.h> /* off_t */

#include "config.h"
#include "jz4740.h"
#include "lcd.h"
#include "lcd-target.h"
#include "system.h"
#include "kernel.h"

/*
   Warning: code behaviour is unpredictable when switch_thread() gets called in IRQ mode!
   So don't update the LCD in an interrupt handler!
 */

static volatile bool lcd_is_on = false;
static struct mutex lcd_mtx;
static struct semaphore lcd_wkup;
static int lcd_count = 0;

void lcd_clock_enable(void)
{
    if(++lcd_count == 1)
        __cpm_start_lcd();
}

void lcd_clock_disable(void)
{
    if(--lcd_count == 0)
        __cpm_stop_lcd();
}

/* LCD init */
void lcd_init_device(void)
{
    lcd_init_controller();

    lcd_is_on = true;
    mutex_init(&lcd_mtx);
    semaphore_init(&lcd_wkup, 1, 0);
    system_enable_irq(DMA_IRQ(DMA_LCD_CHANNEL));
}

#ifdef HAVE_LCD_ENABLE
void lcd_enable(bool state)
{
    if(lcd_is_on == state)
        return;

    if(state)
    {
        lcd_on();
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
        lcd_off();

    lcd_is_on = state;
}
#endif

bool lcd_active(void)
{
    return lcd_is_on;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
   /* Currently only do updates with full LCD width.
     * DMA can't handle full partial updates and CPU is too slow compared
     * to DMA updates */
    x = 0;
    width = LCD_WIDTH;

    mutex_lock(&lcd_mtx);

    lcd_clock_enable();

    lcd_set_target(x, y, width, height);

    dma_enable();

    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) = DMAC_DCCSR_NDES;
    REG_DMAC_DSAR(DMA_LCD_CHANNEL)  = PHYSADDR((unsigned long)FBADDR(x,y));
    REG_DMAC_DRSR(DMA_LCD_CHANNEL)  = DMAC_DRSR_RS_SLCD;
    REG_DMAC_DTAR(DMA_LCD_CHANNEL)  = PHYSADDR(SLCD_FIFO);
    REG_DMAC_DTCR(DMA_LCD_CHANNEL)  = (width * height) >> 3;

    REG_DMAC_DCMD(DMA_LCD_CHANNEL)  = ( DMAC_DCMD_SAI     | DMAC_DCMD_RDIL_IGN | DMAC_DCMD_SWDH_32
                                      | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_16BYTE );

    // XXX range
    commit_discard_dcache(); /* Size of framebuffer is way bigger than cache size.
                                We need to find a way to make the framebuffer uncached, so this statement can get removed. */

    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    REG_SLCD_CTRL |= SLCD_CTRL_DMA_EN; /* Enable SLCD DMA support */

    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) |= DMAC_DCCSR_EN; /* Enable DMA channel */
    REG_DMAC_DCMD(DMA_LCD_CHANNEL) |= DMAC_DCMD_TIE; /* Enable DMA interrupt */

    semaphore_wait(&lcd_wkup, TIMEOUT_BLOCK); /* Sleeping in lcd_update() should be safe */

    REG_DMAC_DCCSR(DMA_LCD_CHANNEL) &= ~DMAC_DCCSR_EN; /* Disable DMA channel */
    dma_disable();

    while(REG_SLCD_STATE & SLCD_STATE_BUSY);
    REG_SLCD_CTRL &= ~SLCD_CTRL_DMA_EN; /* Disable SLCD DMA support */

    lcd_clock_disable();

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

    semaphore_release(&lcd_wkup);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if(!lcd_is_on)
        return;

    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
