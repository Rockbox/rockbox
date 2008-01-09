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
#include "hwcompat.h"

/* LCD command codes for HD66753 */

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

#ifdef HAVE_BACKLIGHT_INVERSION
/* The backlight makes the LCD appear negative on the 1st/2nd gen */
static bool lcd_inverted = false;
static bool lcd_backlit = false;
static void invert_display(void);
#endif

#if defined(IPOD_1G2G) || defined(IPOD_3G)
static unsigned short power_reg_h;
#define POWER_REG_H power_reg_h
#else
#define POWER_REG_H 0x1200
#endif

#ifdef IPOD_1G2G
static unsigned short contrast_reg_h;
#define CONTRAST_REG_H contrast_reg_h
#else
#define CONTRAST_REG_H 0x400
#endif

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
    while (LCD1_CONTROL & LCD1_BUSY_MASK);
}

/* send LCD data */
#if CONFIG_CPU == PP5002
STATICIRAM void ICODE_ATTR lcd_send_data(unsigned data)
#else
static void lcd_send_data(unsigned data)
#endif
{
    lcd_wait_write();
#ifdef IPOD_MINI2G
    LCD1_CMD = data | 0x760000;
#else
    LCD1_DATA = data >> 8;
    lcd_wait_write();
    LCD1_DATA = data & 0xff;
#endif
}

/* send LCD command */
static void lcd_prepare_cmd(unsigned cmd)
{
    lcd_wait_write();
#ifdef IPOD_MINI2G
    LCD1_CMD = cmd | 0x740000;
#else
    LCD1_CMD = 0;
    lcd_wait_write();
    LCD1_CMD = cmd;
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
#ifdef IPOD_1G2G
    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        power_reg_h = 0x1500;
        contrast_reg_h = 0x700;
    }
    else /* 2nd gen */
    {
        if (inl(0xcf00404c) & 0x01) /* check bit 0 */
        {
            power_reg_h = 0x1520;   /* Set step-up frequency to f/8 instead of
                                     * f/32, for better blacklevel stability */
            contrast_reg_h = 0x400;
        }
        else
        {
            power_reg_h = 0x1100;
            contrast_reg_h = 0x300;
        }
    }
#elif defined IPOD_3G
    if (inl(0xcf00404c) & 0x01) /* check bit 0 */
        power_reg_h = 0x1520;   /* Set step-up frequency to f/8 instead of
                                 * f/32, for better blacklevel stability */
    else
        power_reg_h = 0x1100;
#endif

#ifdef IPOD_MINI2G /* serial LCD hookup */
    lcd_wait_write();
    LCD1_CONTROL = 0x01730084; /* fastest setting */
#elif defined(IPOD_1G2G) || defined(IPOD_3G)
    LCD1_CONTROL = (LCD1_CONTROL & 0x0002) | 0x0084; /* fastest setting, keep backlight bit */
#else
    LCD1_CONTROL = 0x0084; /* fastest setting */
#endif

    lcd_cmd_and_data(R_POWER_CONTROL, POWER_REG_H | 0xc);
#ifdef HAVE_BACKLIGHT_INVERSION
    invert_display();
#else
    lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0015);
#endif
    lcd_set_flip(false);
    lcd_cmd_and_data(R_ENTRY_MODE, 0x0000);
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
#ifdef IPOD_1G2G
    return 28;
#elif defined(IPOD_MINI) || defined(IPOD_MINI2G) || defined(IPOD_3G)
    return 42;
#elif defined(IPOD_4G)
    return 35;
#endif
}

/* Rockbox stores the contrast as 0..63 - we add 64 to it */
void lcd_set_contrast(int val)
{
    if (val < 0) val = 0;
    else if (val > 63) val = 63;

    lcd_cmd_and_data(R_CONTRAST_CONTROL, CONTRAST_REG_H | (val + 64));
}

#ifdef HAVE_BACKLIGHT_INVERSION
static void invert_display(void)
{
    if (lcd_inverted ^ lcd_backlit)
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0017);
    else
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0015);
}

void lcd_set_invert_display(bool yesno)
{
    lcd_inverted = yesno;
    invert_display();
}

void lcd_set_backlight_inversion(bool yesno)
{
    lcd_backlit = yesno;
    invert_display();
}
#else
void lcd_set_invert_display(bool yesno)
{
    if (yesno)
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0017);
    else
        lcd_cmd_and_data(R_DISPLAY_CONTROL, 0x0015);
}
#endif

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

