/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


/*
 * palette creation
 * contains lots of experimantal code
 */

/* Standard Includes */
#include "rbconfig.h"
#include "rb_test.h"
#include "fixedpoint.h"

/* 2600 includes */
#include "vmachine.h"
#include "address.h"
#include "keyboard.h"
#include "rb_lcd.h"
#include "lcd.h"

#include "palette.h"

/* helper macro to clamp value to 8 bit range: 0 <= a <= 255 */
#define CLAMP8(a)       ((a)<0 ? 0 : (a)>255 ? 255 : (a))

struct palette {
    enum tv_system type; /* TV_NTSC / TV_PAL / TV_SECAM */
    int brightness;     /* min and max_l lightness in % 0..100 */
    int contrast;
    int saturation;     /* saturation as % value 0..100 */
    int c_shift;        /* colour shift -50..50 of 1/6 colour wheel */
    int col_step;
    // experimental parameters; to be removed!
    int yuv_mode;
    int pr_displacement,
        pb_displacement;
};


static struct palette palette =
{
    .type = TV_NTSC,
    .brightness = 5,
    .contrast = 0,
    .saturation = 35,
    .c_shift = 12,
    .col_step = 30,
};


/*
 * fixed point stuff
 */

typedef int32_t  fixed;
#define FRACBITS    16

/* helper functions */
#define int2fp(i)           ((fixed)((i)<<FRACBITS))
#define fp2int(val)         ((int)((val)>>FRACBITS)) /* not rounded! */
#define float2fp(val)       ((fixed)(val*(float)(1 << FRACBITS)))

/* we use 16 fractional bits */
#define fp16_sin(val)       (fp14_sin(val)<<2)  /* NOTE: argument is degree */
#define fp16_cos(val)       (fp14_cos(val)<<2)  /*       value (0..359) */

#define fp16_mul(x, y)      fp_mul(x, y, FRACBITS)
#define fp16_div(x, y)      fp_div(x, y, FRACBITS)


/* ----------------------------------------------------- */


#if LCD_DEPTH==1
/*
 * each dither pattern is stored as 32 bit word and holds a 8x4 pattern
 * (8 bit x 4 byte). Note that the orientation on the screen depends on the LCD
 * orientation, i.e. on a vertical packing LCD the 8 bit of every byte form a
 * vertical line.
 */
/* 2x2 dither */
static uint32_t dither[] = {
    0x00000000,
    0x00000000,
    0x00550055,
    0xAA00AA00,
    0x55AA55AA,
    0x55555555,
    0xFFAAFFAA,
    0x55FF55FF,
    0xFFFFFFFF,
    0xFFFFFFFF
};
#elif LCD_DEPTH==2
/*
 * each dither pattern is stored as a 32 bit word holds a 4x4 pattern
 * (4*2 bit x 4 byte). Note that the actual output orientation depends on the
 * LCD orientation, i.e. on a vertical packing LCD the 8 bit of every byte
 * form a vertical line.
 */
# if 1
/* 2x2 dither */
static uint32_t dither[] = {
    0x00000000, /* all black */
    0x00000000,
    0x00110011,
    0x44004400,
    0x44114411,
    0x11441144,
    0x44554455,
    0x55115511,
    0x55555555, /* all dark grey */
    0x22882288,
    0x55665566,
    0x99559955,
    0x66996699,
    0x99669966,
    0x99AA99AA,
    0xAA66AA66,
    0xAAAAAAAA, /* all light grey */
    0x77DD77DD,
    0xAABBAABB,
    0xEEAAEEAA,
    0xBBEEBBEE,
    0xEEBBEEBB,
    0xEEFFEEFF,
    0xFFBBFFBB,
    0xFFFFFFFF, /* all white */
    0xFFFFFFFF
};
# else
/* 2x1 dither */
static uint32_t dither[] = {
    0x00000000, /* all black */
    0x11111111,
    0x55555555, /* all dark grey */
    0x99999999,
    0xAAAAAAAA, /* all light grey */
    0xBBBBBBBB,
    0xFFFFFFFF  /* all white */
};
# endif
#endif

