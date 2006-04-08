/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Rockbox driver for iPod LCDs
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in November 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/fb.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"


/* check if number of useconds has past */
static inline bool timer_check(int clock_start, int usecs)
{
    return ((int)(USEC_TIMER - clock_start)) >= usecs;
}

#if (CONFIG_LCD == LCD_IPOD2BPP)

/*** hardware configuration ***/

#if CONFIG_CPU == PP5002
#define IPOD_LCD_BASE            0xc0001000
#define IPOD_LCD_BUSY_MASK       0x80000000
#else /* PP5020 */
#define IPOD_LCD_BASE            0x70003000
#define IPOD_LCD_BUSY_MASK       0x00008000
#endif

/* LCD command codes for HD66753 */

#define LCD_CMD  0x08
#define LCD_DATA 0x10

#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_WAVEFORM_CONTROL  0x02
#define R_POWER_CONTROL         0x03
#define R_CONTRAST_CONTROL      0x04
#define R_ENTRY_MODE            0x05
#define R_ROTATION              0x06
#define R_DISPLAY_CONTROL       0x07
#define R_CURSOR_CONTROL        0x08
#define R_HORIZONTAL_CURSOR_POS 0x0b
#define R_VERTICAL_CURSOR_POS   0x0c
#define R_1ST_SCR_DRV_POS       0x0d
#define R_2ND_SCR_DRV_POS       0x0e
#define R_RAM_WRITE_MASK        0x10
#define R_RAM_ADDR_SET          0x11
#define R_RAM_DATA              0x12

/* needed for flip */
static int addr_offset;
#if defined(IPOD_MINI) || defined(IPOD_MINI2G)
static int pix_offset;
#endif

static const unsigned char dibits[16] ICONST_ATTR = {
    0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
    0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

/* wait for LCD with timeout */
static inline void lcd_wait_write(void)
{
    int start = USEC_TIMER;

    do {
        if ((inl(IPOD_LCD_BASE) & 0x8000) == 0) break;
    } while (timer_check(start, 1000) == 0);
}


/* send LCD data */
static void lcd_send_data(unsigned data)
{
    lcd_wait_write();
#ifdef IPOD_MINI2G
    outl((inl(IPOD_LCD_BASE) & ~0x1f00000) | 0x1700000, IPOD_LCD_BASE);
    outl(data | 0x760000, IPOD_LCD_BASE+8);
#else
    outl(data >> 8, IPOD_LCD_BASE + LCD_DATA);
    lcd_wait_write();
    outl(data & 0xff, IPOD_LCD_BASE + LCD_DATA);
#endif
}

/* send LCD command */
static void lcd_prepare_cmd(unsigned cmd)
{
    lcd_wait_write();
#ifdef IPOD_MINI2G
    outl((inl(IPOD_LCD_BASE) & ~0x1f00000) | 0x1700000, IPOD_LCD_BASE);
    outl(cmd | 0x740000, IPOD_LCD_BASE+8);
#else
    outl(0x0, IPOD_LCD_BASE + LCD_CMD);
    lcd_wait_write();
    outl(cmd, IPOD_LCD_BASE + LCD_CMD);
#endif
}

/* send LCD command and data */
static void lcd_cmd_and_data(unsigned cmd, unsigned data)
{
    lcd_prepare_cmd(cmd);
    lcd_send_data(data);
}

/* LCD init */
void lcd_init_device(void)
{
    lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0015);
    lcd_set_flip(false);
    lcd_cmd_and_data(R_ENTRY_MODE, 0x0000);

#ifdef IPOD_4G
    outl(inl(0x6000d004) | 0x4, 0x6000d004); /* B02 enable */
    outl(inl(0x6000d004) | 0x8, 0x6000d004); /* B03 enable */
    outl(inl(0x70000084) | 0x2000000, 0x70000084); /* D01 enable */
    outl(inl(0x70000080) | 0x2000000, 0x70000080); /* D01 =1 */

    outl(inl(0x6000600c) | 0x20000, 0x6000600c);    /* PWM enable */
#endif
}

/*** hardware configuration ***/

/* Rockbox stores the contrast as 0..63 - we add 64 to it */
void lcd_set_contrast(int val)
{
    if (val < 0) val = 0;
    else if (val > 63) val = 63;

    lcd_cmd_and_data(R_CONTRAST_CONTROL, 0x400 | (val + 64));
}

void lcd_set_invert_display(bool yesno)
{
    if (yesno)
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0017);
    else
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0015);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
#if defined(IPOD_MINI) || defined(IPOD_MINI2G)
    if (yesno) {
         /* 168x112, inverse COM order */
        lcd_cmd_and_data(R_DRV_OUTPUT_CONTROL, 0x020d);
        lcd_cmd_and_data(R_1ST_SCR_DRV_POS, 0x8316);    /* 22..131 */
        addr_offset = (22 << 5) | (20 - 4);
        pix_offset = -2;
    } else {
        /* 168x112,  inverse SEG order */
        lcd_cmd_and_data(R_DRV_OUTPUT_CONTROL, 0x010d);
        lcd_cmd_and_data(R_1ST_SCR_DRV_POS, 0x6d00);    /* 0..109 */
        addr_offset = 20;
        pix_offset = 0;
    }
