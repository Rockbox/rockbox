#include "config.h"
#include <string.h>
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"

void lcd_init_device(void);
void lcd_update_rec(int, int, int, int);
void lcd_update(void);

bool usedmablit = false;

/* LCD init */
void lcd_init_device(void)
{
    /* Switch from 555I mode to 565 mode */
    LCDCON5 |= 1 << 11;
   
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)width;
    
    if (usedmablit) 
    {
        /* Spin waiting for DMA to become available */
        //while (DSTAT0 & (1<<20)) ;
        if (DSTAT0 & (1<<20)) return;
        
        /* set DMA dest */
        DIDST0 = (int) FRAME + y * sizeof(fb_data) * LCD_WIDTH;

        /* FRAME on AHB buf, increment */
        DIDSTC0 = 0;
        DCON0 = (((1<<30) | (1<<28) | (1<<27) | (1<<22) | (2<<20)) | ((height * sizeof(fb_data) * LCD_WIDTH) >> 4));

        /* set DMA source and options */
        DISRC0 = (int) &lcd_framebuffer + (y * sizeof(fb_data) * LCD_WIDTH) + 0x30000000;
        DISRCC0 = 0x00;  /* memory is on AHB bus, increment addresses */

        /* Activate the channel */
        DMASKTRIG0 = 0x2;
        /* Start DMA */
        DMASKTRIG0 |= 0x1;
    }
    else
    {
        memcpy((void*)FRAME, &lcd_framebuffer, sizeof(lcd_framebuffer));
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













#if 0
/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    fb_data *dst, *dst_end;

    width = (width + 1) & ~1;

    dst = (fb_data*)FRAME + LCD_WIDTH * y + x;
    dst_end = dst + LCD_WIDTH * height;

    do
    {
        fb_data *dst_row = dst;
        fb_data *row_end = dst_row + width;
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
            *dst_row++ = (rbits << 11) | (gbits << 5) | bbits;

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
        while (dst_row < row_end);

        src_y++;
        dst += LCD_WIDTH;
    }
    while (dst < dst_end);
}
#endif



