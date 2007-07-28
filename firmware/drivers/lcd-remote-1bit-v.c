/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charité III
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
#include "lcd-remote.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "rbunicode.h"
#include "bidi.h"
#include "scroll_engine.h"

/*** globals ***/

fb_remote_data lcd_remote_framebuffer[LCD_REMOTE_FBHEIGHT][LCD_REMOTE_FBWIDTH]
                                      IBSS_ATTR;

static int drawmode = DRMODE_SOLID;
static int xmargin = 0;
static int ymargin = 0;
static int curfont = FONT_SYSFIXED;

/*** parameter handling ***/

void lcd_remote_set_drawmode(int mode)
{
    drawmode = mode & (DRMODE_SOLID|DRMODE_INVERSEVID);
}

int lcd_remote_get_drawmode(void)
{
    return drawmode;
}

void lcd_remote_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_remote_getxmargin(void)
{
    return xmargin;
}

int lcd_remote_getymargin(void)
{
    return ymargin;
}

void lcd_remote_setfont(int newfont)
{
    curfont = newfont;
}

int lcd_remote_getfont(void)
{
    return curfont;
}

int lcd_remote_getstringsize(const unsigned char *str, int *w, int *h)
{
    return font_getstringsize(str, w, h, curfont);
}

/*** low-level drawing functions ***/

static void setpixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] |= 1 << (y & 7);
}

static void clearpixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] &= ~(1 << (y & 7));
}

static void flippixel(int x, int y)
{
    lcd_remote_framebuffer[y>>3][x] ^= 1 << (y & 7);
}

static void nopixel(int x, int y)
{
    (void)x;
    (void)y;
}

lcd_remote_pixelfunc_type* const lcd_remote_pixelfuncs[8] = {
    flippixel, nopixel, setpixel, setpixel,
    nopixel, clearpixel, nopixel, clearpixel
};

static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
                      ICODE_ATTR;
static void flipblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= bits & mask;
}

static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void bgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address &= bits | ~mask;
}

static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
                    ICODE_ATTR;
static void fgblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address |= bits & mask;
}

static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void solidblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    bits    ^= data;
    *address = data ^ (bits & mask);
}

static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                         ICODE_ATTR;
static void flipinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address ^= ~bits & mask;
}

static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void bginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address &= ~(bits & mask);
}

static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                       ICODE_ATTR;
static void fginvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    *address |= ~bits & mask;
}

static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
                          ICODE_ATTR;
static void solidinvblock(fb_remote_data *address, unsigned mask, unsigned bits)
{
    unsigned data = *address;

    bits     = ~bits ^ data;
    *address = data ^ (bits & mask);
}

lcd_remote_blockfunc_type* const lcd_remote_blockfuncs[8] = {
    flipblock, bgblock, fgblock, solidblock,
    flipinvblock, bginvblock, fginvblock, solidinvblock
};

/*** drawing functions ***/

/* Clear the whole display */
void lcd_remote_clear_display(void)
{
    unsigned bits = (drawmode & DRMODE_INVERSEVID) ? 0xFFu : 0;

    memset(lcd_remote_framebuffer, bits, sizeof lcd_remote_framebuffer);
    lcd_remote_scroll_info.lines = 0;
}

/* Set a single pixel */
void lcd_remote_drawpixel(int x, int y)
{
    if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
        lcd_remote_pixelfuncs[drawmode](x, y);
}

/* Draw a line */
void lcd_remote_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    lcd_remote_pixelfunc_type *pfunc = lcd_remote_pixelfuncs[drawmode];

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i = 0; i < numpixels; i++)
    {
        if (((unsigned)x < LCD_REMOTE_WIDTH) && ((unsigned)y < LCD_REMOTE_HEIGHT))
            pfunc(x, y);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a horizontal line (optimised) */
void lcd_remote_hline(int x1, int x2, int y)
{
    int x;
    fb_remote_data *dst, *dst_end;
    unsigned mask;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (x2 < x1)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }

    /* nothing to draw? */
    if (((unsigned)y >= LCD_REMOTE_HEIGHT) || (x1 >= LCD_REMOTE_WIDTH)
        || (x2 < 0))
        return;

    /* clipping */
    if (x1 < 0)
        x1 = 0;
    if (x2 >= LCD_REMOTE_WIDTH)
        x2 = LCD_REMOTE_WIDTH-1;

    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x1];
    mask  = 1 << (y & 7);

    dst_end = dst + x2 - x1;
    do
        bfunc(dst++, mask, 0xFFu);
    while (dst <= dst_end);
}

