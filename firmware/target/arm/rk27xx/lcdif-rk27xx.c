/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 Marcin Bukat
 * Copyright (C) 2012 Andrew Ryabinin
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

/* dwdma linked list struct - hw defined
 * we don't use __attribute__((packed)) just because
 * all fields are 32bits and so are aligned
 */
struct llp_t {
    uint32_t sar;
    uint32_t dar;
    struct llp_t *llp;
    uint32_t ctl_l;
    uint32_t ctl_h;
    uint32_t dstat;
};

/* array of structs which describes full screen update
 * each struct describes dma transfer for single line
 */
static struct llp_t scr_llp[LCD_HEIGHT];

static uint32_t lcd_data_transform(uint32_t data)
{
    unsigned int r, g, b;

#if (LCD_DATABUS_WIDTH == LCDIF_18BIT)
    /* 18 bit interface */
    r = (data & 0x0000fc00)<<8;
    /* g = ((data & 0x00000300) >> 2) | ((data & 0x000000e0) >> 3); */
    g = ((data & 0x00000300) << 6) | ((data & 0x000000e0) << 5);
    b = (data & 0x00000001f) << 3;
#elif (LCD_DATABUS_WIDTH == LCDIF_16BIT)
    /* 16 bit interface */
    r = (data & 0x0000f800) << 8;
    g = (data & 0x000007e0) << 5;
    b = (data & 0x0000001f) << 3;
#else
#error "LCD_DATABUS_WIDTH needs to be defined in lcd-target.h"
#endif

    return (r | g | b);
}

static void lcdctrl_buff_setup(int width, int height)
{
    /* Warning: datasheet addresses and OF addresses
     * don't match for HOR_ACT and VERT_ACT
     */
    HOR_ACT = width + 3;        /* define horizonatal active region */
    VERT_ACT = height;          /* define vertical active region */

    /* This lines define layout of data in lcdif internal buffer
     * LINEx_UVADDR = LINEx_YADDR + 1
     * buffer is organized as 2048 x 32bit
     * we use RGB565 (16 bits per pixel) so we pack 2 pixels
     * in every lcdbuffer mem cell
     */
    width = width  >> 1;

    LINE0_YADDR = 0;
    LINE1_YADDR = 1 * width;
    LINE2_YADDR = 2 * width;
    LINE3_YADDR = 3 * width;

    LINE0_UVADDR = LINE0_YADDR + 1;
    LINE1_UVADDR = LINE1_YADDR + 1;
    LINE2_UVADDR = LINE2_YADDR + 1;
    LINE3_UVADDR = LINE3_YADDR + 1;
}

static void lcdctrl_init(void)
{
    int i;

    /* alpha b111
     * stop at current frame complete
     * MCU mode
     * 24b RGB
     */
    LCDC_CTRL = ALPHA(7) | LCDC_STOP | LCDC_MCU | RGB24B;
    MCU_CTRL = ALPHA_BASE(0x3f) | MCU_CTRL_BYPASS;

    VERT_PERIOD = (1<<7)|(1<<5)|1;  /* CSn/WEn/RDn signal timings */

    lcd_display_init();
    lcdctrl_bypass(0);
    lcdctrl_buff_setup(LCD_WIDTH, LCD_HEIGHT);

    LCDC_INTR_MASK = INTR_MASK_EVENLINE;

    /* This loop seems to fix strange glitch where
     * whole lcd content was shifted ~4 lines verticaly
     * on second lcd_update call
     */
    for (i=0; i<2048; i++)
        *((volatile uint32_t *)LCD_BUFF + i) = 0;

    /* Setup buffered writes to lcd controler */
    MCU_CTRL = MCU_CTRL_RS_HIGH|MCU_CTRL_BUFF_WRITE|MCU_CTRL_BUFF_START;
}

/* configure pins to drive lcd in 18bit or 16bit mode */
static void iomux_lcd(void)
{
    unsigned long muxa;

    muxa = SCU_IOMUXA_CON & ~(IOMUX_LCD_VSYNC|IOMUX_LCD_DEN|0xff);
#if (LCD_DATABUS_WIDTH == LCDIF_18BIT)
    muxa |= IOMUX_LCD_D18|IOMUX_LCD_D20|IOMUX_LCD_D22|IOMUX_LCD_D17|IOMUX_LCD_D16;
#endif

    SCU_IOMUXA_CON = muxa;
    SCU_IOMUXB_CON |= IOMUX_LCD_D815;
}

static void dwdma_init(void)
{
    DWDMA_DMA_CHEN = 0xf00;
    DWDMA_CLEAR_BLOCK = 0x0f;
    DWDMA_DMA_CFG = 1;        /* global enable */
}

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
                 (1<<28) ;
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
                      (1<<28) ;
              
   DWDMA_CTL_H(ch) = 1;
   DWDMA_CFG_L(ch) = (7<<5);
   DWDMA_CFG_H(ch) = (handshake<<11)|(1<<2);
   DWDMA_SGR(ch) = (13<<20);
   DWDMA_DMA_CHEN = (0x101<<ch);
}

static void create_llp(int x, int y, int width, int height)
{
    int i;

    width = width>>1;

    void* (*fbaddr)(int x, int y) = FB_CURRENTVP_BUFFER->get_address_fn;
    /* build LLPs */
    for (i=0; i<height; i++)
        llp_setup((void *)fbaddr(x,y+i),
                  (void*)(LCD_BUFF+((i%4)*4*width)),
                  &(scr_llp[i]),
                  width);

    llp_end(&scr_llp[height-1]);
}

/* Public functions */

/* Switch between lcdif bypass mode and buffered mode
 * used in private implementations of lcd_set_gram_area()
 */
void lcdctrl_bypass(unsigned int on_off)
{
    while (!(LCDC_STA & LCDC_MCU_IDLE));

    if (on_off)
        MCU_CTRL |= MCU_CTRL_BYPASS;
    else
        MCU_CTRL &= ~MCU_CTRL_BYPASS;
}

/* Helper functions used to write commands
 * to lcd controler in bypass mode
 * used in controler specific implementations of:
 * lcd_sleep()
 * lcd_display_init()
 */
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

/* rockbox API functions */
void lcd_init_device(void)
{
    iomux_lcd();       /* setup pins for lcd interface */
    dwdma_init();      /* init dwdma module */
    lcdctrl_init();    /* basic lcdc module configuration */
}

void lcd_update_rect(int x, int y, int width, int height)
{
    int x_end, y_end, x_align, y_align;

    /* min alowed transfer seems to be 4x4 pixels */
    x_align = x & 3;
    y_align = y & 3;
    x = x - x_align;
    y = y - y_align;
    width = (width + x_align + 3) & ~3;
    height = (height + y_align + 3) & ~3;
    x_end = x + width - 1;
    y_end = y + height - 1;

    lcd_set_gram_area(x, y, x_end, y_end);
    create_llp(x, y, width, height);
    lcdctrl_bypass(0);

    /* whole framebuffer for now */
    commit_discard_dcache_range(FBADDR(0,0), 2*LCD_WIDTH*LCD_HEIGHT);

    while (!(LCDC_STA & LCDC_MCU_IDLE));

    lcdctrl_buff_setup(width, height);
    dwdma_start(0, scr_llp, 6);
    udelay(10);

    /* Setup buffered writes to lcd controler */
    MCU_CTRL = MCU_CTRL_RS_HIGH|MCU_CTRL_BUFF_WRITE|MCU_CTRL_BUFF_START;

    /* Wait for DMA transfer to finish */
    while (DWDMA_CTL_L(0) & (1<<27));
}

void lcd_update()
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
