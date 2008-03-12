/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "file.h"
#include "lcd.h"
#include "scroll_engine.h"

/* The LCD in the iAudio M3/M5/X5 remote control is a Tomato LSI 0350 */

#define LCD_SET_DUTY_RATIO 0x48
#define LCD_SELECT_ADC     0xa0
#define LCD_SELECT_SHL     0xc0
#define LCD_SET_COM0       0x44
#define LCD_OSC_ON         0xab
#define LCD_SELECT_DCDC    0x64
#define LCD_SELECT_RES     0x20
#define LCD_SET_VOLUME     0x81
#define LCD_SET_BIAS       0x50
#define LCD_CONTROL_POWER  0x28
#define LCD_DISPLAY_ON     0xae
#define LCD_SET_INITLINE   0x40
#define LCD_SET_COLUMN     0x10
#define LCD_SET_PAGE       0xb0
#define LCD_SET_GRAY       0x88
#define LCD_SET_PWM_FRC    0x90
#define LCD_SET_POWER_SAVE 0xa8
#define LCD_REVERSE        0xa6

#define CS_LO      and_l(~0x00010000, &GPIO1_OUT)
#define CS_HI      or_l(  0x00010000, &GPIO1_OUT)
#define CLK_LO     and_l(~0x20000000, &GPIO_OUT)
#define CLK_HI     or_l(  0x20000000, &GPIO_OUT)
#define DATA_LO    and_l(~0x04000000, &GPIO_OUT)
#define DATA_HI    or_l(  0x04000000, &GPIO_OUT)
#define RS_LO      and_l(~0x00001000, &GPIO1_OUT)
#define RS_HI      or_l(  0x00001000, &GPIO1_OUT)

/* cached settings values */
static bool cached_invert = false;
static bool cached_flip = false;
static int cached_contrast = DEFAULT_CONTRAST_SETTING;

bool initialized = false;

static void lcd_tick(void);

/* Standard low-level byte writer. Requires CLK high on entry */
static inline void _write_byte(unsigned data)
{
    asm volatile (
        "move.l  (%[gpo0]), %%d0     \n"  /* Get current state of data line */
        "and.l   %[dbit], %%d0       \n"
        "beq.s   1f                  \n"  /*   and set it as previous-state bit */
        "bset    #8, %[data]         \n"  
    "1:                              \n"
        "move.l  %[data], %%d0       \n"  /* Compute the 'bit derivative', i.e. a value */
        "lsr.l   #1, %%d0            \n"  /*   with 1's where the data changes from the */
        "eor.l   %%d0, %[data]       \n"  /*   previous state, and 0's where it doesn't */
        "swap    %[data]             \n"  /* Shift data to upper byte */
        "lsl.l   #8, %[data]         \n"
        
        "move.l  %[cbit], %%d1       \n"  /* Prepare mask for flipping CLK */
        "or.l    %[dbit], %%d1       \n"  /*   and DATA at once */

        "lsl.l   #1,%[data]          \n"  /* Shift out MSB */
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"  /* 1: Flip both CLK and DATA */
        ".word   0x51fa              \n"  /* (trapf.w - shadow next insn) */
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"  /*   else flip CLK only */
        "eor.l   %[cbit], (%[gpo0])  \n"  /* Flip CLK again */

        "lsl.l   #1,%[data]          \n"  /* ..unrolled.. */
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %%d1, (%[gpo0])     \n"
        ".word   0x51fa              \n"
    "1:                              \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        "eor.l   %[cbit], (%[gpo0])  \n"
        : /* outputs */
        [data]"+d"(data)
        : /* inputs */
        [gpo0]"a"(&GPIO_OUT),
        [cbit]"d"(0x20000000),
        [dbit]"d"(0x04000000)
        : /* clobbers */
        "d0", "d1"
    );
}

/* Fast low-level byte writer. Don't use with high CPU clock.
 * Requires CLK high on entry */
