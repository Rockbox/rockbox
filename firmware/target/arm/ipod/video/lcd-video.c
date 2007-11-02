/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * LCD driver for iPod Video
 * 
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
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

/* The BCM bus width is 16 bits. But since the low address bits aren't decoded
 * by the chip (the 3 BCM address bits are mapped to address bits 16..18 of the
 * PP5022), writing 32 bits (and even more, using 'stmia') at once works. */
#define BCM_DATA      (*(volatile unsigned short*)(0x30000000))
#define BCM_DATA32    (*(volatile unsigned long *)(0x30000000))
#define BCM_WR_ADDR   (*(volatile unsigned short*)(0x30010000))
#define BCM_WR_ADDR32 (*(volatile unsigned long *)(0x30010000))
#define BCM_RD_ADDR   (*(volatile unsigned short*)(0x30020000))
#define BCM_RD_ADDR32 (*(volatile unsigned long *)(0x30020000))
#define BCM_CONTROL   (*(volatile unsigned short*)(0x30030000))

#define BCM_ALT_DATA      (*(volatile unsigned short*)(0x30040000))
#define BCM_ALT_DATA32    (*(volatile unsigned long *)(0x30040000))
#define BCM_ALT_WR_ADDR   (*(volatile unsigned short*)(0x30050000))
#define BCM_ALT_WR_ADDR32 (*(volatile unsigned long *)(0x30050000))
#define BCM_ALT_RD_ADDR   (*(volatile unsigned short*)(0x30060000))
#define BCM_ALT_RD_ADDR32 (*(volatile unsigned long *)(0x30060000))
#define BCM_ALT_CONTROL   (*(volatile unsigned short*)(0x30070000))

#define BCM_FB_BASE 0xE0020 /* Address of internal BCM framebuffer */

/* Time until the BCM is considered stalled and will be re-kicked.
 * Must be guaranteed to be >~ 20ms. */
#define BCM_UPDATE_TIMEOUT (HZ/20)  

enum lcd_status {
    LCD_IDLE,
    LCD_INITIAL,
    LCD_NEED_UPDATE,
    LCD_UPDATING
};

struct {
    long update_timeout;         
    enum lcd_status state;
    bool blocked;
    struct corelock cl;   /* inter-core sync */
} lcd_state IBSS_ATTR;


static inline void bcm_write_addr(unsigned address)
{
    BCM_WR_ADDR32 = address;       /* write destination address */

    while (!(BCM_CONTROL & 0x2));  /* wait for it to be write ready */
}

static inline void bcm_write32(unsigned address, unsigned value)
{

    bcm_write_addr(address);       /* set destination address */

    BCM_DATA32 = value;            /* write value */
}

static inline unsigned bcm_read32(unsigned address)
{
    while (!(BCM_RD_ADDR & 1));

    BCM_RD_ADDR32 = address;       /* write source address */

    while (!(BCM_CONTROL & 0x10)); /* wait for it to be read ready */

    return BCM_DATA32;             /* read value */
}

static void bcm_setup_rect(unsigned x, unsigned y,
                           unsigned width, unsigned height)
{
    bcm_write_addr(0xE0004);
    BCM_DATA32 = x;
    BCM_DATA32 = y;
    BCM_DATA32 = x + width - 1;
    BCM_DATA32 = y + height - 1;
}

static void lcd_tick(void)
{
    /* No set_irq_level - already in interrupt context */
    corelock_lock(&lcd_state.cl);

    if (!lcd_state.blocked && lcd_state.state >= LCD_NEED_UPDATE)
    {
        unsigned data = bcm_read32(0x1F8);
        bool bcm_is_busy = (data == 0xFFFA0005 || data == 0xFFFF);
        
        if (((lcd_state.state == LCD_NEED_UPDATE) && !bcm_is_busy) 
            /* Update requested and BCM is no longer busy. */
         || (TIME_AFTER(current_tick, lcd_state.update_timeout) && bcm_is_busy))
            /* BCM still busy after timeout, i.e. stalled. */
        {
            bcm_write32(0x1F8, 0xFFFA0005);  /* Kick off update */
            BCM_CONTROL = 0x31;
            lcd_state.update_timeout = current_tick + BCM_UPDATE_TIMEOUT;
            lcd_state.state = LCD_UPDATING;
        }
        else if ((lcd_state.state == LCD_UPDATING) && !bcm_is_busy)
        {
            /* Update finished properly and no new update pending. */
            lcd_state.state = LCD_IDLE;
        }
    }
    corelock_unlock(&lcd_state.cl);
}