/* Draw a vertical line (optimised) */
void lcd_remote_vline(int x, int y1, int y2)
{
    int ny;
    fb_remote_data *dst;
    unsigned mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* direction flip */
    if (y2 < y1)
    {
        ny = y1;
        y1 = y2;
        y2 = ny;
    }

    /* nothing to draw? */
    if (((unsigned)x >= LCD_REMOTE_WIDTH) || (y1 >= LCD_REMOTE_HEIGHT)
        || (y2 < 0))
        return;

    /* clipping */
    if (y1 < 0)
        y1 = 0;
    if (y2 >= LCD_REMOTE_HEIGHT)
        y2 = LCD_REMOTE_HEIGHT-1;

    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y1>>3][x];
    ny    = y2 - (y1 & ~7);
    mask  = 0xFFu << (y1 & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        bfunc(dst, mask, 0xFFu);
        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;
    bfunc(dst, mask, 0xFFu);
}

/* Draw a rectangular box */
void lcd_remote_drawrect(int x, int y, int width, int height)
{
    if ((width <= 0) || (height <= 0))
        return;

    int x2 = x + width - 1;
    int y2 = y + height - 1;

    lcd_remote_vline(x, y, y2);
    lcd_remote_vline(x2, y, y2);
    lcd_remote_hline(x, x2, y);
    lcd_remote_hline(x, x2, y2);
}

/* Fill a rectangular area */
void lcd_remote_fillrect(int x, int y, int width, int height)
{
    int ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;
    unsigned bits = 0;
    lcd_remote_blockfunc_type *bfunc;
    bool fillopt = false;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH)
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    if (drawmode & DRMODE_INVERSEVID)
    {
        if (drawmode & DRMODE_BG)
        {
            fillopt = true;
        }
    }
    else
    {
        if (drawmode & DRMODE_FG)
        {
            fillopt = true;
            bits = 0xFFu;
        }
    }
    bfunc = lcd_remote_blockfuncs[drawmode];
    dst   = &lcd_remote_framebuffer[y>>3][x];
    ny    = height - 1 + (y & 7);
    mask  = 0xFFu << (y & 7);
    mask_bottom = 0xFFu >> (~ny & 7);

    for (; ny >= 8; ny -= 8)
    {
        if (fillopt && (mask == 0xFFu))
            memset(dst, bits, width);
        else
        {
            fb_remote_data *dst_row = dst;

            dst_end = dst_row + width;
            do
                bfunc(dst_row++, mask, 0xFFu);
            while (dst_row < dst_end);
        }

        dst += LCD_REMOTE_WIDTH;
        mask = 0xFFu;
    }
    mask &= mask_bottom;

    if (fillopt && (mask == 0xFFu))
        memset(dst, bits, width);
    else
    {
        dst_end = dst + width;
        do
            bfunc(dst++, mask, 0xFFu);
        while (dst < dst_end);
    }
}

/* About Rockbox' internal bitmap format:
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * black (1) or white (0). Bits within a byte are arranged vertically, LSB
 * at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * This is the same as the internal lcd hw format. */

/* Draw a partial bitmap */
void lcd_remote_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
                            ICODE_ATTR;
