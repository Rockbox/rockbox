#include "config.h"
#include <string.h>
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"
#include "mmu-meg-fx.h"
#include <stdlib.h>
#include "memory.h"
#include "lcd-target.h"

/*
** We prepare foreground and background fills ahead of time - DMA fills in 16 byte groups
*/
unsigned long fg_pattern_blit[4];
unsigned long bg_pattern_blit[4];

volatile bool use_dma_blit = false;
static volatile bool lcd_on = true;
volatile bool lcd_poweroff = false;
/*
** These are imported from lcd-16bit.c
*/
extern unsigned fg_pattern;
extern unsigned bg_pattern;

bool lcd_enabled()
{
    return lcd_on;
}

/* LCD init */
void lcd_init_device(void)
{
    memset16(fg_pattern_blit, fg_pattern, sizeof(fg_pattern_blit)/2);
    memset16(bg_pattern_blit, bg_pattern, sizeof(bg_pattern_blit)/2);
    clean_dcache_range((void *)fg_pattern_blit, sizeof(fg_pattern_blit));
    clean_dcache_range((void *)bg_pattern_blit, sizeof(bg_pattern_blit));

    LCDSADDR1 = 0x18F00000; /* These values are pulled from an F40  */
    LCDSADDR2 = 0x00112C00; /* They should move FRAME to the correct location */
    LCDSADDR3 = 0x000000F0; /* TODO: Move FRAME to where we want it */

    LCDCON5 |= 1 << 11;     /* Switch from 555I mode to 565 mode */

#if !defined(BOOTLOADER)
    use_dma_blit = true;
    lcd_poweroff = true;
#endif
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)width;
    (void)y;
    (void)height;

    if(!lcd_on)
    {
        sleep(200);
        return;
    }
    if (use_dma_blit)
    {
        /* Wait for this controller to stop pending transfer */
        while((DSTAT1 & 0x000fffff))
            CLKCON |= (1 << 2); /* set IDLE bit */

        /* Flush DCache */
        invalidate_dcache_range((void *)(((int) &lcd_framebuffer)+(y * sizeof(fb_data) * LCD_WIDTH)), (height * sizeof(fb_data) * LCD_WIDTH));

        /* set DMA dest */
        DIDST1 = ((int) FRAME) + (y * sizeof(fb_data) * LCD_WIDTH);

        /* FRAME on AHB buf, increment */
        DIDSTC1 = 0;
        /* Handshake on AHB, Burst transfer, Whole service, Don't reload, transfer 32-bits */
        DCON1 = ((1<<30) | (1<<28) |  (1<<27) | (1<<22) | (2<<20)) | ((height * sizeof(fb_data) * LCD_WIDTH) >> 4);

        /* set DMA source  */
        DISRC1 = ((int) &lcd_framebuffer) + (y * sizeof(fb_data) * LCD_WIDTH) + 0x30000000;
        /* memory is on AHB bus, increment addresses */
        DISRCC1 = 0x00;

        /* Activate the channel */
        DMASKTRIG1 = 0x2;

        /* Start DMA */
        DMASKTRIG1 |= 0x1;

        /* Wait for transfer to complete */
        while((DSTAT1 & 0x000fffff))
            CLKCON |= (1 << 2); /* set IDLE bit */
    }
    else
        memcpy(((char*)FRAME) + (y * sizeof(fb_data) * LCD_WIDTH), ((char *)&lcd_framebuffer) + (y * sizeof(fb_data) * LCD_WIDTH), ((height * sizeof(fb_data) * LCD_WIDTH)));
}


void lcd_enable(bool state)
{
    if(!lcd_poweroff)
        return;
    if(state) {
        if(!lcd_on) {
            lcd_on = true;
            memcpy(FRAME, lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT*2);
            LCDCON1 |= 1;
        }
    }
    else {
        if(lcd_on) {
            lcd_on = false;
            LCDCON1 &= ~1;
        }
    }
}

void lcd_set_foreground(unsigned color)
{
    fg_pattern = color;

    memset16(fg_pattern_blit, fg_pattern, sizeof(fg_pattern_blit)/2);
    invalidate_dcache_range((void *)fg_pattern_blit, sizeof(fg_pattern_blit));
}

void lcd_set_background(unsigned color)
{
    bg_pattern = color;
    memset16(bg_pattern_blit, bg_pattern, sizeof(bg_pattern_blit)/2);
    invalidate_dcache_range((void *)bg_pattern_blit, sizeof(bg_pattern_blit));
}

void lcd_device_prepare_backdrop(fb_data* backdrop)
{
    invalidate_dcache_range((void *)backdrop, (LCD_HEIGHT * sizeof(fb_data) * LCD_WIDTH));
}

