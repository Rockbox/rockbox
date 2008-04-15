#include "config.h"
#include "cpu.h"
#include "string.h"
#include "lcd.h"
#include "kernel.h"
#include "lcd-target.h"

#define LCDADDR(x, y) (&lcd_framebuffer[(y)][(x)])

static volatile bool lcd_on = true;
volatile bool lcd_poweroff = false;
static unsigned lcd_yuv_options = 0;
/*
** This is imported from lcd-16bit.c
*/
extern struct viewport* current_vp;

/* Copies a rectangle from one framebuffer to another. Can be used in
   single transfer mode with width = num pixels, and height = 1 which
   allows a full-width rectangle to be copied more efficiently. */
extern void lcd_copy_buffer_rect(fb_data *dst, const fb_data *src,
                                 int width, int height);

#if 0
bool lcd_enabled()
{
    return lcd_on;
}
#endif

void printscreen(unsigned int colour)
{
    int i;
    char * base = (char *)FRAME;
    for(i = 0; i < (320*240*2); i++)
    {
        writel(colour, base);
        base++;
    }
}

/* LCD init */
void lcd_init_device(void)
{
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data *dst, *src;

    if (!lcd_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    /* TODO: It may be faster to swap the addresses of lcd_driver_framebuffer
     * and lcd_framebuffer */
    dst = (fb_data *)FRAME + LCD_WIDTH*y + x;
    src = &lcd_framebuffer[y][x];

    /* Copy part of the Rockbox framebuffer to the second framebuffer */
    if (width < LCD_WIDTH)
    {
        /* Not full width - do line-by-line */
        lcd_copy_buffer_rect(dst, src, width, height);
    }
    else
    {
        /* Full width - copy as one line */
        lcd_copy_buffer_rect(dst, src, LCD_WIDTH*height, 1);
    }
}

void lcd_enable(bool state)
{
    (void)state;
}

bool lcd_enabled(void)
{
    return true;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!lcd_on)
        return;

    lcd_copy_buffer_rect((fb_data *)FRAME, &lcd_framebuffer[0][0],
                         LCD_WIDTH*LCD_HEIGHT, 1);
}

void lcd_bitmap_transparent_part(const fb_data *src, int src_x, int src_y,
                                 int stride, int x, int y, int width,
                                 int height)
{
    int w, px;
    fb_data *dst;

    if (x + width > current_vp->width)
        width = current_vp->width - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    if (y + height > current_vp->height)
        height = current_vp->height - y; /* Clip bottom */
    if (y < 0)
        height += y, y = 0; /* Clip top */
    if (height <= 0)
        return; /* nothing left to do */

    src += stride * src_y + src_x; /* move starting point */
    dst = &lcd_framebuffer[current_vp->y+y][current_vp->x+x];

    asm volatile (
    ".rowstart:                             \r\n"
        "mov    %[w], %[width]              \r\n" /* Load width for inner loop */
    ".nextpixel:                            \r\n"
        "ldrh   %[px], [%[s]], #2           \r\n" /* Load src pixel */
        "add    %[d], %[d], #2              \r\n" /* Uncoditionally increment dst */
        "cmp    %[px], %[fgcolor]           \r\n" /* Compare to foreground color */
        "streqh %[fgpat], [%[d], #-2]       \r\n" /* Store foregroud if match */
        "cmpne  %[px], %[transcolor]        \r\n" /* Compare to transparent color */
        "strneh %[px], [%[d], #-2]          \r\n" /* Store dst if not transparent */
        "subs   %[w], %[w], #1              \r\n" /* Width counter has run down? */
        "bgt    .nextpixel                  \r\n" /* More in this row? */
        "add    %[s], %[s], %[sstp], lsl #1 \r\n" /* Skip over to start of next line */
        "add    %[d], %[d], %[dstp], lsl #1 \r\n"
        "subs   %[h], %[h], #1              \r\n" /* Height counter has run down? */
        "bgt    .rowstart                   \r\n" /* More rows? */
        : [w]"=&r"(w), [h]"+&r"(height), [px]"=&r"(px),
          [s]"+&r"(src), [d]"+&r"(dst)
        : [width]"r"(width),
          [sstp]"r"(stride - width),
          [dstp]"r"(LCD_WIDTH - width),
          [transcolor]"r"(TRANSPARENT_COLOR),
          [fgcolor]"r"(REPLACEWITHFG_COLOR),
          [fgpat]"r"(current_vp->fg_pattern)
    );
}

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(fb_data *dst,
                                   unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(fb_data *dst,
                                           unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
/* For the Gigabeat - show it rotated */
/* So the LCD_WIDTH is now the height */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* Caches for chroma data so it only need be recaculated every other
       line */
    unsigned char const * yuv_src[3];
    off_t z;

    if (!lcd_on)
        return;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    y = LCD_WIDTH - 1 - y;
    fb_data *dst = (fb_data*)FRAME + x * LCD_WIDTH + y;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_write_yuv420_lines_odither(dst, yuv_src, width, stride, y, x);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
            y -= 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_write_yuv420_lines(dst, yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            dst -= 2;
        }
        while (--height > 0);
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

void lcd_set_flip(bool yesno) {
  (void) yesno;
  // TODO:
}