void lcd_remote_bitmap_part(const unsigned char *src, int src_x, int src_y,
                            int stride, int x, int y, int width, int height)
{
    int shift, ny;
    fb_remote_data *dst, *dst_end;
    unsigned mask, mask_bottom;
    lcd_remote_blockfunc_type *bfunc;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_REMOTE_WIDTH)
        || (y >= LCD_REMOTE_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    /* clipping */
    if (x < 0)
    {
        width += x;
        src_x -= x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        src_y -= y;
        y = 0;
    }
    if (x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (y + height > LCD_REMOTE_HEIGHT)
        height = LCD_REMOTE_HEIGHT - y;

    src   += stride * (src_y >> 3) + src_x; /* move starting point */
    src_y &= 7;
    y     -= src_y;
    dst    = &lcd_remote_framebuffer[y>>3][x];
    shift  = y & 7;
    ny     = height - 1 + shift + src_y;

    bfunc  = lcd_remote_blockfuncs[drawmode];
    mask   = 0xFFu << (shift + src_y);
    mask_bottom = 0xFFu >> (~ny & 7);

    if (shift == 0)
    {
        bool copyopt = (drawmode == DRMODE_SOLID);

        for (; ny >= 8; ny -= 8)
        {
            if (copyopt && (mask == 0xFFu))
                memcpy(dst, src, width);
            else
            {
                const unsigned char *src_row = src;
                fb_remote_data *dst_row = dst;

                dst_end = dst_row + width;
                do
                    bfunc(dst_row++, mask, *src_row++);
                while (dst_row < dst_end);
            }

            src += stride;
            dst += LCD_REMOTE_WIDTH;
            mask = 0xFFu;
        }
        mask &= mask_bottom;

        if (copyopt && (mask == 0xFFu))
            memcpy(dst, src, width);
        else
        {
            dst_end = dst + width;
            do
                bfunc(dst++, mask, *src++);
            while (dst < dst_end);
        }
    }
    else
    {
        dst_end = dst + width;
        do
        {
            const unsigned char *src_col = src++;
            fb_remote_data *dst_col = dst++;
            unsigned mask_col = mask;
            unsigned data = 0;

            for (y = ny; y >= 8; y -= 8)
            {
                data |= *src_col << shift;

                if (mask_col & 0xFFu)
                {
                    bfunc(dst_col, mask_col, data);
                    mask_col = 0xFFu;
                }
                else
                    mask_col >>= 8;

                src_col += stride;
                dst_col += LCD_REMOTE_WIDTH;
                data >>= 8;
            }
            data |= *src_col << shift;
            bfunc(dst_col, mask_col & mask_bottom, data);
        }
        while (dst < dst_end);
    }
}

/* Draw a full bitmap */
void lcd_remote_bitmap(const unsigned char *src, int x, int y, int width,
                       int height)
{
    lcd_remote_bitmap_part(src, 0, 0, width, x, y, width, height);
}

/* put a string at a given pixel position, skipping first ofs pixel columns */
void lcd_remote_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ch;
    unsigned short *ucs;
    struct font* pf = font_get(curfont);

    ucs = bidi_l2v(str, 1);

    while ((ch = *ucs++) != 0 && x < LCD_REMOTE_WIDTH)
    {
        int width;
        const unsigned char *bits;

        /* get proportional width and glyph bits */
        width = font_get_width(pf, ch);

        if (ofs > width)
        {
            ofs -= width;
            continue;
        }

        bits = font_get_bits(pf, ch);

        lcd_remote_bitmap_part(bits, ofs, 0, width, x, y, width - ofs,
                               pf->height);

        x += width - ofs;
        ofs = 0;
    }
}

/* put a string at a given pixel position */
void lcd_remote_putsxy(int x, int y, const unsigned char *str)
{
    lcd_remote_putsxyofs(x, y, 0, str);
}

/*** line oriented text output ***/

/* put a string at a given char position */
void lcd_remote_puts(int x, int y, const unsigned char *str)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, 0);
}

void lcd_remote_puts_style(int x, int y, const unsigned char *str, int style)
{
    lcd_remote_puts_style_offset(x, y, str, style, 0);
}

void lcd_remote_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    lcd_remote_puts_style_offset(x, y, str, STYLE_DEFAULT, offset);
}

/* put a string at a given char position, style, and pixel position,
 * skipping first offset pixel columns */
void lcd_remote_puts_style_offset(int x, int y, const unsigned char *str,
                                  int style, int offset)
{
    int xpos,ypos,w,h,xrect;
    int lastmode = drawmode;

    /* make sure scrolling is turned off on the line we are updating */
    lcd_remote_scroll_info.lines &= ~(1 << y);

    if(!str || !str[0])
        return;

    lcd_remote_getstringsize(str, &w, &h);
    xpos = xmargin + x*w / utf8length((char *)str);
    ypos = ymargin + y*h;
    drawmode = (style & STYLE_INVERT) ?
               (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
    lcd_remote_putsxyofs(xpos, ypos, offset, str);
    drawmode ^= DRMODE_INVERSEVID;
    xrect = xpos + MAX(w - offset, 0);
    lcd_remote_fillrect(xrect, ypos, LCD_REMOTE_WIDTH - xrect, h);
    drawmode = lastmode;
}

/*** scrolling ***/

void lcd_remote_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_remote_puts_scroll_style(x, y, string, STYLE_DEFAULT);
}

