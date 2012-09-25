/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "lcdif-rk27xx.h"
#include "lcd-target.h"
#include <string.h>

struct mutex lcd_updating;

void INT_DWDMA(void)
{
    DWDMA_CLEAR_TFR |= 1;
    mutex_unlock(&lcd_updating);    
    updating = 0;
}


unsigned int lcd_data_transform(unsigned int data)
{
    unsigned int r, g, b;

#if defined(RK27_GENERIC)
    /* 18 bit interface */
    r = (data & 0x0000fc00)<<8;
    /* g = ((data & 0x00000300) >> 2) | ((data & 0x000000e0) >> 3); */
    g = ((data & 0x00000300) << 6) | ((data & 0x000000e0) << 5);
    b = (data & 0x00000001f) << 3;
#elif defined(HM60X) || defined(HM801)
    /* 16 bit interface */
    r = (data & 0x0000f800) << 8;
    g = (data & 0x000007e0) << 5;
    b = (data & 0x0000001f) << 3;
#else
#error "Unknown target"
#endif

    return (r | g | b);
}

void lcd_cmd(unsigned int cmd)
{
    LCD_COMMAND = lcd_data_transform(cmd);
}

void lcd_data(unsigned int data)
{
    LCD_DATA = lcd_data_transform(data);
}

void lcd_write_reg(unsigned int reg, unsigned int val)
{
    lcd_cmd(reg);
    lcd_data(val);
}

void lcdctrl_bypass(unsigned int on_off)
{
    while (!(LCDC_STA & LCDC_MCU_IDLE));

    if (on_off)
        MCU_CTRL |= MCU_CTRL_BYPASS;
    else
        MCU_CTRL &= ~MCU_CTRL_BYPASS;
}

/* This part is unclear. I am unable to use buffered/FIFO based writes
 * to lcd. Depending on settings of IF I get various patterns on display
 * but not what I want to display apparently.
 */
void lcdctrl_init(void)
{
    /* alpha b111
     * stop at current frame complete
     * MCU mode
     * 24b RGB
     */
    MCU_CTRL = 0;
    START_X = 0;
    START_Y = 0;
    ALPHA_BLX = 0x3FF;
    ALPHA_BTY = 0x3FF;
    ALPHA_BRX = 0x3FF;
    ALPHA_BBY = 0x3FF;

    memset(LCD_BUFF-8*4, 0, (2048+8)*4);

    LCDC_CTRL = ALPHA(7) | LCDC_STOP | LCDC_MCU | RGB24B;
    MCU_CTRL = ALPHA_BASE(0x3f) | MCU_CTRL_BYPASS;

    HOR_BP = LCD_WIDTH + 3;    /* define horizonatal active region */
    VERT_BP = LCD_HEIGHT;       /* define vertical active region */
    VERT_PERIOD = (1<<7)|(1<<5)|1;  /* CSn/WEn/RDn signal timings */

    lcd_display_init();
    lcdctrl_bypass(0);

    LINE0_YADDR = 0;
    LINE1_YADDR = 1 * LCD_WIDTH/2;
    LINE2_YADDR = 2 * LCD_WIDTH/2;
    LINE3_YADDR = 3 * LCD_WIDTH/2;

    LINE0_UVADDR = 1;
    LINE1_UVADDR = (1 * LCD_WIDTH/2) + 1;
    LINE2_UVADDR = (2 * LCD_WIDTH/2) + 1;
    LINE3_UVADDR = (3 * LCD_WIDTH/2) + 1;
    DELTA_X = 0x200;
    DELTA_Y = 0x200;

    ALPHA_ALX = LCD_WIDTH - 16;
    ALPHA_ATY = 0;
    ALPHA_ARX = LCD_WIDTH - 1;
    ALPHA_ABY = LCD_HEIGHT;

    LCDC_INTR_MASK = 0; /*INTR_MASK_LINE;  INTR_MASK_EVENLINE; */
    MCU_CTRL = (1<<1)|(1<<2)|(1<<5);
}

