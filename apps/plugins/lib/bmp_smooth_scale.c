/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Code for the scaling algorithm:
 * Imlib2 is (C) Carsten Haitzler and various contributors. The MMX code
 * is by Willem Monsuwe <willem@stack.nl>. Additional modifications are by
 * (C) Daniel M. Duley.
 *
 * Port to Rockbox
 * Copyright (C) 2007 Jonas Hurrelmann (j@outpo.st)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Copyright (C) 2004, 2005 Daniel M. Duley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* OTHER CREDITS:
 *
 * This is the normal smoothscale method, based on Imlib2's smoothscale.
 *
 * Originally I took the algorithm used in NetPBM and Qt and added MMX/3dnow
 * optimizations. It ran in about 1/2 the time as Qt. Then I ported Imlib's
 * C algorithm and it ran at about the same speed as my MMX optimized one...
 * Finally I ported Imlib's MMX version and it ran in less than half the
 * time as my MMX algorithm, (taking only a quarter of the time Qt does).
 * After further optimization it seems to run at around 1/6th.
 *
 * Changes include formatting, namespaces and other C++'ings, removal of old
 * #ifdef'ed code, and removal of unneeded border calculation code.
 *
 * Imlib2 is (C) Carsten Haitzler and various contributors. The MMX code
 * is by Willem Monsuwe <willem@stack.nl>. All other modifications are
 * (C) Daniel M. Duley.
 */

#include "bmp.h"
#include "lcd.h"