#else
    if (yesno) {
        /* 168x128, inverse SEG & COM order */
        lcd_cmd_and_data(R_DRV_OUTPUT_CONTROL, 0x030f);
        lcd_cmd_and_data(R_1ST_SCR_DRV_POS, 0x8304);    /* 4..131 */
        addr_offset = (4 << 5) | (20 - 1);
    } else {
        /* 168x128 */
        lcd_cmd_and_data(R_DRV_OUTPUT_CONTROL, 0x000f);
        lcd_cmd_and_data(R_1ST_SCR_DRV_POS, 0x7f00);    /* 0..127 */
        addr_offset = 20;
    }
#endif
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that x, bwidtht and stride are in 8-pixel units! */
void lcd_blit(const unsigned char* data, int bx, int y, int bwidth,
              int height, int stride)
{
    const unsigned char *src, *src_end;

    while (height--) {
        src = data;
        src_end = data + bwidth;
        lcd_cmd_and_data(R_RAM_ADDR_SET, (y++ << 5) + addr_offset - bx);
        lcd_prepare_cmd(R_RAM_DATA);
        do {
            unsigned byte = *src++;
            lcd_send_data((dibits[byte>>4] << 8) | dibits[byte&0x0f]);
        } while (src < src_end);
        data += stride;
    }
}

void lcd_update_rect(int x, int y, int width, int height)
{
    int xmax, ymax;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return;
    
    ymax = y + height - 1;
    if (ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT - 1;

#if defined(IPOD_MINI) || defined(IPOD_MINI2G)
    x += pix_offset;
#endif
     /* writing is done in 16-bit units (8 pixels) */
    xmax = (x + width - 1) >> 3;
    x >>= 3;
    width = xmax - x + 1;

    for (; y <= ymax; y++) {
        unsigned char *data, *data_end;

        lcd_cmd_and_data(R_RAM_ADDR_SET, (y << 5) + addr_offset - x);
        lcd_prepare_cmd(R_RAM_DATA);

        data = &lcd_framebuffer[y][2*x];
        data_end = data + 2 * width;
#if defined(IPOD_MINI) || defined(IPOD_MINI2G)
        if (pix_offset == -2) {
            unsigned cur_word = *data++;
            do {
                cur_word = (cur_word << 8) | *data++;
                cur_word = (cur_word << 8) | *data++;
                lcd_send_data((cur_word >> 4) & 0xffff);
            } while (data <= data_end);
        } else
#endif
        {
            do {
                unsigned highbyte = *data++;
                lcd_send_data((highbyte << 8) | *data++);
            } while (data < data_end);
        }
    }
}

/* Update the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#else

#define IPOD_LCD_BASE           0x70008a0c
#define IPOD_LCD_BUSY_MASK      0x80000000

/* LCD command codes for HD66789R */
#define LCD_CNTL_RAM_ADDR_SET           0x21
#define LCD_CNTL_WRITE_TO_GRAM          0x22
#define LCD_CNTL_HORIZ_RAM_ADDR_POS     0x44
#define LCD_CNTL_VERT_RAM_ADDR_POS      0x45

/*** globals ***/
static int lcd_type = 1; /* 0 = "old" Color/Photo, 1 = "new" Color & Nano */

static void lcd_wait_write(void)
{
    if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) != 0) {
        int start = USEC_TIMER;

        do {
            if ((inl(IPOD_LCD_BASE) & IPOD_LCD_BUSY_MASK) == 0) break;
        } while (timer_check(start, 1000) == 0);
    }
}

static void lcd_send_lo(int v)
{
    lcd_wait_write();
    outl(v | 0x80000000, IPOD_LCD_BASE);
}

static void lcd_send_hi(int v)
{
    lcd_wait_write();
    outl(v | 0x81000000, IPOD_LCD_BASE);
}

static void lcd_cmd_data(int cmd, int data)
{
    if (lcd_type == 0) {
        lcd_send_lo(cmd);
        lcd_send_lo(data);
    } else {
        lcd_send_lo(0x0);
        lcd_send_lo(cmd);
        lcd_send_hi((data >> 8) & 0xff);
        lcd_send_hi(data & 0xff);
    }
}

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
  /* TODO: Implement lcd_set_contrast() */
  (void)val;
}