void lcd_clear_display_dma(void)
{
    void *src;
    bool inc = false;

    if(!lcd_on) {
        sleep(200);
    }
    if (lcd_get_drawmode() & DRMODE_INVERSEVID)
        src = fg_pattern_blit;
    else
    {
        fb_data* lcd_backdrop = lcd_get_backdrop();

        if (!lcd_backdrop)
            src = bg_pattern_blit;
        else
        {
            src = lcd_backdrop;
            inc = true;
        }
    }
    /* Wait for any pending transfer to complete */
    while((DSTAT3 & 0x000fffff))
        CLKCON |= (1 << 2); /* set IDLE bit */
    DMASKTRIG3 |= 0x4; /* Stop controller */
    DIDST3 = ((int) lcd_framebuffer) + 0x30000000; /* set DMA dest, physical address */
    DIDSTC3 = 0; /* Dest on AHB, increment */

    DISRC3 = ((int) src) + 0x30000000; /* Set source, in physical space */
    DISRCC3 = inc ? 0x00 : 0x01;  /* memory is on AHB bus, increment addresses based on backdrop */

    /* Handshake on AHB, Burst mode, whole service mode, no reload, move 32-bits */
    DCON3 = ((1<<30) | (1<<28) | (1<<27) | (1<<22) | (2<<20)) | (sizeof(lcd_framebuffer) >> 4);

    /* Dump DCache for dest, we are about to overwrite it with DMA */
    invalidate_dcache_range((void *)lcd_framebuffer, sizeof(lcd_framebuffer));
    /* Activate the channel */
    DMASKTRIG3 = 2;
    /* Start DMA */
    DMASKTRIG3 |= 1;

    /* Wait for transfer to complete */
    while((DSTAT3 & 0x000fffff))
        CLKCON |= (1 << 2); /* set IDLE bit */
}

void lcd_clear_display(void)
{
    lcd_stop_scroll();

    if(use_dma_blit)
    {
        lcd_clear_display_dma();
        return;
    }

    fb_data *dst = &lcd_framebuffer[0][0];

    if (lcd_get_drawmode() & DRMODE_INVERSEVID)
    {
        memset16(dst, fg_pattern, LCD_WIDTH*LCD_HEIGHT);
    }
    else
    {
        fb_data* lcd_backdrop = lcd_get_backdrop();
        if (!lcd_backdrop)
            memset16(dst, bg_pattern, LCD_WIDTH*LCD_HEIGHT);
        else
            memcpy(dst, lcd_backdrop, sizeof(lcd_framebuffer));
    }
}


/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

#define CSUB_X 2
#define CSUB_Y 2

#define RYFAC (31*257)
#define GYFAC (63*257)
#define BYFAC (31*257)
#define RVFAC 11170     /* 31 * 257 *  1.402    */
#define GVFAC (-11563)  /* 63 * 257 * -0.714136 */
#define GUFAC (-5572)   /* 63 * 257 * -0.344136 */
#define BUFAC 14118     /* 31 * 257 *  1.772    */

#define ROUNDOFFS (127*257)

/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    width = (width + 1) & ~1;
    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + (LCD_WIDTH - y) - 1;
    fb_data *dst_last = dst - (height - 1);

    for (;;)
    {
        fb_data *dst_row = dst;
        int count = width;
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;
        int y, u, v;
        int red, green, blue;
        unsigned rbits, gbits, bbits;

        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *usrc = src[1] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                           + (src_x/CSUB_X);
        const unsigned char *vsrc = src[2] + (stride/CSUB_X) * (src_y/CSUB_Y)
                                           + (src_x/CSUB_X);
        int xphase = src_x % CSUB_X;
        int rc, gc, bc;

        u = *usrc++ - 128;
        v = *vsrc++ - 128;
        rc = RVFAC * v + ROUNDOFFS;
        gc = GVFAC * v + GUFAC * u + ROUNDOFFS;
        bc = BUFAC * u + ROUNDOFFS;

        do
        {
            y = *ysrc++;
            red   = RYFAC * y + rc;
            green = GYFAC * y + gc;
            blue  = BYFAC * y + bc;

            if ((unsigned)red > (RYFAC*255+ROUNDOFFS))
            {
                if (red < 0)
                    red = 0;
                else
                    red = (RYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)green > (GYFAC*255+ROUNDOFFS))
            {
                if (green < 0)
                    green = 0;
                else
                    green = (GYFAC*255+ROUNDOFFS);
            }
            if ((unsigned)blue > (BYFAC*255+ROUNDOFFS))
            {
                if (blue < 0)
                    blue = 0;
                else
                    blue = (BYFAC*255+ROUNDOFFS);
            }
            rbits = ((unsigned)red) >> 16 ;
            gbits = ((unsigned)green) >> 16 ;
            bbits = ((unsigned)blue) >> 16 ;

            *dst_row = (rbits << 11) | (gbits << 5) | bbits;

            /* next pixel - since rotated, add WIDTH */
            dst_row += LCD_WIDTH;

            if (++xphase >= CSUB_X)
            {
                u = *usrc++ - 128;
                v = *vsrc++ - 128;
                rc = RVFAC * v + ROUNDOFFS;
                gc = GVFAC * v + GUFAC * u + ROUNDOFFS;
                bc = BUFAC * u + ROUNDOFFS;
                xphase = 0;
            }
        }
        while (--count);

        if (dst == dst_last) break;

        dst--;
        src_y++;
    }
}


void lcd_set_contrast(int val) {
  (void) val;
  // TODO:
}

void lcd_set_invert_display(bool yesno) {
  (void) yesno;
  // TODO:
}

void lcd_blit(const fb_data* data, int bx, int y, int bwidth,
              int height, int stride)
{
  (void) data;
  (void) bx;
  (void) y;
  (void) bwidth;
  (void) height;
  (void) stride;
  //TODO:
}

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