static inline void lcd_block_tick(void)
{
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    corelock_lock(&lcd_state.cl);
    lcd_state.blocked = true;
    corelock_unlock(&lcd_state.cl);

    set_irq_level(oldlevel);
}

static void lcd_unblock_and_update(void)
{
    unsigned data;
    bool bcm_is_busy;
    int oldlevel = set_irq_level(HIGHEST_IRQ_LEVEL);

    corelock_lock(&lcd_state.cl);

    data = bcm_read32(0x1F8);
    bcm_is_busy = (data == 0xFFFA0005 || data == 0xFFFF);
    
    if (!bcm_is_busy || (lcd_state.state == LCD_INITIAL) ||
        TIME_AFTER(current_tick, lcd_state.update_timeout))
    {
        bcm_write32(0x1F8, 0xFFFA0005);  /* Kick off update */
        BCM_CONTROL = 0x31;
        lcd_state.update_timeout = current_tick + BCM_UPDATE_TIMEOUT;
        lcd_state.state = LCD_UPDATING;
    }
    else
    {
         lcd_state.state = LCD_NEED_UPDATE; /* Post update request */
    }
    lcd_state.blocked = false;
    corelock_unlock(&lcd_state.cl);

    set_irq_level(oldlevel);
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
    lcd_state.blocked = false;
    lcd_state.state = LCD_INITIAL;
    corelock_init(&lcd_state.cl);
    bcm_setup_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    tick_add_task(&lcd_tick);
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
    const fb_data *addr;
    unsigned bcmaddr;

    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    /* Ensure x and width are both even. The BCM doesn't like small unaligned
     * writes and would just ignore them. */
    width = (width + (x & 1) + 1) & ~1;
    x &= ~1;
    
    /* Prevent the tick from triggering BCM updates while we're writing. */
    lcd_block_tick();

    addr = &lcd_framebuffer[y][x];
    bcmaddr = BCM_FB_BASE + (LCD_WIDTH*2) * y + (x << 1);

    if (width == LCD_WIDTH)
    {
        bcm_write_addr(bcmaddr);
        lcd_write_data(addr, width * height);
    }
    else
    {
        do
        {
            bcm_write_addr(bcmaddr);
            bcmaddr += (LCD_WIDTH*2);
            lcd_write_data(addr, width);
            addr += LCD_WIDTH;
        }
        while (--height > 0);
    }
    lcd_unblock_and_update();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Line write helper functions for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_upper(unsigned char const * const src[3],
                                   unsigned char *chroma_buf, int width);
extern void lcd_write_yuv420_lower(unsigned const char *y_src,
                                   unsigned char *chroma_buf, int width);

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned bcmaddr;
    off_t z;
    unsigned char const * yuv_src[3];
    unsigned char chroma_buf[3*width]; /* dynamic */

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    /* Prevent the tick from triggering BCM updates while we're writing. */
    lcd_block_tick();

    bcmaddr = BCM_FB_BASE + (LCD_WIDTH*2) * y + (x << 1);
    height >>= 1;

    if (width == LCD_WIDTH)
    {
        bcm_write_addr(bcmaddr);
        do
        {
            lcd_write_yuv420_upper(yuv_src, chroma_buf, width);
            yuv_src[0] += stride;
            lcd_write_yuv420_lower(yuv_src[0], chroma_buf, width);
            yuv_src[0] += stride;
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            bcm_write_addr(bcmaddr);
            bcmaddr += (LCD_WIDTH*2);
            lcd_write_yuv420_upper(yuv_src, chroma_buf, width);
            yuv_src[0] += stride;
            bcm_write_addr(bcmaddr);
            bcmaddr += (LCD_WIDTH*2);
            lcd_write_yuv420_lower(yuv_src[0], chroma_buf, width);
            yuv_src[0] += stride;
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
        }
        while (--height > 0);
    }
    lcd_unblock_and_update();
}