// TODO: to be implemented
#if LCD_DEPTH==2
static uint32_t dither_none[] = {
    0x00000000, /* all black */
    0x55555555, /* all dark grey */
    0xAAAAAAAA, /* all light grey */
    0xFFFFFFFF  /* all white */
};
#endif

#if LCD_DEPTH>=8
fb_data vcs_palette[NUMCOLS];   /* holds colour in framebuffer format */
#else
uint32_t vcs_palette[NUMCOLS];  /* holds dither pattern */
#endif

/* convert colour to lightness value for greyscale targets */
#define RGB2Y(r,g,b)    ((77*(r) + 150*(g) + 29*(b)) >> 8)


/* =========================================================================
 * display palette on lcd
 */
void show_palette(void) COLD_ATTR;
void show_palette(void)
{
    //rb->lcd_clear_display();
    rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int y = 0; y < LCD_HEIGHT; ++y) {
        for (int x = 0; x < LCD_WIDTH; ++x) {
#if 1       /* colours horizontal, brightness vertical */
            int vcs_colour = (((x * 16 / LCD_WIDTH) << 4)
                            | (y * 16 / LCD_HEIGHT));
#else       /* colours vertical, brightness horizontal */
            int vcs_colour = (((y * 16 / LCD_HEIGHT) << 4)
                            | (x * 16 / LCD_WIDTH));
#endif
#if NUMCOLS < 256
            vcs_colour >>= 1;
#endif
#if LCD_DEPTH>=8
            rb->lcd_set_foreground(FB_UNPACK_SCALAR_LCD(vcs_palette[vcs_colour]));
            rb->lcd_drawpixel(x, y);
#else /* greyscale/mono */
            put_pixel(vcs_palette[vcs_colour], x, y);
#endif
        }
    }
    rb->lcd_update();
}

/* =========================================================================
 *
 */
// YCbCr/YPbPr/YUV - colour scheme conversion
/*
 * yuv_mode: experimental
 * h=hue -> 0..3600 (10th degree)
 * s=saturation 0..1
 * l=lightness 0..1
 * rr,gg,bb= int rgb values
 */
static void hsl2rgb(int h, fixed s, fixed l, int *rr, int *gg, int *bb) COLD_ATTR;
static void hsl2rgb(int h, fixed s, fixed l, int *rr, int *gg, int *bb)
{
    fixed r, g, b;
    fixed y = l;
    fixed pb = fp16_cos(h/10);
    fixed pr = fp16_sin(h/10);

    int yuv_mode = palette.yuv_mode;
    int pr_displacement = palette.pr_displacement;
    int pb_displacement = palette.pb_displacement;

    if (yuv_mode==4) { // YUV
        pb = fp16_mul(pb, float2fp(0.872));
        pr = fp16_mul(pr, float2fp(1.23));
        pb = fp16_mul(pb, s);
        pr = fp16_mul(pr, s);
    }
    else if (yuv_mode!=2) {
        pb = fp16_mul(pb, s) + int2fp(pb_displacement)/100;
        pr = fp16_mul(pr, s) + int2fp(pr_displacement)/100;
    }
    else {
        pb = fp16_mul(pb, s);
        pr = fp16_mul(pr, s);
    }

    // YPbPr to rgb according to Wikipedia
    if (yuv_mode==0) {
        r = y + fp16_mul(float2fp(1.140), pr);
        b = y + fp16_mul(float2fp(2.028), pb);
        g = y - fp16_mul(float2fp(0.394), pb) - fp16_mul(float2fp(0.581), pr);
    }
    else if (yuv_mode==1) {
        r = y + fp16_mul(float2fp(1.000), pr);
        b = y + fp16_mul(float2fp(1.000), pb);
        g = y - fp16_mul(float2fp(0.5), pb) - fp16_mul(float2fp(0.5), pr);
    }
    else if (yuv_mode==2) {
        r = y + fp16_mul(float2fp(1.403), pr);
        b = y + fp16_mul(float2fp(1.770), pb);
        g = y - fp16_mul(float2fp(0.344), pb) - fp16_mul(float2fp(0.714), pr);
    }
#if 1
    else if (yuv_mode==3) {
        r = y + fp16_mul(float2fp(0.600), pr);
        b = y + fp16_mul(float2fp(3.000), pb);
        g = y - fp16_mul(float2fp(0.40), pb) - fp16_mul(float2fp(0.60), pr);
    }
#else
    else if (yuv_mode==3) { // looks exactly like standard mode (0)
        fixed i = fp16_cos((480-h/10)%360);
        fixed q = fp16_sin((480-h/10)%360);
        i = fp16_mul(i, s);
        q = fp16_mul(q, s);
        r = y + fp16_mul(float2fp(0.947), i) + fp16_mul(float2fp(0.624), q);
        g = y - fp16_mul(float2fp(0.275), i) - fp16_mul(float2fp(0.636), q);
        b = y - fp16_mul(float2fp(1.1), i) + fp16_mul(float2fp(1.7), q);
    }
#endif
    else if (yuv_mode==4) { // YUV
        r = y + fp16_mul(float2fp(1.140), pr);
        b = y + fp16_mul(float2fp(2.028), pb);
        g = y - fp16_mul(float2fp(0.394), pb) - fp16_mul(float2fp(0.581), pr);
    }
    else if (yuv_mode==5) {
        g = y - fp16_div(pb_displacement*pb, int2fp(100))
              - fp16_div(pr_displacement*pr, int2fp(100));
        r = y + fp16_div(pr, (fp16_div(int2fp(pr_displacement), int2fp(100))));
        b = y + fp16_div(pb, (fp16_div(int2fp(pb_displacement), int2fp(100))));
    }
    else {
        r = g = b = 0; /* silence warning */
    }

    *rr = fp2int(256 * r);
    *rr = CLAMP8(*rr);
    *gg = fp2int(256 * g);
    *gg = CLAMP8(*gg);
    *bb = fp2int(256 * b);
    *bb = CLAMP8(*bb);
}