/* configure pins to drive lcd in 18bit mode (16bit mode for HiFiMAN's) */
static void iomux_lcd(enum lcdif_mode_t mode)
{
    unsigned long muxa;

    muxa = SCU_IOMUXA_CON & ~(IOMUX_LCD_VSYNC|IOMUX_LCD_DEN|0xff);

    if (mode == LCDIF_18BIT)
    {
        muxa |= IOMUX_LCD_D18|IOMUX_LCD_D20|IOMUX_LCD_D22|IOMUX_LCD_D17|IOMUX_LCD_D16;
    }

    SCU_IOMUXA_CON = muxa;
    SCU_IOMUXB_CON |= IOMUX_LCD_D815;
}

static void dwdma_init(void)
{
    DWDMA_DMA_CHEN = 0xf00;
    DWDMA_CLEAR_BLOCK = 0x0f;
    DWDMA_DMA_CFG = 1; /* global enable */
    INTC_IECR |= (1<<25);
    INTC_IMR |= (1<<25);
}

/* dwdma linked list struct */
struct llp_t {
    uint32_t sar;
    uint32_t dar;
    struct llp_t *llp;
    uint32_t ctl_l;
    uint32_t ctl_h;
    uint32_t dstat;
};


/* structs which describe full screen update */
static struct llp_t scr_llp[LCD_HEIGHT];


static void llp_setup(void *src, void *dst, struct llp_t *llp, uint32_t size)
{
    llp->sar = (uint32_t)src;
    llp->dar = (uint32_t)dst;
    llp->llp = llp + 1;
    llp->ctl_h = size;
    llp->ctl_l = (1<<20) |
                 (1<<23) |
                 (1<<17) |
                 (2<<1)  |
                 (2<<4)  |
                 (3<<11) |
                 (3<<14) |
                 (1<<27) |
                 (1<<28) |
                       1 ;
}

static void llp_end(struct llp_t *llp)
{
    llp->ctl_l &= ~((1<<27)|(1<<28));
}


static void dwdma_start(uint8_t ch, struct llp_t *llp, uint8_t handshake)
{
    DWDMA_SAR(ch) = 0;
    DWDMA_DAR(ch) = 0;
    DWDMA_LLP(ch) = (uint32_t)llp;
    DWDMA_CTL_L(ch) = (1<<20) |
                      (1<<23) |
                      (1<<17) |
                      (2<<1)  |
                      (2<<4)  |
                      (3<<11) |
                      (3<<14) |
                      (1<<27) |
                      (1<<28) |
                            1 ;

   DWDMA_CTL_H(ch) = 1;
   DWDMA_CFG_L(ch) = (7<<5);
   DWDMA_CFG_H(ch) = (handshake<<11)|(1<<2);
   DWDMA_SGR(ch) = (13<<20);
   DWDMA_DMA_CHEN = (0x101<<ch);
   DWDMA_MASK_TFR = (1 << (ch+8)) | (1<<ch);
}


void create_llp(void)
{
    int i = 0;

    /* build LLPs */
    for (i=0; i<LCD_HEIGHT; i++)
        llp_setup((void *)FBADDR(0,i), (void*)(&LCD_BUFF+((i%4)*4*LCD_WIDTH/2)), &(scr_llp[i]), LCD_WIDTH/2);

    llp_end(&scr_llp[LCD_HEIGHT-1]);
}

void lcd_init_device(void)
{
    iomux_lcd(LCD_DATABUS_WIDTH);   /* setup pins for lcd interface */

    INTC_IMR |= (1<<27);
    INTC_IECR |= (1<<27);
    INTC_SCR27 = 0xc0;

    dwdma_init();
    create_llp();
    lcdctrl_init();    /* basic lcdc module configuration */

    mutex_init(&lcd_updating);
}

void lcd_update()
{
    mutex_lock(&lcd_updating);

    lcd_set_gram_area(0, 0, LCD_WIDTH, LCD_HEIGHT);
    lcdctrl_bypass(0);

    commit_discard_dcache_range(FBADDR(0,0), 2*LCD_WIDTH*LCD_HEIGHT);

    while (!(LCDC_STA & LCDC_MCU_IDLE));

    dwdma_start(0, scr_llp, 6);
    udelay(10);

    MCU_CTRL=(1<<1)|(1<<2)|(1<<5);
}