static inline void _write_fast(unsigned data)
{
    asm volatile (
        "move.w  %%sr,%%d3           \n"  /* Get current interrupt level */
        "move.w  #0x2700,%%sr        \n"  /* Disable interrupts */

        "move.l  (%[gpo0]), %%d0     \n"  /* Get current state of data port */
        "move.l  %%d0, %%d1          \n"
        "and.l   %[dbit], %%d1       \n"  /* Check current state of data line */
        "beq.s   1f                  \n"  /*   and set it as previous-state bit */
        "bset    #8, %[data]         \n"
    "1:                              \n"
        "move.l  %[data], %%d1       \n"  /* Compute the 'bit derivative', i.e. a value */
        "lsr.l   #1, %%d1            \n"  /*   with 1's where the data changes from the */
        "eor.l   %%d1, %[data]       \n"  /*   previous state, and 0's where it doesn't */
        "swap    %[data]             \n"  /* Shift data to upper byte */
        "lsl.l   #8, %[data]         \n"
        
        "move.l  %%d0, %%d1          \n"  /* precalculate opposite state of clock line */
        "eor.l   %[cbit], %%d1       \n"
        
        "lsl.l   #1,%[data]          \n"  /* Shift out MSB */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"  /* 1: Flip data bit */
        "eor.l   %[dbit], %%d1       \n"  /*   for both clock states */
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"  /* Output new state and set CLK */
        "move.l  %%d0, (%[gpo0])     \n"  /* reset CLK */

        "lsl.l   #1,%[data]          \n"  /* ..unrolled.. */
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "lsl.l   #1,%[data]          \n"
        "bcc.s   1f                  \n"
        "eor.l   %[dbit], %%d0       \n"
        "eor.l   %[dbit], %%d1       \n"
    "1:                              \n"
        "move.l  %%d1, (%[gpo0])     \n"
        "move.l  %%d0, (%[gpo0])     \n"

        "move.w  %%d3, %%sr          \n" /* Restore interrupt level */
        : /* outputs */
        [data]"+d"(data)
        : /* inputs */
        [gpo0]"a"(&GPIO_OUT),
        [cbit]"d"(0x20000000),
        [dbit]"d"(0x04000000)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
    );
}

void lcd_write_command(int cmd)
{
    RS_LO;
    CS_LO;
    _write_byte(cmd);
    CS_HI;
}

void lcd_write_command_e(int cmd, int data)
{
    RS_LO;
    CS_LO;
    _write_byte(cmd);
    _write_byte(data);
    CS_HI;
}

void lcd_write_data(const fb_data *p_words, int count)
{
    const unsigned char *p_bytes = (const unsigned char *)p_words;
    const unsigned char *p_end = (const unsigned char *)(p_words + count);

    RS_HI;
    CS_LO;
    if (cpu_frequency < 50000000)
    {
        while (p_bytes < p_end)
            _write_fast(*p_bytes++);
    }
    else
    {
        while (p_bytes < p_end)
            _write_byte(*p_bytes++);
    }
    CS_HI;
}

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcdset_contrast(int val)
{
    if (val < 0)
        val = 0;
    else if (val > 63)
        val = 63;

    cached_contrast = val;
    if (initialized)
        lcd_write_command_e(LCD_SET_VOLUME, val);
}

bool remote_detect(void)
{
    return (GPIO_READ & 0x40000000) == 0;
}

void lcd_init_device(void)
{
    or_l(0x24000000, &GPIO_OUT);
    or_l(0x24000000, &GPIO_ENABLE);
    or_l(0x24000000, &GPIO_FUNCTION);
    
    or_l(0x00011000, &GPIO1_OUT);
    or_l(0x00011000, &GPIO1_ENABLE);
    or_l(0x00011000, &GPIO1_FUNCTION);

    and_l(~0x40000000, &GPIO_OUT);
    and_l(~0x40000000, &GPIO_ENABLE);
    or_l(0x40000000, &GPIO_FUNCTION);

    lcd_clear_display();
    tick_add_task(lcd_tick);
}