/* PAL colour mapping */
static int8_t ntsc2pal[] = {
    0, 2, 4, 6, 8, 10, 12, 13, 11, 9, 7, 5, 3, 1, 14, 15
};


/**
 * Change palette type and create palette according to struct palette data
 * @param   type    new palette type (TV_NTSC/TV_PAL/TV_SECAM). If type is
 *                  set to TV_UNCHANGED the type is kept unchanged.
 */
void setup_palette(const enum tv_system type) COLD_ATTR;
void setup_palette(const enum tv_system type)
{
    if (type != TV_UNCHANGED)
        palette.type = type;

    fixed s = int2fp(palette.saturation)/100;
    int r, g, b;

    fixed min_l = int2fp(0) + int2fp(palette.brightness) / 100;
    fixed max_l = int2fp(1) + int2fp(palette.contrast) / 100;

    if (palette.type!=TV_SECAM) {
        int h; // 0..3600 (10th degree)
        h = 1800 + palette.c_shift*10/2; // we start at ~yellow
        // h-step in 10th degree
        int h_step = -3600 / 13 + palette.col_step;
        fixed l_step = (max_l - min_l) / 7;
        fixed l;
        for (int i=0; i<16; ++i) {
            bool grey;
            int ii;
            l = min_l;
            /* calculate colour row index according to palette */
            if (palette.type==TV_NTSC) {
                ii = i;
                grey = (ii==0);
            }
            else { /* TV_PAL */
                ii = ntsc2pal[i];
                grey = (ii<2 || ii>=14);
            }
#if NUMCOLS==256
            ii *= 16;
            for (int j = 0; j < 16; j+=2) {
#else
            ii *= 8;
            for (int j = 0; j < 8; ++j) {
#endif
                if (grey) {
                    // special case grey: palette.saturation=0
                    hsl2rgb(h, int2fp(0), l, &r, &g, &b);
                }
                else {
                    hsl2rgb(h, s, l, &r, &g, &b);
                }
#if defined(HAVE_LCD_COLOR)
                vcs_palette[ii+j] = FB_RGBPACK(r, g, b);
#else /* greyscale/mono */
                int tmp;
                tmp = (RGB2Y(r, g, b)*ARRAY_LEN(dither)) >> 8;
                vcs_palette[ii+j] = dither[tmp];
#endif
#if NUMCOLS==256
                vcs_palette[ii+j+1] = vcs_palette[ii+j];
#endif
                l += l_step;
            }
            h += h_step;
            if (h >= 3600) h -= 3600;
            if (h < 0)    h += 3600;
        }
    }
    else { /* TV_SECAM */
#if NUMCOLS==256
        for (int j = 0; j < 16; j+=2) {
#else
        for (int j = 0; j < 8; ++j) {
#endif
            int jj = j;
#if NUMCOLS==256
            jj = j/2;
#endif
            if (jj==0) {
                r = g = b = min_l;
            }
            else if (jj==7) {
                r = g = b = max_l;
            }
            else {
                b= jj & 0x1 ? float2fp(0.5)+s/2 : float2fp(0.5)-s/2;
                r= jj & 0x2 ? float2fp(0.5)+s/2 : float2fp(0.5)-s/2;
                g= jj & 0x4 ? float2fp(0.5)+s/2 : float2fp(0.5)-s/2;
                // TODO: change lightness of colours if min_l or max_l
                // gets too dark/bright
            }
            r = fp2int(r*256);
            r = CLAMP8(r);
            g = fp2int(g*256);
            g = CLAMP8(g);
            b = fp2int(b*256);
            b = CLAMP8(b);
            for (int i=0; i<16; ++i) {
#if defined(HAVE_LCD_COLOR)
                vcs_palette[i*(NUMCOLS/16)+j] = FB_RGBPACK(r, g, b);
#else /* greyscale/mono */
                int tmp = (RGB2Y(r, g, b)*ARRAY_LEN(dither)) >> 8;
                vcs_palette[i*(NUMCOLS/16)+j] = dither[tmp];
#endif
#if NUMCOLS==256
                vcs_palette[i*(NUMCOLS/16)+j+1] = vcs_palette[i*(NUMCOLS/16)+j];
#endif
            }
        }
    }
}



/*
 * Change/set/output parameters. Parameters are organized in groups of 2 parameters
 * (called "horizontal" and "vertical" parameter because they are changed using
 * right/left or up/down buttons).
 * s: parameter set: -1=reset to set#0 (first set); 0=keep, 1=increase.
 *    NOTE: if s is != 0 then h and v should not be used (set to 0)!
 * h: "horizontal" parameter: +1, 0 or -1 (increase/keep/decrease)
 * v: "vertical" parameter: +1, 0 or -1 (increase/keep/decrease)
 * return: string pointer to parameter value and description and value
 */
char *palette_chg_parameter(const int s, const int h, const int v) COLD_ATTR;
char *palette_chg_parameter(const int s, const int h, const int v)
{
    static int set;
    static char output[40];

    if (s<0) {
        set = 0;
    }
    else if (s>0) {
        ++set;
        if (set>3) set = 0;
    }
    switch (set) {
        case 0:
            if (h) {
                palette.contrast += h;
            }
            if (v) {
                palette.brightness += v;
            }
            rb->snprintf(output, sizeof(output),
                "Contrast=%d, Brightness=%d", palette.contrast, palette.brightness);
            break;
        case 1:
            if (h) {
                palette.yuv_mode += h;
            }
            if (v) {
                palette.saturation += v;
            }
            rb->snprintf(output, sizeof(output),
                "yuv-mode=%d, Satur=%d", palette.yuv_mode, palette.saturation);
            break;
        case 2:
            if (h) {
                palette.pb_displacement += h;
            }
            if (v) {
                palette.pr_displacement += v;
            }
            rb->snprintf(output, sizeof(output),
                "pr_displacement=%d, pb=%d", palette.pr_displacement, palette.pb_displacement);
            break;
        case 3:
            if (h) {
                palette.c_shift += h;
            }
            if (v) {
                palette.col_step += v;
            }
            rb->snprintf(output, sizeof(output),
                "C-shift=%d, col_step=%d", palette.c_shift, palette.col_step);
            break;
    }
    return output;
}

enum tv_system get_palette_type(void)
{
    return palette.type;
}
