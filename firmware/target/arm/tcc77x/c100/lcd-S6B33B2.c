/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Mark Arigo
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

/* Display status */
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;

/* LCD command set for Samsung S6B33B2 */

#define R_NOP                  0x00
#define R_OSCILLATION_MODE     0x02
#define R_DRIVER_OUTPUT_MODE   0x10
#define R_DCDC_SET             0x20
#define R_BIAS_SET             0x22
#define R_DCDC_CLOCK_DIV       0x24
#define R_DCDC_AMP_ONOFF       0x26
#define R_TEMP_COMPENSATION    0x28
#define R_CONTRAST_CONTROL1    0x2a
#define R_CONTRAST_CONTROL2    0x2b
#define R_STANDBY_OFF          0x2c
#define R_STANDBY_ON           0x2d
#define R_DDRAM_BURST_OFF      0x2e
#define R_DDRAM_BURST_ON       0x2f
#define R_ADDRESSING_MODE      0x30
#define R_ROW_VECTOR_MODE      0x32
#define R_N_LINE_INVERSION     0x34
#define R_FRAME_FREQ_CONTROL   0x36
#define R_RED_PALETTE          0x38
#define R_GREEN_PALETTE        0x3a
#define R_BLUE_PALETTE         0x3c
#define R_ENTRY_MODE           0x40
#define R_X_ADDR_AREA          0x42
#define R_Y_ADDR_AREA          0x43
#define R_RAM_SKIP_AREA        0x45
#define R_DISPLAY_OFF          0x50
#define R_DISPLAY_ON           0x51
#define R_SPEC_DISPLAY_PATTERN 0x53
#define R_PARTIAL_DISPLAY_MODE 0x55
#define R_PARTIAL_START_LINE   0x56
#define R_PARTIAL_END_LINE     0x57
#define R_AREA_SCROLL_MODE     0x59
#define R_SCROLL_START_LINE    0x5a
#define R_DATA_FORMAT_SELECT   0x60

/* TCC77x specific defines */
#define LCD_BASE  0x50000000
#define LCD_CMD   *(volatile unsigned char*)(LCD_BASE)
#define LCD_DATA  *(volatile unsigned char*)(LCD_BASE+1)

static void lcd_send_command(unsigned cmd)
{
    LCD_CMD = cmd;
        
    asm volatile (
        "nop      \n\t"
        "nop      \n\t"
        "nop      \n\t"
    );
}

static void lcd_send_data(unsigned data)
{
    LCD_DATA = (data & 0xff00) >> 8;
    LCD_DATA = (data & 0x00ff);
}

/* End of TCC77x specific defines */

/* LCD init */
void lcd_init_device(void)
{
    uint32_t bus_width;

    /* Telechips init the same as the original firmware */
    bus_width = ((MCFG >> 11) & 0x3) ^ 3;

    CSCFG1 = (bus_width << 28) |
             (3 << 26) |                 /* MTYPE = 3 */
             ((LCD_BASE >> 28) << 22) |  /* CSBASE = 0x5 */
             (1 << 20) |                 /* Unknown */
             (2 << 11) |                 /* Setup time = 2 cycles */
             (2 << 3) |                  /* Pulse width = 2+1 cycles */
             (2 << 0);                   /* Hold time = 2 cycle */
			 
    GPIOE &= ~0x8;
    sleep(HZ/100);   /* 10ms */

    GPIOE |= 0x08;
    sleep(HZ/100);   /* 10ms */
	
    lcd_send_command(R_STANDBY_OFF);
    sleep(HZ/20);    /* 50ms */

    lcd_send_command(R_OSCILLATION_MODE);
    lcd_send_command(0x01);
    sleep(HZ/100);   /* 10ms */

    lcd_send_command(R_DCDC_AMP_ONOFF);
    lcd_send_command(0x01);
    sleep(HZ/100);   /* 10ms */

    lcd_send_command(R_DCDC_AMP_ONOFF);
    lcd_send_command(0x09);
    sleep(HZ/100);   /* 10ms */

    lcd_send_command(R_DCDC_AMP_ONOFF);
    lcd_send_command(0x0b);
    sleep(HZ/100);   /* 10ms */

    lcd_send_command(R_DCDC_AMP_ONOFF);
    lcd_send_command(0x0f);
    sleep(HZ/100);   /* 10ms */

    lcd_send_command(R_DCDC_SET);
    lcd_send_command(0x01);
    sleep(HZ/100);     /* 10ms */
    sleep(HZ/10);      /* 100ms */

    lcd_send_command(R_TEMP_COMPENSATION);
    lcd_send_command(0x01);
    sleep(HZ/100);     /* 10ms */

    lcd_send_command(R_DRIVER_OUTPUT_MODE);
    lcd_send_command(0x03);

    lcd_send_command(R_ENTRY_MODE);
    lcd_send_command(0x81);

    lcd_send_command(R_N_LINE_INVERSION);
    lcd_send_command(0x04);
    lcd_send_command(0xfa);
    lcd_send_command(0x5f);

    lcd_set_contrast(0x28);

    lcd_send_command(R_SPEC_DISPLAY_PATTERN);
    lcd_send_command(0x0);
    sleep(HZ/100);     /* 10ms */

    lcd_send_command(R_ADDRESSING_MODE);
    lcd_send_command(0x0);
    sleep(HZ/100);     /* 10ms */

    lcd_send_command(R_PARTIAL_DISPLAY_MODE);
    lcd_send_command(0x0);
    sleep(HZ/100);     /* 10ms */

    lcd_send_command(R_X_ADDR_AREA);
    lcd_send_command(0);
    lcd_send_command(0x80);

    lcd_send_command(R_Y_ADDR_AREA);
    lcd_send_command(0x0);
    lcd_send_command(0x80);

    lcd_send_command(R_DISPLAY_ON);

    lcd_send_command(R_SPEC_DISPLAY_PATTERN);
    lcd_send_command(0x0);

    /* Rockbox init */
    lcd_clear_display();
    lcd_update();
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return 0x28;
}

