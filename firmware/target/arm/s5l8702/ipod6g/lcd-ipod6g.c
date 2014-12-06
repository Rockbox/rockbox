/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-nano2g.c 28868 2010-12-21 06:59:17Z Buschel $
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "pmu-target.h"
#include "power.h"
#include "string.h"
#include "dma-s5l8702.h"


#define R_HORIZ_GRAM_ADDR_SET     0x200
#define R_VERT_GRAM_ADDR_SET      0x201
#define R_WRITE_DATA_TO_GRAM      0x202
#define R_HORIZ_ADDR_START_POS    0x210
#define R_HORIZ_ADDR_END_POS      0x211
#define R_VERT_ADDR_START_POS     0x212
#define R_VERT_ADDR_END_POS       0x213


/* LCD type 1 register defines */

#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c


/** globals **/

int lcd_type; /* also needed in debug-s5l8702.c */
static struct mutex lcd_mutex;
static uint16_t lcd_dblbuf[LCD_HEIGHT][LCD_WIDTH] CACHEALIGN_ATTR;
static bool lcd_ispowered;

#define SLEEP       0
#define CMD16       1
#define DATA16      2
#define REG15       3
#define END         0xff

/* powersave sequences */

static const unsigned short lcd_sleep_sequence_01[] =
{
    CMD16,  0x028,  /* Display Off */
    SLEEP,  0x005,  /* 50 ms */
    CMD16,  0x010,  /* Sleep In Mode */
    SLEEP,  0x005,  /* 50 ms */
    END
};

static const unsigned short lcd_deep_stby_sequence_23[] =
{
    /* Display Off */
    REG15,  0x007,  0x0172,
    REG15,  0x030,  0x03ff,
    SLEEP,  0x00a,
    REG15,  0x007,  0x0120,
    REG15,  0x030,  0x0000,
    REG15,  0x100,  0x0780,
    REG15,  0x007,  0x0000,
    REG15,  0x101,  0x0260,
    REG15,  0x102,  0x00a9,
    SLEEP,  0x003,
    REG15,  0x100,  0x0700,

    /* Deep Standby Mode */
    REG15,  0x100,  0x0704,
    SLEEP,  0x005,
    END
};

#ifdef HAVE_LCD_SLEEP
/* init sequences */

static const unsigned short lcd_init_sequence_01[] =
{
    CMD16,  0x011,  /* Sleep Out Mode */
    SLEEP,  0x006,  /* 60 ms */
    CMD16,  0x029,  /* Display On */
    END
};

static const unsigned short lcd_init_sequence_23[] =
{
    /* Display settings */
    REG15,  0x008,  0x0808,
    REG15,  0x010,  0x0013,
    REG15,  0x011,  0x0300,
    REG15,  0x012,  0x0101,
    REG15,  0x013,  0x0a03,
    REG15,  0x014,  0x0a0e,
    REG15,  0x015,  0x0a19,
    REG15,  0x016,  0x2402,
    REG15,  0x018,  0x0001,
    REG15,  0x090,  0x0021,

    /* Gamma settings */
    REG15,  0x300,  0x0307,
    REG15,  0x301,  0x0003,
    REG15,  0x302,  0x0402,
    REG15,  0x303,  0x0303,
    REG15,  0x304,  0x0300,
    REG15,  0x305,  0x0407,
    REG15,  0x306,  0x1c04,
    REG15,  0x307,  0x0307,
    REG15,  0x308,  0x0003,
    REG15,  0x309,  0x0402,
    REG15,  0x30a,  0x0303,
    REG15,  0x30b,  0x0300,
    REG15,  0x30c,  0x0407,
    REG15,  0x30d,  0x1c04,

    REG15,  0x310,  0x0707,
    REG15,  0x311,  0x0407,
    REG15,  0x312,  0x0306,
    REG15,  0x313,  0x0303,
    REG15,  0x314,  0x0300,
    REG15,  0x315,  0x0407,
    REG15,  0x316,  0x1c01,
    REG15,  0x317,  0x0707,
    REG15,  0x318,  0x0407,
    REG15,  0x319,  0x0306,
    REG15,  0x31a,  0x0303,
    REG15,  0x31b,  0x0300,
    REG15,  0x31c,  0x0407,
    REG15,  0x31d,  0x1c01,

    REG15,  0x320,  0x0206,
    REG15,  0x321,  0x0102,
    REG15,  0x322,  0x0404,
    REG15,  0x323,  0x0303,
    REG15,  0x324,  0x0300,
    REG15,  0x325,  0x0407,
    REG15,  0x326,  0x1c1f,
    REG15,  0x327,  0x0206,
    REG15,  0x328,  0x0102,
    REG15,  0x329,  0x0404,
    REG15,  0x32a,  0x0303,
    REG15,  0x32b,  0x0300,
    REG15,  0x32c,  0x0407,
    REG15,  0x32d,  0x1c1f,

    /* GRAM and Base Imagen settings (ili9326ds) */
    REG15,  0x400,  0x001d,
    REG15,  0x401,  0x0001,
    REG15,  0x205,  0x0060,

    /* Power settings */
    REG15,  0x007,  0x0001,
    REG15,  0x031,  0x0071,
    REG15,  0x110,  0x0001,
    REG15,  0x100,  0x17b0,
    REG15,  0x101,  0x0220,
    REG15,  0x102,  0x00bd,
    REG15,  0x103,  0x1500,
    REG15,  0x105,  0x0103,
    REG15,  0x106,  0x0105,

    /* Display On */
    REG15,  0x007,  0x0021,
    REG15,  0x001,  0x0110,
    REG15,  0x003,  0x0230,
    REG15,  0x002,  0x0500,
    REG15,  0x007,  0x0031,
    REG15,  0x030,  0x0007,
    SLEEP,  0x003,
    REG15,  0x030,  0x03ff,
    SLEEP,  0x006,
    REG15,  0x007,  0x0072,
    SLEEP,  0x00f,
    REG15,  0x007,  0x0173,
    END
};
#endif