void lcd_set_invert_display(bool yesno)
{
  /* TODO: Implement lcd_set_invert_display() */
  (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
  /* TODO: Implement lcd_set_flip() */
  (void)yesno;
}

/* LCD init */
void lcd_init_device(void)
{  
#if CONFIG_LCD == LCD_IPODCOLOR
    if (ipod_hw_rev == 0x60000) {
        lcd_type = 0;
    } else {
        int gpio_a01, gpio_a04;

        /* A01 */
        gpio_a01 = (inl(0x6000D030) & 0x2) >> 1;
        /* A04 */
        gpio_a04 = (inl(0x6000D030) & 0x10) >> 4;

        if (((gpio_a01 << 1) | gpio_a04) == 0 || ((gpio_a01 << 1) | gpio_a04) == 2) {
            lcd_type = 0;
        } else {
            lcd_type = 1;
        }
    }

    outl(inl(0x6000d004) | 0x4, 0x6000d004); /* B02 enable */
    outl(inl(0x6000d004) | 0x8, 0x6000d004); /* B03 enable */
    outl(inl(0x70000084) | 0x2000000, 0x70000084); /* D01 enable */
    outl(inl(0x70000080) | 0x2000000, 0x70000080); /* D01 =1 */

    outl(inl(0x6000600c) | 0x20000, 0x6000600c);    /* PWM enable */

#elif CONFIG_LCD == LCD_IPODNANO
    /* iPodLinux doesn't appear have any LCD init code for the Nano */
#endif
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 4-pixel units! */
void lcd_blit(const fb_data* data, int x, int by, int width,
              int bheight, int stride)
{
    /* TODO: Implement lcd_blit() */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    int newx,newwidth;

    unsigned long *addr = (unsigned long *)lcd_framebuffer;

    /* Ensure x and width are both even - so we can read 32-bit aligned 
       data from lcd_framebuffer */
    newx=x&~1;
    newwidth=width&~1;
    if (newx+newwidth < x+width) { newwidth+=2; }
    x=newx; width=newwidth;

    /* calculate the drawing region */
#if CONFIG_LCD == LCD_IPODNANO
    y0 = x;                         /* start horiz */
    x0 = y;                         /* start vert */
    y1 = (x + width) - 1;           /* max horiz */
    x1 = (y + height) - 1;          /* max vert */
#elif CONFIG_LCD == LCD_IPODCOLOR
    y0 = y;                         /* start vert */
    x0 = (LCD_WIDTH - 1) - x;       /* start horiz */
    y1 = (y + height) - 1;          /* end vert */
    x1 = (x0 - width) + 1;          /* end horiz */
#endif
    /* setup the drawing region */
    if (lcd_type == 0) {
        lcd_cmd_data(0x12, y0);      /* start vert */
        lcd_cmd_data(0x13, x0);      /* start horiz */
        lcd_cmd_data(0x15, y1);      /* end vert */
        lcd_cmd_data(0x16, x1);      /* end horiz */
    } else {
        /* swap max horiz < start horiz */
        if (y1 < y0) {
            int t;
            t = y0;
            y0 = y1;
            y1 = t;
        }

        /* swap max vert < start vert */
        if (x1 < x0) {
            int t;
            t = x0;
            x0 = x1;
            x1 = t;
        }

        /* max horiz << 8 | start horiz */
        lcd_cmd_data(LCD_CNTL_HORIZ_RAM_ADDR_POS, (y1 << 8) | y0);
        /* max vert << 8 | start vert */
        lcd_cmd_data(LCD_CNTL_VERT_RAM_ADDR_POS, (x1 << 8) | x0);

        /* start vert = max vert */
#if CONFIG_LCD == LCD_IPODCOLOR
        x0 = x1;
#endif

        /* position cursor (set AD0-AD15) */
        /* start vert << 8 | start horiz */
        lcd_cmd_data(LCD_CNTL_RAM_ADDR_SET, ((x0 << 8) | y0));

        /* start drawing */
        lcd_send_lo(0x0);
        lcd_send_lo(LCD_CNTL_WRITE_TO_GRAM);
    }

    addr = (unsigned long*)&lcd_framebuffer[y][x];

    while (height > 0) {
        int c, r;
        int h, pixels_to_write;

        pixels_to_write = (width * height) * 2;
        h = height;

        /* calculate how much we can do in one go */
        if (pixels_to_write > 64000) {
            h = (64000/2) / width;
            pixels_to_write = (width * h) * 2;
        }

        outl(0x10000080, 0x70008a20);
        outl((pixels_to_write - 1) | 0xc0010000, 0x70008a24);
        outl(0x34000000, 0x70008a20);

        /* for each row */
        for (r = 0; r < h; r++) {
            /* for each column */
            for (c = 0; c < width; c += 2) {
                while ((inl(0x70008a20) & 0x1000000) == 0);

                /* output 2 pixels */
                outl(*(addr++), 0x70008b00);
            }

            addr += (LCD_WIDTH - width)/2;
        }

        while ((inl(0x70008a20) & 0x4000000) == 0);

        outl(0x0, 0x70008a24);

        height = height - h;
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#endif
