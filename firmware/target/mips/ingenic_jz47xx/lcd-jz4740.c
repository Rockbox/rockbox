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
    
    __dcache_writeback_all(); /* Size of framebuffer is way bigger than cache size.
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

/* (Mis)use LCD framebuffer as a temporary buffer */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    register off_t z;
    
    if(!lcd_is_on)
        return;
    
    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);
    
    __dcache_writeback_all();
    
    __cpm_start_ipu();
    
    IPU_STOP_IPU();
    IPU_RESET_IPU();
    IPU_CLEAR_END_FLAG();
    
    IPU_DISABLE_RSIZE();
    IPU_DISABLE_IRQ();
    
    IPU_SET_INFMT(INFMT_YUV420);
    IPU_SET_OUTFMT(OUTFMT_RGB565);
    
    IPU_SET_IN_FM(width, height);
    IPU_SET_Y_STRIDE(stride);
    IPU_SET_UV_STRIDE(stride, stride);
    
    IPU_SET_Y_ADDR(PHYSADDR((unsigned long)yuv_src[0]));
    IPU_SET_U_ADDR(PHYSADDR((unsigned long)yuv_src[1]));
    IPU_SET_V_ADDR(PHYSADDR((unsigned long)yuv_src[2]));
    IPU_SET_OUT_ADDR(PHYSADDR((unsigned long)FBADDR(y,x)));
    
    IPU_SET_OUT_FM(height, width);
    IPU_SET_OUT_STRIDE(height);
    
    IPU_SET_CSC_C0_COEF(YUV_CSC_C0);
    IPU_SET_CSC_C1_COEF(YUV_CSC_C1);
    IPU_SET_CSC_C2_COEF(YUV_CSC_C2);
    IPU_SET_CSC_C3_COEF(YUV_CSC_C3);
    IPU_SET_CSC_C4_COEF(YUV_CSC_C4);
    
    IPU_RUN_IPU();
    
    while(!(IPU_POLLING_END_FLAG()) && IPU_IS_ENABLED());
    
    IPU_CLEAR_END_FLAG();
    IPU_STOP_IPU();
    IPU_RESET_IPU();
    
    __cpm_stop_ipu();

    /* YUV speed is limited by LCD speed */
    lcd_update_rect(y, x, height, width);
}