/* DMA configuration */

/* one single transfer at once, needed LLIs:
 *   screen_size / (DMAC_LLI_MAX_COUNT << swidth) =
 *   (320*240*2) / (4095*2) = 19
 */
#define LCD_DMA_TSKBUF_SZ   1   /* N tasks, MUST be pow2 */
#define LCD_DMA_LLIBUF_SZ   32  /* N LLIs, MUST be pow2 */

static struct dmac_tsk lcd_dma_tskbuf[LCD_DMA_TSKBUF_SZ];
static struct dmac_lli volatile \
            lcd_dma_llibuf[LCD_DMA_LLIBUF_SZ] CACHEALIGN_ATTR;

static struct dmac_ch lcd_dma_ch = {
    .dmac = &s5l8702_dmac0,
    .prio = DMAC_CH_PRIO(4),
    .cb_fn = NULL,

    .tskbuf = lcd_dma_tskbuf,
    .tskbuf_mask = LCD_DMA_TSKBUF_SZ - 1,
    .queue_mode = QUEUE_NORMAL,

    .llibuf = lcd_dma_llibuf,
    .llibuf_mask = LCD_DMA_LLIBUF_SZ - 1,
    .llibuf_bus = DMAC_MASTER_AHB1,
};

static struct dmac_ch_cfg lcd_dma_ch_cfg = {
    .srcperi = S5L8702_DMAC0_PERI_MEM,
    .dstperi = S5L8702_DMAC0_PERI_LCD_WR,
    .sbsize  = DMACCxCONTROL_BSIZE_1,
    .dbsize  = DMACCxCONTROL_BSIZE_1,
    .swidth  = DMACCxCONTROL_WIDTH_16,
    .dwidth  = DMACCxCONTROL_WIDTH_16,
    .sbus    = DMAC_MASTER_AHB1,
    .dbus    = DMAC_MASTER_AHB1,
    .sinc    = DMACCxCONTROL_INC_ENABLE,
    .dinc    = DMACCxCONTROL_INC_DISABLE,
    .prot    = DMAC_PROT_CACH | DMAC_PROT_BUFF | DMAC_PROT_PRIV,
    .lli_xfer_max_count = DMAC_LLI_MAX_COUNT,
};