void lcd_enable(bool on)
{
    if (on)
    {
        lcd_cmd_and_data(R_START_OSC, 1);               /* start oscillation */
        sleep(HZ/10);                                           /* wait 10ms */
        lcd_cmd_and_data(R_POWER_CONTROL, POWER_REG_H); /*clear standby mode */
        lcd_cmd_and_data(R_POWER_CONTROL, POWER_REG_H | 0xc);
                                                   /* enable opamp & booster */
    }
    else
    {
        lcd_cmd_and_data(R_POWER_CONTROL, POWER_REG_H);
                                               /* switch off opamp & booster */
        lcd_cmd_and_data(R_POWER_CONTROL, POWER_REG_H | 0x1);
                                                       /* enter standby mode */
    }
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

/* Performance function that works with an external buffer
   note that bx and bwidth are in 8-pixel units! */
void lcd_grey_phase_blit(unsigned char *values, unsigned char *phases,
                         int bx, int y, int bwidth, int height, int stride)
{
    unsigned char *val, *ph;
    int bw;

    while (height--) {
        lcd_cmd_and_data(R_RAM_ADDR_SET, (y++ << 5) + addr_offset - bx);
        lcd_prepare_cmd(R_RAM_DATA);

        val = values;
        ph = phases;
        bw = bwidth;
        asm volatile (
        "10:                                 \n"
            "ldmia   %[ph], {r0-r1}          \n" /* Fetch 8 pixel phases */
            "ldmia   %[val]!, {r2-r3}        \n" /* Fetch 8 pixel values */
#ifdef IPOD_MINI2G
            "mov     r4, #0x7600             \n"
#else
            "mov     r4, #0                  \n"
#endif
            "tst     r0, #0x80               \n"
            "orreq   r4, r4, #0xc0           \n"
            "tst     r0, #0x8000             \n"
            "orreq   r4, r4, #0x30           \n"
            "tst     r0, #0x800000           \n"
            "orreq   r4, r4, #0x0c           \n"
            "tst     r0, #0x80000000         \n"
            "orreq   r4, r4, #0x03           \n"
            "bic     r0, r0, %[clbt]         \n"
            "add     r0, r0, r2              \n"

#ifdef IPOD_MINI2G
            "mov     r4, r4, lsl #8          \n"
#else
        "1:                                  \n"
            "ldr     r2, [%[lcdb]]           \n"
            "tst     r2, #0x8000             \n"
            "bne     1b                      \n"

            "str     r4, [%[lcdb], #0x10]    \n"
            "mov     r4, #0                  \n"
#endif

            "tst     r1, #0x80               \n"
            "orreq   r4, r4, #0xc0           \n"
            "tst     r1, #0x8000             \n"
            "orreq   r4, r4, #0x30           \n"
            "tst     r1, #0x800000           \n"
            "orreq   r4, r4, #0x0c           \n"
            "tst     r1, #0x80000000         \n"
            "orreq   r4, r4, #0x03           \n"
            "bic     r1, r1, %[clbt]         \n"
            "add     r1, r1, r3              \n"

            "stmia   %[ph]!, {r0-r1}         \n"

        "1:                                  \n"
            "ldr     r2, [%[lcdb]]           \n"
            "tst     r2, #0x8000             \n"
            "bne     1b                      \n"
#ifdef IPOD_MINI2G
            "str     r4, [%[lcdb], #0x08]    \n"
#else
            "str     r4, [%[lcdb], #0x10]    \n"
#endif

            "subs    %[bw], %[bw], #1        \n"
            "bne     10b                     \n"
            : /* outputs */
            [val]"+r"(val),
            [ph] "+r"(ph),
            [bw] "+r"(bw)
            : /* inputs */
            [clbt]"r"(0x80808080),
            [lcdb]"r"(LCD1_BASE)
            : /* clobbers */
            "r0", "r1", "r2", "r3", "r4"
        );
        values += stride;
        phases += stride;
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

#ifdef HAVE_LCD_SHUTDOWN
/* LCD powerdown */
void lcd_shutdown(void)
{
    lcd_cmd_and_data(R_POWER_CONTROL, 0x1500); /* Turn off op amp power */
    lcd_cmd_and_data(R_POWER_CONTROL, 0x1502); /* Put LCD driver in standby */
}
#endif