void lcd_on(void)
{
    CS_HI;
    CLK_HI;
    sleep(10);
    
    lcd_write_command(LCD_SET_DUTY_RATIO);
    lcd_write_command(0x70);  /* 1/128 */
    
    lcd_write_command(LCD_OSC_ON);
    
    lcd_write_command(LCD_SELECT_DCDC | 2); /* DC/DC 5xboost */
    
    lcd_write_command(LCD_SELECT_RES | 7); /* Regulator resistor: 7.2 */
    
    lcd_write_command(LCD_SET_BIAS | 6); /* 1/11 */
    
    lcd_write_command(LCD_CONTROL_POWER | 7); /* All circuits ON */
    
    sleep(30);
    
    lcd_write_command_e(LCD_SET_GRAY | 0, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 1, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 2, 0x0c);
    lcd_write_command_e(LCD_SET_GRAY | 3, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 4, 0xcc);
    lcd_write_command_e(LCD_SET_GRAY | 5, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 6, 0xcc);
    lcd_write_command_e(LCD_SET_GRAY | 7, 0x0c);
    
    lcd_write_command(LCD_SET_PWM_FRC | 6); /* 4FRC + 12PWM */
    
    lcd_write_command(LCD_DISPLAY_ON | 1); /* display on */

    initialized = true;

    lcd_set_flip(cached_flip);
    lcd_set_contrast(cached_contrast);
    lcd_set_invert_display(cached_invert);
}

void lcd_off(void)
{
    initialized = false;
    CS_HI;
    RS_HI;
}

void lcd_poweroff(void)
{
    /* Set power save -> Power OFF (VDD - VSS) .. that's it */
    if (initialized && remote_detect())
        lcd_write_command(LCD_SET_POWER_SAVE | 1);
}

/* Monitor remote hotswap */
static void lcd_tick(void)
{
    static bool last_status = false;
    static int countdown = 0;
    bool current_status;

    current_status = remote_detect();

    /* Only report when the status has changed */
    if (current_status != last_status)
    {
        last_status = current_status;
        countdown = current_status ? 20*HZ : 1;
    }
    else
    {
        /* Count down until it gets negative */
        if (countdown >= 0)
            countdown--;

        if (current_status)
        {
            if (!(countdown % 48))
            {
                queue_broadcast(SYS_REMOTE_PLUGGED, 0);
            }
        }
        else
        {
            if (countdown == 0)
            {
                queue_broadcast(SYS_REMOTE_UNPLUGGED, 0);
            }
        }
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y;
    if(initialized) {
        for(y = 0;y < LCD_FBHEIGHT;y++) {
            /* Copy display bitmap to hardware.
               The COM48-COM63 lines are not connected so we have to skip
               them. Further, the column address doesn't wrap, so we
               have to update one page at a time. */
            lcd_write_command(LCD_SET_PAGE | (y>5?y+2:y));
            lcd_write_command_e(LCD_SET_COLUMN | 0, 0);
            lcd_write_data(lcd_framebuffer[y], LCD_WIDTH);
        }
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    if(initialized) {
        int ymax;

        /* The Y coordinates have to work on even 8 pixel rows */
        ymax = (y + height-1) >> 3;
        y >>= 3;

        if(x + width > LCD_WIDTH)
            width = LCD_WIDTH - x;
        if (width <= 0)
            return; /* nothing left to do, 0 is harmful to lcd_write_data() */
        if(ymax >= LCD_FBHEIGHT)
            ymax = LCD_FBHEIGHT-1;

        /* Copy specified rectangle bitmap to hardware
           COM48-COM63 are not connected, so we need to skip those */
        for (; y <= ymax; y++) 
        {
            lcd_write_command(LCD_SET_PAGE | ((y > 5?y + 2:y) & 0xf));
            lcd_write_command_e(LCD_SET_COLUMN | ((x >> 4) & 0xf), x & 0xf);

            lcd_write_data(&lcd_framebuffer[y][x], width);
        } 
    }
}

void lcd_set_invert_display(bool yesno)
{
    cached_invert = yesno;
    if(initialized)
        lcd_write_command(LCD_REVERSE | yesno);
}

void lcd_set_flip(bool yesno)
{
    cached_flip = yesno;
    if(initialized) {
        if(yesno) {
            lcd_write_command(LCD_SELECT_ADC | 0);
            lcd_write_command(LCD_SELECT_SHL | 0);
            lcd_write_command_e(LCD_SET_COM0, 16);
        } else {
            lcd_write_command(LCD_SELECT_ADC | 1);
            lcd_write_command(LCD_SELECT_SHL | 8);
            lcd_write_command_e(LCD_SET_COM0, 0);
        }
    }
}