void smooth_resize_bitmap(struct bitmap *src_bmp,  struct bitmap *dest_bmp)
{
    fb_data *sptr, *dptr;
    int x, y, end;
    int val_y = 0, val_x;
    const int sw = src_bmp->width;
    const int sh = src_bmp->height;
    const int dw = dest_bmp->width;
    const int dh = dest_bmp->height;
    const int inc_x = (sw << 16) / dw;
    const int inc_y = (sh << 16) / dh;
    const int Cp_x = ((dw << 14) / sw) + 1;
    const int Cp_y = ((dh << 14) / sh) + 1;
    const int xup_yup = (dw >= sw) + ((dh >= sh) << 1);
    const int dow = dw;
    const int sow = sw;
    fb_data *src = (fb_data*)src_bmp->data;
    fb_data *dest = (fb_data*)dest_bmp->data;
    int XAP, YAP, INV_YAP, INV_XAP;
    int xpoint;
    fb_data *ypoint;

    end = dw;
    /* scaling up both ways */
    if (xup_yup == 3) {
        /* go through every scanline in the output buffer */
        for (y = 0; y < dh; y++) {
            /* calculate the source line we'll scan from */
            ypoint = src + ((val_y >> 16) * sw);
            YAP = ((val_y >> 16) >= (sh - 1)) ? 0 : (val_y >> 8) - ((val_y >> 8) & 0xffffff00);
            INV_YAP = 256 - YAP;

            val_y += inc_y;
            val_x = 0;

            dptr = dest + (y * dow);
            sptr = ypoint;
            if (YAP > 0) {
                for (x = 0; x < end; x++) {
                    int r = 0, g = 0, b = 0;
                    int rr = 0, gg = 0, bb = 0;
                    fb_data *pix;

                    xpoint = (val_x >> 16);
                    XAP = ((val_x >> 16) >= (sw - 1)) ? 0 : (val_x >> 8) - ((val_x >> 8) & 0xffffff00);
                    INV_XAP = 256 - XAP;
                    val_x += inc_x;

                    if (XAP > 0) {
                        pix = ypoint + xpoint;
                        r = RGB_UNPACK_RED(*pix) * INV_XAP;
                        g = RGB_UNPACK_GREEN(*pix) * INV_XAP;
                        b = RGB_UNPACK_BLUE(*pix) * INV_XAP;
                        pix++;
                        r += RGB_UNPACK_RED(*pix) * XAP;
                        g += RGB_UNPACK_GREEN(*pix) * XAP;
                        b += RGB_UNPACK_BLUE(*pix) * XAP;
                        pix += sow;
                        rr = RGB_UNPACK_RED(*pix) * XAP;
                        gg = RGB_UNPACK_GREEN(*pix) * XAP;
                        bb = RGB_UNPACK_BLUE(*pix) * XAP;
                        pix--;
                        rr += RGB_UNPACK_RED(*pix) * INV_XAP;
                        gg += RGB_UNPACK_GREEN(*pix) * INV_XAP;
                        bb += RGB_UNPACK_BLUE(*pix) * INV_XAP;
                        r = ((rr * YAP) + (r * INV_YAP)) >> 16;
                        g = ((gg * YAP) + (g * INV_YAP)) >> 16;
                        b = ((bb * YAP) + (b * INV_YAP)) >> 16;
                        *dptr++ = LCD_RGBPACK(r, g, b);
                    } else {
                        pix = ypoint + xpoint;
                        r = RGB_UNPACK_RED(*pix) * INV_YAP;
                        g = RGB_UNPACK_GREEN(*pix) * INV_YAP;
                        b = RGB_UNPACK_BLUE(*pix) * INV_YAP;
                        pix += sow;
                        r += RGB_UNPACK_RED(*pix) * YAP;
                        g += RGB_UNPACK_GREEN(*pix) * YAP;
                        b += RGB_UNPACK_BLUE(*pix) * YAP;
                        r >>= 8;
                        g >>= 8;
                        b >>= 8;
                        *dptr++ = LCD_RGBPACK(r, g, b);
                    }
                }
            } else {
                for (x = 0; x < end; x++) {
                    int r = 0, g = 0, b = 0;
                    fb_data *pix;

                    xpoint = (val_x >> 16);
                    XAP = ((val_x >> 16) >= (sw - 1)) ? 0 : (val_x >> 8) - ((val_x >> 8) & 0xffffff00);
                    INV_XAP = 256 - XAP;
                    val_x += inc_x;

                    if (XAP > 0) {
                        pix = ypoint + xpoint;
                        r = RGB_UNPACK_RED(*pix) * INV_XAP;
                        g = RGB_UNPACK_GREEN(*pix) * INV_XAP;
                        b = RGB_UNPACK_BLUE(*pix) * INV_XAP;
                        pix++;
                        r += RGB_UNPACK_RED(*pix) * XAP;
                        g += RGB_UNPACK_GREEN(*pix) * XAP;
                        b += RGB_UNPACK_BLUE(*pix) * XAP;
                        r >>= 8;
                        g >>= 8;
                        b >>= 8;
                        *dptr++ = LCD_RGBPACK(r, g, b);
                    } else
                        *dptr++ = sptr[xpoint];
                }
            }
        }
    }
    /* if we're scaling down vertically */
    else if (xup_yup == 1) {
        /*\ 'Correct' version, with math units prepared for MMXification \ */
        int Cy, j;
        fb_data *pix;
        int r, g, b, rr, gg, bb;
        int yap;

        /* go through every scanline in the output buffer */
        for (y = 0; y < dh; y++) {
            ypoint = src + ((val_y >> 16) * sw);
            YAP = (((0x100 - ((val_y >> 8) & 0xff)) * Cp_y) >> 8) | (Cp_y << 16);
            INV_YAP = 256 - YAP;
            val_y += inc_y;
            val_x = 0;

            Cy = YAP >> 16;
            yap = YAP & 0xffff;


            dptr = dest + (y * dow);
            for (x = 0; x < end; x++) {
                xpoint = (val_x >> 16);
                XAP = ((val_x >> 16) >= (sw - 1)) ? 0 : (val_x >> 8) - ((val_x >> 8) & 0xffffff00);
                INV_XAP = 256 - XAP;
                val_x += inc_x;

                pix = ypoint + xpoint;
                r = (RGB_UNPACK_RED(*pix) * yap) >> 10;
                g = (RGB_UNPACK_GREEN(*pix) * yap) >> 10;
                b = (RGB_UNPACK_BLUE(*pix) * yap) >> 10;
                pix += sow;
                for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                    r += (RGB_UNPACK_RED(*pix) * Cy) >> 10;
                    g += (RGB_UNPACK_GREEN(*pix) * Cy) >> 10;
                    b += (RGB_UNPACK_BLUE(*pix) * Cy) >> 10;
                    pix += sow;
                }
                if (j > 0) {
                    r += (RGB_UNPACK_RED(*pix) * j) >> 10;
                    g += (RGB_UNPACK_GREEN(*pix) * j) >> 10;
                    b += (RGB_UNPACK_BLUE(*pix) * j) >> 10;
                }
                if (XAP > 0) {
                    pix = ypoint + xpoint + 1;
                    rr = (RGB_UNPACK_RED(*pix) * yap) >> 10;
                    gg = (RGB_UNPACK_GREEN(*pix) * yap) >> 10;
                    bb = (RGB_UNPACK_BLUE(*pix) * yap) >> 10;
                    pix += sow;
                    for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                        rr += (RGB_UNPACK_RED(*pix) * Cy) >> 10;
                        gg += (RGB_UNPACK_GREEN(*pix) * Cy) >> 10;
                        bb += (RGB_UNPACK_BLUE(*pix) * Cy) >> 10;
                        pix += sow;
                    }
                    if (j > 0) {
                        rr += (RGB_UNPACK_RED(*pix) * j) >> 10;
                        gg += (RGB_UNPACK_GREEN(*pix) * j) >> 10;
                        bb += (RGB_UNPACK_BLUE(*pix) * j) >> 10;
                    }
                    r = r * INV_XAP;
                    g = g * INV_XAP;
                    b = b * INV_XAP;
                    r = (r + ((rr * XAP))) >> 12;
                    g = (g + ((gg * XAP))) >> 12;
                    b = (b + ((bb * XAP))) >> 12;
                } else {
                    r >>= 4;
                    g >>= 4;
                    b >>= 4;
                }
                *dptr = LCD_RGBPACK(r, g, b);
                dptr++;
            }
        }
    }
    /* if we're scaling down horizontally */
    else if (xup_yup == 2) {
        /*\ 'Correct' version, with math units prepared for MMXification \ */
        int Cx, j;
        fb_data *pix;
        int r, g, b, rr, gg, bb;
        int xap;

        /* go through every scanline in the output buffer */
        for (y = 0; y < dh; y++) {
            ypoint = src + ((val_y >> 16) * sw);
            YAP = ((val_y >> 16) >= (sh - 1)) ? 0 : (val_y >> 8) - ((val_y >> 8) & 0xffffff00);
            INV_YAP = 256 - YAP;
            val_y += inc_y;
            val_x = 0;

            dptr = dest + (y * dow);
            for (x = 0; x < end; x++) {
                xpoint = (val_x >> 16);
                XAP = (((0x100 - ((val_x >> 8) & 0xff)) * Cp_x) >> 8) | (Cp_x << 16);
                INV_XAP = 256 - XAP;

                val_x += inc_x;

                Cx = XAP >> 16;
                xap = XAP & 0xffff;

                pix = ypoint + xpoint;
                r = (RGB_UNPACK_RED(*pix) * xap) >> 10;
                g = (RGB_UNPACK_GREEN(*pix) * xap) >> 10;
                b = (RGB_UNPACK_BLUE(*pix) * xap) >> 10;
                pix++;
                for (j = (1 << 14) - xap; j > Cx; j -= Cx) {
                    r += (RGB_UNPACK_RED(*pix) * Cx) >> 10;
                    g += (RGB_UNPACK_GREEN(*pix) * Cx) >> 10;
                    b += (RGB_UNPACK_BLUE(*pix) * Cx) >> 10;
                    pix++;
                }
                if (j > 0) {
                    r += (RGB_UNPACK_RED(*pix) * j) >> 10;
                    g += (RGB_UNPACK_GREEN(*pix) * j) >> 10;
                    b += (RGB_UNPACK_BLUE(*pix) * j) >> 10;
                }
                if (YAP > 0) {
                    pix = ypoint + xpoint + sow;
                    rr = (RGB_UNPACK_RED(*pix) * xap) >> 10;
                    gg = (RGB_UNPACK_GREEN(*pix) * xap) >> 10;
                    bb = (RGB_UNPACK_BLUE(*pix) * xap) >> 10;
                    pix++;
                    for (j = (1 << 14) - xap; j > Cx; j -= Cx) {
                        rr += (RGB_UNPACK_RED(*pix) * Cx) >> 10;
                        gg += (RGB_UNPACK_GREEN(*pix) * Cx) >> 10;
                        bb += (RGB_UNPACK_BLUE(*pix) * Cx) >> 10;
                        pix++;
                    }
                    if (j > 0) {
                        rr += (RGB_UNPACK_RED(*pix) * j) >> 10;
                        gg += (RGB_UNPACK_GREEN(*pix) * j) >> 10;
                        bb += (RGB_UNPACK_BLUE(*pix) * j) >> 10;
                    }
                    r = r * INV_YAP;
                    g = g * INV_YAP;
                    b = b * INV_YAP;
                    r = (r + ((rr * YAP))) >> 12;
                    g = (g + ((gg * YAP))) >> 12;
                    b = (b + ((bb * YAP))) >> 12;
                } else {
                    r >>= 4;
                    g >>= 4;
                    b >>= 4;
                }
                *dptr = LCD_RGBPACK(r, g, b);
                dptr++;
            }
        }
    }
    /* fully optimized (i think) - only change of algorithm can help */
    /* if we're scaling down horizontally & vertically */
    else {
        /*\ 'Correct' version, with math units prepared for MMXification \ */
        int Cx, Cy, i, j;
        fb_data *pix;
        int r, g, b, rx, gx, bx;
        int xap, yap;

        for (y = 0; y < dh; y++) {
            ypoint = src + ((val_y >> 16) * sw);
            YAP = (((0x100 - ((val_y >> 8) & 0xff)) * Cp_y) >> 8) | (Cp_y << 16);
            INV_YAP = 256 - YAP;
            val_y += inc_y;
            val_x = 0;

            Cy = YAP >> 16;
            yap = YAP & 0xffff;

            dptr = dest + (y * dow);
            for (x = 0; x < end; x++) {
                xpoint = (val_x >> 16);
                XAP = (((0x100 - ((val_x >> 8) & 0xff)) * Cp_x) >> 8) | (Cp_x << 16);
                INV_XAP = 256 - XAP;
                val_x += inc_x;

                Cx = XAP >> 16;
                xap = XAP & 0xffff;

                sptr = ypoint + xpoint;

                pix = sptr;
                sptr += sow;
                rx = (RGB_UNPACK_RED(*pix) * xap) >> 9;
                gx = (RGB_UNPACK_GREEN(*pix) * xap) >> 9;
                bx = (RGB_UNPACK_BLUE(*pix) * xap) >> 9;
                pix++;
                for (i = (1 << 14) - xap; i > Cx; i -= Cx) {
                    rx += (RGB_UNPACK_RED(*pix) * Cx) >> 9;
                    gx += (RGB_UNPACK_GREEN(*pix) * Cx) >> 9;
                    bx += (RGB_UNPACK_BLUE(*pix) * Cx) >> 9;
                    pix++;
                }
                if (i > 0) {
                    rx += (RGB_UNPACK_RED(*pix) * i) >> 9;
                    gx += (RGB_UNPACK_GREEN(*pix) * i) >> 9;
                    bx += (RGB_UNPACK_BLUE(*pix) * i) >> 9;
                }

                r = (rx * yap) >> 14;
                g = (gx * yap) >> 14;
                b = (bx * yap) >> 14;

                for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                    pix = sptr;
                    sptr += sow;
                    rx = (RGB_UNPACK_RED(*pix) * xap) >> 9;
                    gx = (RGB_UNPACK_GREEN(*pix) * xap) >> 9;
                    bx = (RGB_UNPACK_BLUE(*pix) * xap) >> 9;
                    pix++;
                    for (i = (1 << 14) - xap; i > Cx; i -= Cx) {
                        rx += (RGB_UNPACK_RED(*pix) * Cx) >> 9;
                        gx += (RGB_UNPACK_GREEN(*pix) * Cx) >> 9;
                        bx += (RGB_UNPACK_BLUE(*pix) * Cx) >> 9;
                        pix++;
                    }
                    if (i > 0) {
                        rx += (RGB_UNPACK_RED(*pix) * i) >> 9;
                        gx += (RGB_UNPACK_GREEN(*pix) * i) >> 9;
                        bx += (RGB_UNPACK_BLUE(*pix) * i) >> 9;
                    }

                    r += (rx * Cy) >> 14;
                    g += (gx * Cy) >> 14;
                    b += (bx * Cy) >> 14;
                }
                if (j > 0) {
                    pix = sptr;
                    sptr += sow;
                    rx = (RGB_UNPACK_RED(*pix) * xap) >> 9;
                    gx = (RGB_UNPACK_GREEN(*pix) * xap) >> 9;
                    bx = (RGB_UNPACK_BLUE(*pix) * xap) >> 9;
                    pix++;
                    for (i = (1 << 14) - xap; i > Cx; i -= Cx) {
                        rx += (RGB_UNPACK_RED(*pix) * Cx) >> 9;
                        gx += (RGB_UNPACK_GREEN(*pix) * Cx) >> 9;
                        bx += (RGB_UNPACK_BLUE(*pix) * Cx) >> 9;
                        pix++;
                    }
                    if (i > 0) {
                        rx += (RGB_UNPACK_RED(*pix) * i) >> 9;
                        gx += (RGB_UNPACK_GREEN(*pix) * i) >> 9;
                        bx += (RGB_UNPACK_BLUE(*pix) * i) >> 9;
                    }

                    r += (rx * j) >> 14;
                    g += (gx * j) >> 14;
                    b += (bx * j) >> 14;
                }

                *dptr = LCD_RGBPACK(r >> 5, g >> 5, b >> 5);
                dptr++;
            }
        }
    }
}
