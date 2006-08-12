#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"

void lcd_init_device(void);
void lcd_update_rec(int, int, int, int);
void lcd_update(void);

/* LCD init */
void lcd_init_device(void)
{
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    memcpy(FRAME, &lcd_framebuffer, sizeof(lcd_framebuffer));
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
