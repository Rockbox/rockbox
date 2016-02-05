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

#define CMD         0   /* send command with N data */
#define MREG        1   /* write multiple registers */
#define SLEEP       2
#define END         0xff

#define CMD16(len)  (CMD | ((len) << 8))
#define MREG16(len) (MREG | ((len) << 8))
#define SLEEP16(t)  (SLEEP | ((t) << 8))

/* powersave sequences */

static const unsigned char lcd_sleep_seq_01[] =
{
    CMD,   0x28,  0,  /* Display Off */
    SLEEP, 5,         /* 50 ms */
    CMD,   0x10,  0,  /* Sleep In Mode */
    SLEEP, 5,         /* 50 ms */
    END
};

static const unsigned short lcd_enter_deepstby_seq_23[] =
{
    /* Display Off */
    MREG16(1),  0x007, 0x0172,
    MREG16(1),  0x030, 0x03ff,
    SLEEP16(9),
    MREG16(1),  0x007, 0x0120,
    MREG16(1),  0x030, 0x0000,
    MREG16(1),  0x100, 0x0780,
    MREG16(1),  0x007, 0x0000,
    MREG16(1),  0x101, 0x0260,
    MREG16(1),  0x102, 0x00a9,
    SLEEP16(3),
    MREG16(1),  0x100, 0x0700,

    /* Deep Standby Mode */
    MREG16(1),  0x100, 0x0704,
    SLEEP16(5),
    END
};

/* init sequences */

#ifdef HAVE_LCD_SLEEP
static const unsigned char lcd_awake_seq_01[] =
{
    CMD,   0x11,  0,  /* Sleep Out Mode */
    SLEEP, 6,         /* 60 ms */
    CMD,   0x29,  0,  /* Display On */
    END
};
#endif

#if defined(HAVE_LCD_SLEEP) || defined(BOOTLOADER)
static const unsigned short lcd_init_seq_23[] =
{
    /* Display settings */
    MREG16(1),  0x008, 0x0808,
    MREG16(7),  0x010, 0x0013, 0x0300, 0x0101, 0x0a03, 0x0a0e, 0x0a19, 0x2402,
    MREG16(1),  0x018, 0x0001,
    MREG16(1),  0x090, 0x0021,

    /* Gamma settings */
    MREG16(14), 0x300, 0x0307, 0x0003, 0x0402, 0x0303, 0x0300, 0x0407, 0x1c04,
                       0x0307, 0x0003, 0x0402, 0x0303, 0x0300, 0x0407, 0x1c04,
    MREG16(14), 0x310, 0x0707, 0x0407, 0x0306, 0x0303, 0x0300, 0x0407, 0x1c01,
                       0x0707, 0x0407, 0x0306, 0x0303, 0x0300, 0x0407, 0x1c01,
    MREG16(14), 0x320, 0x0206, 0x0102, 0x0404, 0x0303, 0x0300, 0x0407, 0x1c1f,
                       0x0206, 0x0102, 0x0404, 0x0303, 0x0300, 0x0407, 0x1c1f,

    /* GRAM and Base Imagen settings (ili9326ds) */
    MREG16(2),  0x400, 0x001d, 0x0001,
    MREG16(1),  0x205, 0x0060,

    /* Power settings */
    MREG16(1),  0x007, 0x0001,
    MREG16(1),  0x031, 0x0071,
    MREG16(1),  0x110, 0x0001,
    MREG16(6),  0x100, 0x17b0, 0x0220, 0x00bd, 0x1500, 0x0103, 0x0105,

    /* Display On */
    MREG16(1),  0x007, 0x0021,
    MREG16(1),  0x001, 0x0110,
    MREG16(1),  0x003, 0x0230,
    MREG16(1),  0x002, 0x0500,
    MREG16(1),  0x007, 0x0031,
    MREG16(1),  0x030, 0x0007,
    SLEEP16(3),
    MREG16(1),  0x030, 0x03ff,
    SLEEP16(6),
    MREG16(1),  0x007, 0x0072,
    SLEEP16(15),
    MREG16(1),  0x007, 0x0173,
    END
};
#endif  /* HAVE_LCD_SLEEP || BOOTLOADER */

#ifdef BOOTLOADER
static const unsigned char lcd_init_seq_0[] =
{
    CMD,   0x11,  0,        /* Sleep Out Mode */
    SLEEP, 0x03,            /* 30 ms */
    CMD,   0x35,  1, 0x00,  /* TEON (TBC) */
    CMD,   0x3a,  1, 0x06,  /* COLMOD (TBC) */
    CMD,   0x36,  1, 0x00,  /* MADCTR (TBC) */
    CMD,   0x13,  0,        /* NORON: Normal Mode On (Partial
                               Mode Off, Scroll Mode Off) */
    CMD,   0x29,  0,        /* Display On */
    END
};

