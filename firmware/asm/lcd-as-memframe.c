
#include <string.h>
#include "lcd.h"
void lcd_copy_buffer_rect(fb_data *dst, fb_data *src, int width, int height)
{
    do {
        memcpy(dst, src, width * sizeof(fb_data));
        src += LCD_WIDTH;
        dst += LCD_WIDTH;
    } while (--height);
}

#define YFAC    (74)
#define RVFAC   (101)
#define GUFAC   (-24)
#define GVFAC   (-51)
#define BUFAC   (128)

static inline int clamp(int val, int min, int max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
    return val;
}

extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride)
{
    /* Draw a partial YUV colour bitmap - similiar behavior to lcd_blit_yuv
       in the core */
    const unsigned char *ysrc, *usrc, *vsrc;
    int height = 2, linecounter;
    fb_data *row_end;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    linecounter = height >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
    row_end = dst + width;
#else
    row_end = dst + LCD_WIDTH * width;
#endif

    ysrc = src[0];
    usrc = src[1];
    vsrc = src[2];

    /* stride => amount to jump from end of last row to start of next */
    stride -= width;

    /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */

    do
    {
        int y, cb, cr, rv, guv, bu, r, g, b;

        y  = YFAC*(*ysrc++ - 16);
        cb = *usrc++ - 128;
        cr = *vsrc++ - 128;

        rv  =            RVFAC*cr;
        guv = GUFAC*cb + GVFAC*cr;
        bu  = BUFAC*cb;

        r = y + rv;
        g = y + guv;
        b = y + bu;

        if ((unsigned)(r | g | b) > 64*256-1)
        {
            r = clamp(r, 0, 64*256-1);
            g = clamp(g, 0, 64*256-1);
            b = clamp(b, 0, 64*256-1);
        }

        *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
        dst++;
#else
        dst += LCD_WIDTH;
#endif

        y = YFAC*(*ysrc++ - 16);
        r = y + rv;
        g = y + guv;
        b = y + bu;

        if ((unsigned)(r | g | b) > 64*256-1)
        {
            r = clamp(r, 0, 64*256-1);
            g = clamp(g, 0, 64*256-1);
            b = clamp(b, 0, 64*256-1);
        }

        *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
        dst++;
#else
        dst += LCD_WIDTH;
#endif
    }
    while (dst < row_end);

    ysrc    += stride;
    usrc    -= width >> 1;
    vsrc    -= width >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
    row_end += LCD_WIDTH;
    dst     += LCD_WIDTH - width;
#else
    row_end -= 1;
    dst     -= LCD_WIDTH*width + 1;
#endif

    do
    {
        int y, cb, cr, rv, guv, bu, r, g, b;

        y  = YFAC*(*ysrc++ - 16);
        cb = *usrc++ - 128;
        cr = *vsrc++ - 128;

        rv  =            RVFAC*cr;
        guv = GUFAC*cb + GVFAC*cr;
        bu  = BUFAC*cb;

        r = y + rv;
        g = y + guv;
        b = y + bu;

        if ((unsigned)(r | g | b) > 64*256-1)
        {
            r = clamp(r, 0, 64*256-1);
            g = clamp(g, 0, 64*256-1);
            b = clamp(b, 0, 64*256-1);
        }

        *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
        dst++;
#else
        dst += LCD_WIDTH;
#endif

        y = YFAC*(*ysrc++ - 16);
        r = y + rv;
        g = y + guv;
        b = y + bu;

        if ((unsigned)(r | g | b) > 64*256-1)
        {
            r = clamp(r, 0, 64*256-1);
            g = clamp(g, 0, 64*256-1);
            b = clamp(b, 0, 64*256-1);
        }

        *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
        dst++;
#else
        dst += LCD_WIDTH;
#endif
    }
    while (dst < row_end);
}

void lcd_write_yuv420_lines_odither(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width, int stride,
                                   int x_screen, int y_screen)
__attribute__((alias("lcd_write_yuv420_lines")));