void lcd_set_contrast(int val)
{
	//val &= 0xFF;
    lcd_send_command(R_CONTRAST_CONTROL1);
    lcd_send_command(val);
}

void lcd_set_invert_display(bool yesno)
{
    /* TODO: Implement lcd_set_invert_display() */
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    lcd_send_command(R_DRIVER_OUTPUT_MODE);
    lcd_send_command(yesno ? 0x02 : 0x07);
}

/*** update functions ***/

/*** update functions ***/

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;
    
    y += 0x1a;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    lcd_send_command(R_ENTRY_MODE);
    lcd_send_command(0x80);

    lcd_send_command(R_X_ADDR_AREA);
    lcd_send_command(x);
    lcd_send_command(x + width - 1);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_send_command(R_Y_ADDR_AREA);
            lcd_send_command(y);
            lcd_send_command(y + 1);

            /* NOP needed because on some c200s, the previous lcd_send_command
               is interpreted as a separate command instead of part of
               R_Y_ADDR_AREA. */
            lcd_send_command(R_NOP);

            lcd_write_yuv420_lines_odither(yuv_src, width, stride, x, y);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
         while (--height > 0);
    }
    else
    {
        do
        {
            lcd_send_command(R_Y_ADDR_AREA);
            lcd_send_command(y);
            lcd_send_command(y + 1);

            lcd_send_command(R_NOP);

            lcd_write_yuv420_lines(yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
         while (--height > 0);
    }
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;
    
    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    addr = &lcd_framebuffer[y][x];

    if (width <= 1) {                    
        lcd_send_command(R_ENTRY_MODE);  /* The X end address must be larger */
        lcd_send_command(0x80);          /* that the X start address, so we */
        lcd_send_command(R_X_ADDR_AREA); /* switch to vertical mode for */
        lcd_send_command(x);             /* single column updates and set */
        lcd_send_command(x + 1);         /* the window width to 2 */
    } else {
        lcd_send_command(R_ENTRY_MODE);
        lcd_send_command(0x82);
        lcd_send_command(R_X_ADDR_AREA);
        lcd_send_command(x);
        lcd_send_command(x + width - 1);
    }

    lcd_send_command(R_Y_ADDR_AREA);
    lcd_send_command(y);
    lcd_send_command(y + height - 1);

    /* NOP needed because on some c200s, the previous lcd_send_command is
       interpreted as a separate command instead of part of R_Y_ADDR_AREA. */
    lcd_send_command(R_NOP);

    do {
        int w = width;
        do {
            lcd_send_data(*addr++);
        } while (--w > 0);
        addr += LCD_WIDTH - width;
    } while (--height > 0);
}