static const unsigned char lcd_init_seq_1[] =
{
    CMD,   0xb0, 21, 0x3a, 0x3a, 0x80, 0x80, 0x0a, 0x0a, 0x0a, 0x0a,
                     0x0a, 0x0a, 0x0a, 0x0a, 0x3c, 0x30, 0x0f, 0x00,
                     0x01, 0x54, 0x06, 0x66, 0x66,
    CMD,   0xb8,  1, 0xd8,
    CMD,   0xb1, 30, 0x14, 0x59, 0x00, 0x15, 0x57, 0x27, 0x04, 0x85,
                     0x14, 0x59, 0x00, 0x15, 0x57, 0x27, 0x04, 0x85,
                     0x14, 0x09, 0x15, 0x57, 0x27, 0x04, 0x05,
                     0x14, 0x09, 0x15, 0x57, 0x27, 0x04, 0x05,
    CMD,   0xd2,  1, 0x01,

    /* Gamma settings (TBC) */
    CMD,   0xe0, 13, 0x00, 0x00, 0x00, 0x05, 0x0b, 0x12, 0x16, 0x1f,
                     0x25, 0x22, 0x24, 0x29, 0x1c,
    CMD,   0xe1, 13, 0x08, 0x01, 0x01, 0x06, 0x0b, 0x11, 0x15, 0x1f,
                     0x27, 0x26, 0x29, 0x2f, 0x1e,
    CMD,   0xe2, 13, 0x07, 0x01, 0x01, 0x05, 0x09, 0x0f, 0x13, 0x1e,
                     0x26, 0x25, 0x28, 0x2e, 0x1e,
    CMD,   0xe3, 13, 0x00, 0x00, 0x00, 0x05, 0x0b, 0x12, 0x16, 0x1f,
                     0x25, 0x22, 0x24, 0x29, 0x1c,
    CMD,   0xe4, 13, 0x08, 0x01, 0x01, 0x06, 0x0b, 0x11, 0x15, 0x1f,
                     0x27, 0x26, 0x29, 0x2f, 0x1e,
    CMD,   0xe5, 13, 0x07, 0x01, 0x01, 0x05, 0x09, 0x0f, 0x13, 0x1e,
                     0x26, 0x25, 0x28, 0x2e, 0x1e,

    CMD,   0x3a,  1, 0x06,  /* COLMOD (TBC) */
    CMD,   0xc2,  1, 0x00,  /* Power Control 3 (TBC) */
    CMD,   0x35,  1, 0x00,  /* TEON (TBC) */
    CMD,   0x11,  0,        /* Sleep Out Mode */
    SLEEP, 0x06,            /* 60 ms */
    CMD,   0x13,  0,        /* NORON: Normal Mode On (Partial
                               Mode Off, Scroll Mode Off) */
    CMD,   0x29,  0,        /* Display On */
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

static void lcd_run_seq8(const unsigned char *seq)
{
    int n;

    while (1) switch (*seq++)
    {
        case CMD:
            s5l_lcd_write_cmd(*seq++);
            n = *seq++;
            while (n--)
                s5l_lcd_write_data(*seq++);
            break;
        case SLEEP:
            sleep(*seq++);
            break;
        case END:
        default:
            /* bye */
            return;
    }
}

static void lcd_run_seq16(const unsigned short *seq)
{
    int action, param;
    unsigned short reg;

    while (1)
    {
        action = *seq & 0xff;
        param = *seq++ >> 8;

        switch (action)
        {
            case MREG:
                reg = *seq++;
                while (param--)
                    s5l_lcd_write_reg(reg++, *seq++);
                break;
            case SLEEP:
                sleep(param);
                break;
            case END:
            default:
                /* bye */
                return;
        }
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

void lcd_powersave(void)
{
    mutex_lock(&lcd_mutex);

    if (lcd_type & 2)
        lcd_run_seq16(lcd_enter_deepstby_seq_23);
    else
        lcd_run_seq8(lcd_sleep_seq_01);

    /* mask lcd controller clock gate */
    PWRCON(0) |= (1 << CLOCKGATE_LCD);

    lcd_ispowered = false;

    mutex_unlock(&lcd_mutex);
}

void lcd_shutdown(void)
{
    pmu_write(0x2b, 0);  /* Kill the backlight, instantly. */
    pmu_write(0x29, 0);

    lcd_powersave();
}

#ifdef HAVE_LCD_SLEEP
void lcd_sleep(void)
{
    lcd_powersave();
}

void lcd_awake(void)
{
    mutex_lock(&lcd_mutex);

    /* unmask lcd controller clock gate */
    PWRCON(0) &= ~(1 << CLOCKGATE_LCD);

    if (lcd_type & 2) {
        /* release from deep standby mode (ili9320ds s12.3) */
        for (int i = 0; i < 6; i++) {
            s5l_lcd_write_cmd(0x000);
            udelay(1000);
        }

        lcd_run_seq16(lcd_init_seq_23);
    }
    else
        lcd_run_seq8(lcd_awake_seq_01);

    lcd_ispowered = true;

    mutex_unlock(&lcd_mutex);

    send_event(LCD_EVENT_ACTIVATION, NULL);
}
#endif

/* LCD init */
void lcd_init_device(void)
{
    mutex_init(&lcd_mutex);

    /* unmask lcd controller clock gate */
    PWRCON(0) &= ~(1 << CLOCKGATE_LCD);

    /* Detect lcd type */
    lcd_type = (PDAT6 & 0x30) >> 4;

    while (!(LCD_STATUS & 0x2));
    LCD_CONFIG = 0x80100db0;
    LCD_PHTIME = 0x33;

    /* Configure DMA channel */
    dmac_ch_init(&lcd_dma_ch, &lcd_dma_ch_cfg);

#ifdef BOOTLOADER
    switch (lcd_type) {
        case 0: lcd_run_seq8(lcd_init_seq_0); break;
        case 1: lcd_run_seq8(lcd_init_seq_1); break;
        default: lcd_run_seq16(lcd_init_seq_23); break;
    }
#endif

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
