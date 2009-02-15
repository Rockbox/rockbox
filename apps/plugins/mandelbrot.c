/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Matthias Wientapper
 * Heavily extended 2005 Jens Arnold
 *
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
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "lib/grey.h"
#include "lib/xlcd.h"

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_PLAY
#define MANDELBROT_ZOOM_OUT BUTTON_ON
#define MANDELBROT_MAXITER_INC BUTTON_F2
#define MANDELBROT_MAXITER_DEC BUTTON_F1
#define MANDELBROT_RESET BUTTON_F3

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT BUTTON_ON
#define MANDELBROT_MAXITER_INC BUTTON_F2
#define MANDELBROT_MAXITER_DEC BUTTON_F1
#define MANDELBROT_RESET BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE BUTTON_MENU
#define MANDELBROT_ZOOM_IN (BUTTON_MENU | BUTTON_REL)
#define MANDELBROT_ZOOM_IN2 (BUTTON_MENU | BUTTON_UP)
#define MANDELBROT_ZOOM_OUT (BUTTON_MENU | BUTTON_DOWN)
#define MANDELBROT_MAXITER_INC (BUTTON_MENU | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_MENU | BUTTON_LEFT)
#define MANDELBROT_RESET (BUTTON_MENU | BUTTON_OFF)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT BUTTON_MODE
#define MANDELBROT_MAXITER_INC (BUTTON_ON | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_ON | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_REC