void lcd_remote_puts_scroll_style(int x, int y, const unsigned char *string, int style)
{
    lcd_remote_puts_scroll_style_offset(x, y, string, style, 0);
}

void lcd_remote_puts_scroll_offset(int x, int y, const unsigned char *string, int offset)
{
    lcd_remote_puts_scroll_style_offset(x, y, string, STYLE_DEFAULT, offset);
}

void lcd_remote_puts_scroll_style_offset(int x, int y, const unsigned char *string,
                                         int style, int offset)
{
    struct scrollinfo* s;
    int w, h;

    if(y>=LCD_REMOTE_SCROLLABLE_LINES) return;

    s = &lcd_remote_scroll_info.scroll[y];

    s->start_tick = current_tick + lcd_remote_scroll_info.delay;
    s->invert = false;
    if (style & STYLE_INVERT) {
        s->invert = true;
        lcd_remote_puts_style_offset(x,y,string,STYLE_INVERT,offset);
    }
    else
        lcd_remote_puts_offset(x,y,string,offset);

    lcd_remote_getstringsize(string, &w, &h);

    if (LCD_REMOTE_WIDTH - x * 8 - xmargin < w) {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, (char *)string);

        /* get width */
        s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);

        /* scroll bidirectional or forward only depending on the string
           width */
        if ( lcd_remote_scroll_info.bidir_limit ) {
            s->bidir = s->width < (LCD_REMOTE_WIDTH - xmargin) *
                (100 + lcd_remote_scroll_info.bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir) { /* add spaces if scrolling in the round */
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->width = lcd_remote_getstringsize((unsigned char *)s->line, &w, &h);
        }

        end = strchr(s->line, '\0');
        strncpy(end, (char *)string, LCD_REMOTE_WIDTH/2);

        s->len = utf8length((char *)string);
        s->offset = offset;
        s->startx = xmargin + x * s->width / s->len;;
        s->backward = false;
        lcd_remote_scroll_info.lines |= (1<<y);
    }
    else
        /* force a bit switch-off since it doesn't scroll */
        lcd_remote_scroll_info.lines &= ~(1<<y);
}

void lcd_remote_scroll_fn(void)
{
    struct font* pf;
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    int lastmode;

    for ( index = 0; index < LCD_REMOTE_SCROLLABLE_LINES; index++ ) {
        /* really scroll? */
        if ((lcd_remote_scroll_info.lines & (1 << index)) == 0)
            continue;

        s = &lcd_remote_scroll_info.scroll[index];

        /* check pause */
        if (TIME_BEFORE(current_tick, s->start_tick))
            continue;

        if (s->backward)
            s->offset -= lcd_remote_scroll_info.step;
        else
            s->offset += lcd_remote_scroll_info.step;

        pf = font_get(curfont);
        xpos = s->startx;
        ypos = ymargin + index * pf->height;

        if (s->bidir) { /* scroll bidirectional */
            if (s->offset <= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->backward = false;
                s->start_tick = current_tick + lcd_remote_scroll_info.delay*2;
            }
            if (s->offset >= s->width - (LCD_REMOTE_WIDTH - xpos)) {
                /* at end of line */
                s->offset = s->width - (LCD_REMOTE_WIDTH - xpos);
                s->backward = true;
                s->start_tick = current_tick + lcd_remote_scroll_info.delay*2;
            }
        }
        else {
            /* scroll forward the whole time */
            if (s->offset >= s->width)
                s->offset %= s->width;
        }

        lastmode = drawmode;
        drawmode = s->invert ?
            (DRMODE_SOLID|DRMODE_INVERSEVID) : DRMODE_SOLID;
        lcd_remote_putsxyofs(xpos, ypos, s->offset, s->line);
        drawmode = lastmode;
        lcd_remote_update_rect(xpos, ypos, LCD_REMOTE_WIDTH - xpos, pf->height);
    }
}

/* LCD init */
void lcd_remote_init(void)
{
#ifndef SIMULATOR
    /* Call device specific init */
    lcd_remote_init_device();
#endif
}