static inline void s5l_lcd_write_reg(int cmd, unsigned int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;

    while (LCD_STATUS & 0x10);
    /* 16-bit/1-transfer data format (ili9320ds s7.2.2) */
    LCD_WDATA = (data & 0x78ff) |
                ((data & 0x0300) << 1) | ((data & 0x0400) << 5);
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_data(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

static void lcd_run_sequence(const unsigned short *seq)
{
    unsigned short tmp;

    while (1) switch (*seq++)
    {
        case SLEEP:
            sleep(*seq++);
            break;
        case CMD16:
            s5l_lcd_write_cmd(*seq++);
            break;
        case DATA16:
            s5l_lcd_write_data(*seq++);
            break;
        case REG15:
            tmp = *seq++;   /* avoid compiler warning */
            s5l_lcd_write_reg(tmp, *seq++);
            break;
        case END:
        default:
            /* bye */
            return;
     }
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

void lcd_set_flip(bool yesno)
{
    (void)yesno;
}

bool lcd_active(void)
{
    return lcd_ispowered;
}

void lcd_shutdown(void)
{
    pmu_write(0x2b, 0);  /* Kill the backlight, instantly. */
    pmu_write(0x29, 0);

    lcd_sleep();
}

void lcd_sleep(void)
{
    mutex_lock(&lcd_mutex);

    lcd_run_sequence((lcd_type & 2) ? lcd_deep_stby_sequence_23
                                    : lcd_sleep_sequence_01);

    /* mask lcd controller clock gate */
    PWRCON(0) |= (1 << 1);

    lcd_ispowered = false;

    mutex_unlock(&lcd_mutex);
}

#ifdef HAVE_LCD_SLEEP
void lcd_awake(void)
{
    mutex_lock(&lcd_mutex);

    /* unmask lcd controller clock gate */
    PWRCON(0) &= ~(1 << 1);

    if (lcd_type & 2) {
        /* release from deep standby mode (ili9320ds s12.3) */
        for (int i = 0; i < 6; i++) {
            s5l_lcd_write_cmd(0x000);
            udelay(1000);
        }

        lcd_run_sequence(lcd_init_sequence_23);
    }
    else
        lcd_run_sequence(lcd_init_sequence_01);

    lcd_ispowered = true;

    mutex_unlock(&lcd_mutex);

    send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

/* LCD init */
void lcd_init_device(void)
{
    /* Detect lcd type */
    mutex_init(&lcd_mutex);
    lcd_type = (PDAT6 & 0x30) >> 4;
    while (!(LCD_STATUS & 0x2));
    LCD_CONFIG = 0x80100db0;

    /* Configure DMA channel */
    dmac_ch_init(&lcd_dma_ch, &lcd_dma_ch_cfg);

    lcd_ispowered = true;
}

/*** Update functions ***/

static inline void lcd_write_pixel(fb_data pixel)
{
    mutex_lock(&lcd_mutex);
    LCD_WDATA = pixel;
    mutex_unlock(&lcd_mutex);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper function. */
extern void lcd_write_line(const fb_data *addr, 
                           int pixelcount,
                           const unsigned int lcd_base_addr);

static void displaylcd_setup(int x, int y, int width, int height) ICODE_ATTR;
static void displaylcd_setup(int x, int y, int width, int height)
{
    /* TODO: ISR()->panicf()->lcd_update() blocks forever */
    mutex_lock(&lcd_mutex);
    while (dmac_ch_running(&lcd_dma_ch))
        yield();

    int xe = (x + width) - 1;           /* max horiz */
    int ye = (y + height) - 1;          /* max vert */

    if (lcd_type & 2) {
        s5l_lcd_write_reg(R_HORIZ_ADDR_START_POS, x);
        s5l_lcd_write_reg(R_HORIZ_ADDR_END_POS,   xe);
        s5l_lcd_write_reg(R_VERT_ADDR_START_POS,  y);
        s5l_lcd_write_reg(R_VERT_ADDR_END_POS,    ye);

        s5l_lcd_write_reg(R_HORIZ_GRAM_ADDR_SET,  x);
        s5l_lcd_write_reg(R_VERT_GRAM_ADDR_SET,   y);

        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(x >> 8);
        s5l_lcd_write_data(x & 0xff);
        s5l_lcd_write_data(xe >> 8);
        s5l_lcd_write_data(xe & 0xff);

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(y >> 8);
        s5l_lcd_write_data(y & 0xff);
        s5l_lcd_write_data(ye >> 8);
        s5l_lcd_write_data(ye & 0xff);

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }
}

static void displaylcd_dma(int pixels) ICODE_ATTR;
static void displaylcd_dma(int pixels)
{
    commit_dcache();
    dmac_ch_queue(&lcd_dma_ch, lcd_dblbuf,
            (void*)S5L8702_DADDR_PERI_LCD_WR, pixels*2, NULL);
    mutex_unlock(&lcd_mutex);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int pixels = width * height;
    fb_data* p = FBADDR(x,y);
    uint16_t* out = lcd_dblbuf[0];
    
#ifdef HAVE_LCD_SLEEP
    if (!lcd_active()) return;
#endif

    displaylcd_setup(x, y, width, height);

    /* Copy display bitmap to hardware */
    if (LCD_WIDTH == width) {
        /* Write all lines at once */
        memcpy(out, p, pixels * 2);
    } else {
        do {
            /* Write a single line */
            memcpy(out, p, width * 2);
            p += LCD_WIDTH;
            out += width;
        } while (--height);
    }

    displaylcd_dma(pixels);
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   uint16_t* outbuf,
                                   int width,
                                   int stride);

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height) ICODE_ATTR;
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned int z;
    unsigned char const * yuv_src[3];
    
#ifdef HAVE_LCD_SLEEP
    if (!lcd_active()) return;
#endif

    width = (width + 1) & ~1;       /* ensure width is even */

    int pixels = width * height;
    uint16_t* out = lcd_dblbuf[0];

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    displaylcd_setup(x, y, width, height);

    height >>= 1;

    do {
        lcd_write_yuv420_lines(yuv_src, out, width, stride);
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
        out += width << 1;
    } while (--height);

    displaylcd_dma(pixels);
}