#define MANDELBROT_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MANDELBROT_QUIT (BUTTON_SELECT | BUTTON_MENU)
#define MANDELBROT_UP BUTTON_MENU
#define MANDELBROT_DOWN BUTTON_PLAY
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_SCROLL_FWD
#define MANDELBROT_ZOOM_OUT BUTTON_SCROLL_BACK
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET (BUTTON_SELECT | BUTTON_PLAY)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MANDELBROT_QUIT            BUTTON_POWER
#define MANDELBROT_UP              BUTTON_UP
#define MANDELBROT_DOWN            BUTTON_DOWN
#define MANDELBROT_LEFT            BUTTON_LEFT
#define MANDELBROT_RIGHT           BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE     BUTTON_SELECT
#define MANDELBROT_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE    BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC_PRE BUTTON_PLAY
#define MANDELBROT_MAXITER_INC     (BUTTON_PLAY | BUTTON_REL)
#define MANDELBROT_MAXITER_DEC_PRE BUTTON_PLAY
#define MANDELBROT_MAXITER_DEC     (BUTTON_PLAY | BUTTON_REPEAT)
#define MANDELBROT_RESET           BUTTON_REC

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MANDELBROT_QUIT         BUTTON_POWER
#define MANDELBROT_UP           BUTTON_UP
#define MANDELBROT_DOWN         BUTTON_DOWN
#define MANDELBROT_LEFT         BUTTON_LEFT
#define MANDELBROT_RIGHT        BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE  BUTTON_SELECT
#define MANDELBROT_ZOOM_IN      (BUTTON_SELECT | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT     (BUTTON_SELECT | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC  BUTTON_VOL_UP
#define MANDELBROT_MAXITER_DEC  BUTTON_VOL_DOWN
#define MANDELBROT_RESET        BUTTON_A

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MANDELBROT_QUIT BUTTON_POWER
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_SCROLL_FWD
#define MANDELBROT_ZOOM_OUT BUTTON_SCROLL_BACK
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_REC

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define MANDELBROT_QUIT         BUTTON_POWER
#define MANDELBROT_UP           BUTTON_UP
#define MANDELBROT_DOWN         BUTTON_DOWN
#define MANDELBROT_LEFT         BUTTON_LEFT
#define MANDELBROT_RIGHT        BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN      BUTTON_SCROLL_FWD
#define MANDELBROT_ZOOM_OUT     BUTTON_SCROLL_BACK
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET       (BUTTON_SELECT | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define MANDELBROT_QUIT BUTTON_POWER
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_VOL_UP
#define MANDELBROT_ZOOM_OUT BUTTON_VOL_DOWN
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_REC

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define MANDELBROT_QUIT BUTTON_POWER
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_VOL_UP
#define MANDELBROT_ZOOM_OUT BUTTON_VOL_DOWN
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_HOME

#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define MANDELBROT_QUIT BUTTON_POWER
#define MANDELBROT_UP BUTTON_UP
#define MANDELBROT_DOWN BUTTON_DOWN
#define MANDELBROT_LEFT BUTTON_LEFT
#define MANDELBROT_RIGHT BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN BUTTON_VOL_UP
#define MANDELBROT_ZOOM_OUT BUTTON_VOL_DOWN
#define MANDELBROT_MAXITER_INC (BUTTON_SELECT | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_SELECT | BUTTON_LEFT)
#define MANDELBROT_RESET (BUTTON_SELECT | BUTTON_UP)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MANDELBROT_QUIT          BUTTON_POWER
#define MANDELBROT_UP            BUTTON_SCROLL_UP
#define MANDELBROT_DOWN          BUTTON_SCROLL_DOWN
#define MANDELBROT_LEFT          BUTTON_LEFT
#define MANDELBROT_RIGHT         BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE   BUTTON_PLAY
#define MANDELBROT_ZOOM_IN       (BUTTON_PLAY | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE  BUTTON_PLAY
#define MANDELBROT_ZOOM_OUT      (BUTTON_PLAY | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC   BUTTON_FF
#define MANDELBROT_MAXITER_DEC   BUTTON_REW
#define MANDELBROT_RESET         (BUTTON_PLAY | BUTTON_REW)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define MANDELBROT_QUIT          BUTTON_EQ
#define MANDELBROT_UP            BUTTON_UP
#define MANDELBROT_DOWN          BUTTON_DOWN
#define MANDELBROT_LEFT          BUTTON_LEFT
#define MANDELBROT_RIGHT         BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE   BUTTON_SELECT
#define MANDELBROT_ZOOM_IN       (BUTTON_SELECT | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE  BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT      (BUTTON_SELECT | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC   (BUTTON_PLAY | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC   (BUTTON_PLAY | BUTTON_LEFT)
#define MANDELBROT_RESET         BUTTON_MODE

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define MANDELBROT_QUIT          BUTTON_BACK
#define MANDELBROT_UP            BUTTON_UP
#define MANDELBROT_DOWN          BUTTON_DOWN
#define MANDELBROT_LEFT          BUTTON_LEFT
#define MANDELBROT_RIGHT         BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN       BUTTON_VOL_UP
#define MANDELBROT_ZOOM_OUT      BUTTON_VOL_DOWN
#define MANDELBROT_MAXITER_INC   BUTTON_PREV
#define MANDELBROT_MAXITER_DEC   BUTTON_NEXT
#define MANDELBROT_RESET         BUTTON_MENU

#elif CONFIG_KEYPAD == MROBE100_PAD
#define MANDELBROT_QUIT         BUTTON_POWER
#define MANDELBROT_UP           BUTTON_UP
#define MANDELBROT_DOWN         BUTTON_DOWN
#define MANDELBROT_LEFT         BUTTON_LEFT
#define MANDELBROT_RIGHT        BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN_PRE  BUTTON_SELECT
#define MANDELBROT_ZOOM_IN      (BUTTON_SELECT | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT     (BUTTON_SELECT | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC  BUTTON_MENU
#define MANDELBROT_MAXITER_DEC  BUTTON_PLAY
#define MANDELBROT_RESET        BUTTON_DISPLAY

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MANDELBROT_QUIT            BUTTON_RC_REC
#define MANDELBROT_UP              BUTTON_RC_VOL_UP
#define MANDELBROT_DOWN            BUTTON_RC_VOL_DOWN
#define MANDELBROT_LEFT            BUTTON_RC_REW
#define MANDELBROT_RIGHT           BUTTON_RC_FF
#define MANDELBROT_ZOOM_IN_PRE     BUTTON_RC_PLAY
#define MANDELBROT_ZOOM_IN         (BUTTON_RC_PLAY | BUTTON_REL)
#define MANDELBROT_ZOOM_OUT_PRE    BUTTON_RC_PLAY
#define MANDELBROT_ZOOM_OUT        (BUTTON_RC_PLAY | BUTTON_REPEAT)
#define MANDELBROT_MAXITER_INC_PRE BUTTON_RC_MODE
#define MANDELBROT_MAXITER_INC     (BUTTON_RC_MODE|BUTTON_REL)
#define MANDELBROT_MAXITER_DEC_PRE BUTTON_RC_MODE
#define MANDELBROT_MAXITER_DEC     (BUTTON_RC_MODE|BUTTON_REPEAT)
#define MANDELBROT_RESET           BUTTON_RC_MENU

#elif CONFIG_KEYPAD == COWOND2_PAD
#define MANDELBROT_QUIT          BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define MANDELBROT_QUIT          BUTTON_BACK
#define MANDELBROT_UP            BUTTON_UP
#define MANDELBROT_DOWN          BUTTON_DOWN
#define MANDELBROT_LEFT          BUTTON_LEFT
#define MANDELBROT_RIGHT         BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN       BUTTON_PLAY
#define MANDELBROT_ZOOM_OUT      BUTTON_MENU
#define MANDELBROT_MAXITER_INC   (BUTTON_UP | BUTTON_CUSTOM)
#define MANDELBROT_MAXITER_DEC   (BUTTON_DOWN | BUTTON_CUSTOM)
#define MANDELBROT_RESET         BUTTON_SELECT

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define MANDELBROT_QUIT         BUTTON_POWER
#define MANDELBROT_UP           BUTTON_UP
#define MANDELBROT_DOWN         BUTTON_DOWN
#define MANDELBROT_LEFT         BUTTON_LEFT
#define MANDELBROT_RIGHT        BUTTON_RIGHT
#define MANDELBROT_ZOOM_IN      BUTTON_VIEW
#define MANDELBROT_ZOOM_OUT     BUTTON_PLAYLIST
#define MANDELBROT_MAXITER_INC  BUTTON_VOL_UP
#define MANDELBROT_MAXITER_DEC  BUTTON_VOL_DOWN
#define MANDELBROT_RESET        BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MANDELBROT_UP
#define MANDELBROT_UP            BUTTON_TOPMIDDLE
#endif
#ifndef MANDELBROT_DOWN
#define MANDELBROT_DOWN          BUTTON_BOTTOMMIDDLE
#endif
#ifndef MANDELBROT_LEFT
#define MANDELBROT_LEFT          BUTTON_MIDLEFT
#endif
#ifndef MANDELBROT_RIGHT
#define MANDELBROT_RIGHT         BUTTON_MIDRIGHT
#endif
#ifndef MANDELBROT_ZOOM_IN_PRE
#define MANDELBROT_ZOOM_IN_PRE   BUTTON_TOPRIGHT
#endif
#ifndef MANDELBROT_ZOOM_IN
#define MANDELBROT_ZOOM_IN      (BUTTON_TOPRIGHT | BUTTON_REL)
#endif
#ifndef MANDELBROT_ZOOM_OUT_PRE
#define MANDELBROT_ZOOM_OUT_PRE  BUTTON_TOPLEFT
#endif
#ifndef MANDELBROT_ZOOM_OUT
#define MANDELBROT_ZOOM_OUT     (BUTTON_TOPLEFT | BUTTON_REL)
#endif
#ifndef MANDELBROT_MAXITER_INC
#define MANDELBROT_MAXITER_INC   BUTTON_BOTTOMRIGHT
#endif
#ifndef MANDELBROT_MAXITER_DEC
#define MANDELBROT_MAXITER_DEC   BUTTON_BOTTOMLEFT
#endif
#ifndef MANDELBROT_RESET
#define MANDELBROT_RESET         BUTTON_CENTER
#endif
#endif

#if LCD_DEPTH < 8
#define USEGSLIB
#define MYLCD(fn) grey_ub_ ## fn
#define MYLCD_UPDATE()
#define MYXLCD(fn) grey_ub_ ## fn
#else
#define UPDATE_FREQ (HZ/50)
#define MYLCD(fn) rb->lcd_ ## fn
#define MYLCD_UPDATE() rb->lcd_update();
#define MYXLCD(fn) xlcd_ ## fn
#endif

/* Fixed point format s5.26: sign, 5 bits integer part, 26 bits fractional part */
static long x_min;
static long x_max;
static long x_step;
static long x_delta;
static long y_min;
static long y_max;
static long y_step;
static long y_delta;

static int px_min = 0;
static int px_max = LCD_WIDTH;
static int py_min = 0;
static int py_max = LCD_HEIGHT;

static int step_log2;
static unsigned max_iter;

#ifdef USEGSLIB
GREY_INFO_STRUCT
static unsigned char *gbuf;
static size_t  gbuf_size = 0;
static unsigned char imgbuffer[LCD_HEIGHT];
#else
static fb_data imgbuffer[LCD_HEIGHT];
#endif

/* 8 entries cyclical, last entry is black (convergence) */
#ifdef HAVE_LCD_COLOR
static const fb_data color[9] = {
    LCD_RGBPACK(255, 0, 159), LCD_RGBPACK(159, 0, 255), LCD_RGBPACK(0, 0, 255),
    LCD_RGBPACK(0, 159, 255), LCD_RGBPACK(0, 255, 128), LCD_RGBPACK(128, 255, 0),
    LCD_RGBPACK(255, 191, 0), LCD_RGBPACK(255, 0, 0),   LCD_RGBPACK(0, 0, 0)
};
#else /* greyscale */
static const unsigned char color[9] = {
    255, 223, 191, 159, 128, 96, 64, 32, 0
};
#endif

#if CONFIG_CPU == SH7034

#define MULS16_ASR10(a, b) muls16_asr10(a, b)
static inline short muls16_asr10(short a, short b)
{
    short r;
    asm (
        "muls    %[a],%[b]   \n"
        "sts     macl,%[r]   \n"
        "shlr8   %[r]        \n"
        "shlr2   %[r]        \n"
        : /* outputs */
        [r]"=r"(r)
        : /* inputs */
        [a]"r"(a),
        [b]"r"(b)
    );
    return r;
}

#define MULS32_ASR26(a, b) muls32_asr26(a, b)
static inline long muls32_asr26(long a, long b)
{
    long r, t1, t2, t3;
    asm (
        /* Signed 32bit * 32bit -> 64bit multiplication.
           Notation: xxab * xxcd, where each letter represents 16 bits.
           xx is the 64 bit sign extension.  */
        "swap.w  %[a],%[t1]  \n" /* t1 = ba */
        "mulu    %[t1],%[b]  \n" /* a * d */
        "swap.w  %[b],%[t3]  \n" /* t3 = dc */
        "sts     macl,%[t2]  \n" /* t2 = a * d */
        "mulu    %[t1],%[t3] \n" /* a * c */
        "sts     macl,%[r]   \n" /* hi = a * c */
        "mulu    %[a],%[t3]  \n" /* b * c */
        "clrt                \n"
        "sts     macl,%[t3]  \n" /* t3 = b * c */
        "addc    %[t2],%[t3] \n" /* t3 += t2, carry -> t2 */
        "movt    %[t2]       \n"
        "mulu    %[a],%[b]   \n" /* b * d */
        "mov     %[t3],%[t1] \n" /* t1t3 = t2t3 << 16 */
        "xtrct   %[t2],%[t1] \n"
        "shll16  %[t3]       \n"
        "sts     macl,%[t2]  \n" /* lo = b * d */
        "clrt                \n" /* hi.lo += t1t3 */
        "addc    %[t3],%[t2] \n"
        "addc    %[t1],%[r]  \n"
        "cmp/pz  %[a]        \n" /* ab >= 0 ? */
        "bt      1f          \n"
        "sub     %[b],%[r]   \n" /* no: hi -= cd (sign extension of ab is -1) */
    "1:                      \n"
        "cmp/pz  %[b]        \n" /* cd >= 0 ? */
        "bt      2f          \n"
        "sub     %[a],%[r]   \n" /* no: hi -= ab (sign extension of cd is -1) */
    "2:                      \n"
        /* Shift right by 26 and return low 32 bits */
        "shll2   %[r]        \n" /* hi <<= 6 */
        "shll2   %[r]        \n"
        "shll2   %[r]        \n"
        "shlr16  %[t2]       \n" /* (unsigned)lo >>= 26 */
        "shlr8   %[t2]       \n"
        "shlr2   %[t2]       \n"
        "or      %[t2],%[r]  \n" /* combine result */
        : /* outputs */
        [r] "=&r"(r),
        [t1]"=&r"(t1),
        [t2]"=&r"(t2),
        [t3]"=&r"(t3)
        : /* inputs */
        [a] "r"  (a),
        [b] "r"  (b)
    );
    return r;
}

#elif defined CPU_COLDFIRE

#define MULS16_ASR10(a, b) muls16_asr10(a, b)
static inline short muls16_asr10(short a, short b)
{
    asm (
        "muls.w  %[a],%[b]   \n"
        "asr.l   #8,%[b]     \n"
        "asr.l   #2,%[b]     \n"
        : /* outputs */
        [b]"+d"(b)
        : /* inputs */
        [a]"d" (a)
    );
    return b;
}

/* Needs the EMAC initialised to fractional mode w/o rounding and saturation */
#define MULS32_INIT() coldfire_set_macsr(EMAC_FRACTIONAL)
#define MULS32_ASR26(a, b) muls32_asr26(a, b)
static inline long muls32_asr26(long a, long b)
{
    long r, t1;
    asm (
        "mac.l   %[a], %[b], %%acc0  \n" /* multiply */
        "move.l  %%accext01, %[t1]   \n" /* get low part */
        "movclr.l %%acc0, %[r]       \n" /* get high part */
        "asl.l   #5, %[r]            \n" /* hi <<= 5, plus one free */
        "lsr.l   #3, %[t1]           \n" /* lo >>= 3 */
        "and.l   #0x1f, %[t1]        \n" /* mask out unrelated bits */
        "or.l    %[t1], %[r]         \n" /* combine result */
        : /* outputs */
        [r] "=d"(r),
        [t1]"=d"(t1)
        : /* inputs */
        [a] "d" (a),
        [b] "d" (b)
    );
    return r;
}

#elif defined CPU_ARM

#define MULS32_ASR26(a, b) muls32_asr26(a, b)
static inline long muls32_asr26(long a, long b)
{
    long r, t1;
    asm (
        "smull   %[r], %[t1], %[a], %[b]     \n"
        "mov     %[r], %[r], lsr #26         \n"
        "orr     %[r], %[r], %[t1], lsl #6   \n"
        : /* outputs */
        [r] "=&r,&r,&r"(r),
        [t1]"=&r,&r,&r"(t1)
        : /* inputs */
        [a] "%r,%r,%r" (a),
        [b] "r,0,1" (b)
    );
    return r;
}

#endif /* CPU */

/* default macros */
#ifndef MULS16_ASR10
#define MULS16_ASR10(a, b) ((short)(((long)(a) * (long)(b)) >> 10))
#endif
#ifndef MULS32_ASR26
#define MULS32_ASR26(a, b) ((long)(((long long)(a) * (long long)(b)) >> 26))
#endif
#ifndef MULS32_INIT
#define MULS32_INIT()
#endif

int ilog2_fp(long value) /* calculate integer log2(value_fp_6.26) */
{
    int i = 0;

    if (value <= 0) {
        return -32767;
    } else if (value > (1L<<26)) {
        while (value >= (2L<<26)) {
            value >>= 1;
            i++;
        }
    } else {
        while (value < (1L<<26)) {
            value <<= 1;
            i--;
        }
    }
    return i;
}

void recalc_parameters(void)
{
    x_step = (x_max - x_min) / LCD_WIDTH;
    x_delta = (x_step * LCD_WIDTH) / 8;
    y_step = (y_max - y_min) / LCD_HEIGHT;
    y_delta = (y_step * LCD_HEIGHT) / 8;
    step_log2 = ilog2_fp(MIN(x_step, y_step));
    max_iter = MAX(15, -15 * step_log2 - 45);
}

#if CONFIG_LCD == LCD_SSD1815 
/* Recorder, Ondio: pixel_height == 1.25 * pixel_width */
#define MB_HEIGHT (LCD_HEIGHT*5/4)
#else
/* square pixels */
#define MB_HEIGHT LCD_HEIGHT
#endif

#define MB_XOFS (-0x03000000L)       /* -0.75 (s5.26) */
#if 3000*MB_HEIGHT/LCD_WIDTH >= 2400 /* width is limiting factor */
#define MB_XFAC (0x06000000LL)       /* 1.5 (s5.26) */
#define MB_YFAC (MB_XFAC*MB_HEIGHT/LCD_WIDTH)
#else                                /* height is limiting factor */
#define MB_YFAC (0x04cccccdLL)       /* 1.2 (s5.26) */
#define MB_XFAC (MB_YFAC*LCD_WIDTH/MB_HEIGHT)
#endif

void init_mandelbrot_set(void)
{
    x_min = MB_XOFS-MB_XFAC;
    x_max = MB_XOFS+MB_XFAC;
    y_min = -MB_YFAC;
    y_max = MB_YFAC;
    recalc_parameters();
}

void calc_mandelbrot_low_prec(void)
{
    long start_tick, last_yield;
#ifndef USEGSLIB
    long next_update = *rb->current_tick;
    int last_px = px_min;
#endif
    unsigned n_iter;
    long a32, b32;
    short x, x2, y, y2, a, b;
    int p_x, p_y;

    start_tick = last_yield = *rb->current_tick;
    
    for (p_x = 0, a32 = x_min; p_x < px_max; p_x++, a32 += x_step) {
        if (p_x < px_min)
            continue;
        a = a32 >> 16;
        for (p_y = LCD_HEIGHT-1, b32 = y_min; p_y >= py_min; p_y--, b32 += y_step) {
            if (p_y >= py_max)
                continue;
            b = b32 >> 16;
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= max_iter) {
                x2 = MULS16_ASR10(x, x);
                y2 = MULS16_ASR10(y, y);

                if (x2 + y2 > (4<<10)) break;

                y = 2 * MULS16_ASR10(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > max_iter)
                imgbuffer[p_y] = color[8];
            else
                imgbuffer[p_y] = color[n_iter & 7];

            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield) {
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
#ifdef USEGSLIB
        grey_ub_gray_bitmap_part(imgbuffer, 0, py_min, 1,
                                 p_x, py_min, 1, py_max - py_min);
#else
        rb->lcd_bitmap_part(imgbuffer, 0, py_min, 1,
                            p_x, py_min, 1, py_max - py_min);
        if ((p_x == px_max - 1) || TIME_AFTER(*rb->current_tick, next_update))
        {
            next_update = *rb->current_tick + UPDATE_FREQ;
            rb->lcd_update_rect(last_px, py_min, p_x - last_px + 1,
                                py_max - py_min);
            last_px = p_x;
        }
#endif
    }
}

void calc_mandelbrot_high_prec(void)
{
    long start_tick, last_yield;
#ifndef USEGSLIB
    long next_update = *rb->current_tick;
    int last_px = px_min;
#endif
    unsigned n_iter;
    long x, x2, y, y2, a, b;
    int p_x, p_y;

    MULS32_INIT();
    start_tick = last_yield = *rb->current_tick;

    for (p_x = 0, a = x_min; p_x < px_max; p_x++, a += x_step) {
        if (p_x < px_min)
            continue;
        for (p_y = LCD_HEIGHT-1, b = y_min; p_y >= py_min; p_y--, b += y_step) {
            if (p_y >= py_max)
                continue;
            x = a;
            y = b;
            n_iter = 0;

            while (++n_iter <= max_iter) {
                x2 = MULS32_ASR26(x, x);
                y2 = MULS32_ASR26(y, y);

                if (x2 + y2 > (4L<<26)) break;

                y = 2 * MULS32_ASR26(x, y) + b;
                x = x2 - y2 + a;
            }

            if  (n_iter > max_iter)
                imgbuffer[p_y] = color[8];
            else
                imgbuffer[p_y] = color[n_iter & 7];

            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield) {
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
#ifdef USEGSLIB
        grey_ub_gray_bitmap_part(imgbuffer, 0, py_min, 1,
                                 p_x, py_min, 1, py_max - py_min);
#else
        rb->lcd_bitmap_part(imgbuffer, 0, py_min, 1,
                            p_x, py_min, 1, py_max-py_min);
        if ((p_x == px_max - 1) || TIME_AFTER(*rb->current_tick, next_update))
        {
            next_update = *rb->current_tick + UPDATE_FREQ;
            rb->lcd_update_rect(last_px, py_min, p_x - last_px + 1,
                                py_max - py_min);
            last_px = p_x;
        }
#endif
    }
}

void cleanup(void *parameter)
{
    (void)parameter;
#ifdef USEGSLIB
    grey_release();
#endif
}

#define REDRAW_NONE    0
#define REDRAW_PARTIAL 1
#define REDRAW_FULL    2

enum plugin_status plugin_start(const void* parameter)
{
    int button;
    int lastbutton = BUTTON_NONE;
    int redraw = REDRAW_FULL;

    (void)parameter;

#ifdef USEGSLIB
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the greyscale buffer.*/
    if (!grey_init(gbuf, gbuf_size, GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return 0;
    }
    grey_show(true); /* switch on greyscale overlay */
#endif

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    init_mandelbrot_set();

    /* main loop */
    while (true) {
        if (redraw > REDRAW_NONE) {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            if (redraw == REDRAW_FULL) {
                MYLCD(clear_display)();
                MYLCD_UPDATE();
            }

            if (step_log2 <= -10) /* select precision */
                calc_mandelbrot_high_prec();
            else
                calc_mandelbrot_low_prec();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            px_min = 0;
            px_max = LCD_WIDTH;
            py_min = 0;
            py_max = LCD_HEIGHT;
            redraw = REDRAW_NONE;
        }

        button = rb->button_get(true);
        switch (button) {
#ifdef MANDELBROT_RC_QUIT
        case MANDELBROT_RC_QUIT:
#endif
        case MANDELBROT_QUIT:
#ifdef USEGSLIB
            grey_release();
#endif
            return PLUGIN_OK;

        case MANDELBROT_ZOOM_OUT:
#ifdef MANDELBROT_ZOOM_OUT_PRE
            if (lastbutton != MANDELBROT_ZOOM_OUT_PRE)
                break;
#endif
            x_min -= x_delta;
            x_max += x_delta;
            y_min -= y_delta;
            y_max += y_delta;
            recalc_parameters();
            redraw = REDRAW_FULL;
            break;


        case MANDELBROT_ZOOM_IN:
#ifdef MANDELBROT_ZOOM_IN_PRE
            if (lastbutton != MANDELBROT_ZOOM_IN_PRE)
                break;
#endif
#ifdef MANDELBROT_ZOOM_IN2
        case MANDELBROT_ZOOM_IN2:
#endif
            x_min += x_delta;
            x_max -= x_delta;
            y_min += y_delta;
            y_max -= y_delta;
            recalc_parameters();
            redraw = REDRAW_FULL;
            break;

        case MANDELBROT_UP:
            y_min += y_delta;
            y_max += y_delta;
            MYXLCD(scroll_down)(LCD_HEIGHT/8);
            MYLCD_UPDATE();
            py_max = (LCD_HEIGHT/8);
            redraw = REDRAW_PARTIAL;
            break;

        case MANDELBROT_DOWN:
            y_min -= y_delta;
            y_max -= y_delta;
            MYXLCD(scroll_up)(LCD_HEIGHT/8);
            MYLCD_UPDATE();
            py_min = (LCD_HEIGHT-LCD_HEIGHT/8);
            redraw = REDRAW_PARTIAL;
            break;

        case MANDELBROT_LEFT:
            x_min -= x_delta;
            x_max -= x_delta;
            MYXLCD(scroll_right)(LCD_WIDTH/8);
            MYLCD_UPDATE();
            px_max = (LCD_WIDTH/8);
            redraw = REDRAW_PARTIAL;
            break;

        case MANDELBROT_RIGHT:
            x_min += x_delta;
            x_max += x_delta;
            MYXLCD(scroll_left)(LCD_WIDTH/8);
            MYLCD_UPDATE();
            px_min = (LCD_WIDTH-LCD_WIDTH/8);
            redraw = REDRAW_PARTIAL;
            break;

        case MANDELBROT_MAXITER_DEC:
#ifdef MANDELBROT_MAXITER_DEC_PRE
            if (lastbutton != MANDELBROT_MAXITER_DEC_PRE)
                break;
#endif
            if (max_iter >= 15) {
                max_iter -= max_iter / 3;
                redraw = REDRAW_FULL;
            }
            break;

        case MANDELBROT_MAXITER_INC:
#ifdef MANDELBROT_MAXITER_INC_PRE
            if (lastbutton != MANDELBROT_MAXITER_INC_PRE)
                break;
#endif
            max_iter += max_iter / 2;
            redraw = REDRAW_FULL;
            break;

        case MANDELBROT_RESET:
            init_mandelbrot_set();
            redraw = REDRAW_FULL;
            break;

        default:
            if (rb->default_event_handler_ex(button, cleanup, NULL)
                == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
#ifdef USEGSLIB
    grey_release();
#endif
    return PLUGIN_OK;
}
#endif
