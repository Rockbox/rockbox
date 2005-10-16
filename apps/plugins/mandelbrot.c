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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SIMULATOR 
#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (LCD_DEPTH < 4)
#include "gray.h"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN BUTTON_PLAY
#define MANDELBROT_ZOOM_OUT BUTTON_ON
#define MANDELBROT_MAXITER_INC BUTTON_F2
#define MANDELBROT_MAXITER_DEC BUTTON_F1
#define MANDELBROT_RESET BUTTON_F3

#elif CONFIG_KEYPAD == ONDIO_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN_PRE BUTTON_MENU
#define MANDELBROT_ZOOM_IN (BUTTON_MENU | BUTTON_REL)
#define MANDELBROT_ZOOM_IN2 (BUTTON_MENU | BUTTON_UP)
#define MANDELBROT_ZOOM_OUT (BUTTON_MENU | BUTTON_DOWN)
#define MANDELBROT_MAXITER_INC (BUTTON_MENU | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_MENU | BUTTON_LEFT)
#define MANDELBROT_RESET (BUTTON_MENU | BUTTON_OFF)

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define MANDELBROT_QUIT BUTTON_OFF
#define MANDELBROT_ZOOM_IN BUTTON_SELECT
#define MANDELBROT_ZOOM_OUT BUTTON_MODE
#define MANDELBROT_MAXITER_INC (BUTTON_ON | BUTTON_RIGHT)
#define MANDELBROT_MAXITER_DEC (BUTTON_ON | BUTTON_LEFT)
#define MANDELBROT_RESET BUTTON_REC
#endif

static struct plugin_api* rb;
static char buff[32];

/* Fixed point format: 6 bits integer part incl. sign, 26 bits fractional part */
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

static unsigned char *gbuf;
static unsigned int gbuf_size = 0;
static unsigned char graybuffer[LCD_HEIGHT];   

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
        "mov     %[t3],%[t1] \n" /* t2t3 <<= 16 */
        "xtrct   %[t2],%[t1] \n"
        "mov     %[t1],%[t2] \n"
        "shll16  %[t3]       \n"
        "sts     macl,%[t1]  \n" /* lo = b * d */
        "clrt                \n" /* hi.lo += t2t3 */
        "addc    %[t3],%[t1] \n"
        "addc    %[t2],%[r]  \n"
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
        "shlr16  %[t1]       \n" /* (unsigned)lo >>= 26 */
        "shlr8   %[t1]       \n"
        "shlr2   %[t1]       \n"
        "or      %[t1],%[r]  \n" /* combine result */
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
        "mac.l   %[a],%[b],%%acc0\n" /* multiply */
        "mulu.l  %[a],%[b]       \n" /* get lower half */
        "movclr.l %%acc0,%[r]    \n" /* get higher half */
        "asl.l   #5,%[r]         \n" /* hi <<= 5, plus one free */
        "moveq.l #26,%[t1]       \n"
        "lsr.l   %[t1],%[b]      \n" /* (unsigned)lo >>= 26 */
        "or.l    %[b],%[r]       \n" /* combine result */
        : /* outputs */
        [r]"=&d"(r),
        [t1]"=&d"(t1),
        [b] "+d" (b)
        : /* inputs */
        [a] "d"  (a)
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
    x_delta = x_step * (LCD_WIDTH/8);
    y_step = (y_max - y_min) / LCD_HEIGHT;
    y_delta = y_step * (LCD_HEIGHT/8);
    step_log2 = ilog2_fp(MIN(x_step, y_step));
    max_iter = MAX(15, -15 * step_log2 - 45);
}

void init_mandelbrot_set(void)
{
#if CONFIG_LCD == LCD_SSD1815 /* Recorder, Ondio. */
    x_min = -38L<<22;  // -2.375<<26
    x_max =  15L<<22;  //  0.9375<<26
#else  /* Iriver H1x0 */
    x_min = -36L<<22;  // -2.25<<26
    x_max =  12L<<22;  //  0.75<<26
#endif
    y_min = -19L<<22;  // -1.1875<<26
    y_max =  19L<<22;  //  1.1875<<26
    recalc_parameters();
}

void calc_mandelbrot_low_prec(void)
{
    long start_tick, last_yield;
    unsigned n_iter;
    long a32, b32;
    short x, x2, y, y2, a, b;
    int p_x, p_y;
    int brightness;

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

            // "coloring"
            if  (n_iter > max_iter){
                brightness = 0; // black
            } else {
                brightness = 255 - (32 * (n_iter & 7));
            }
            graybuffer[p_y] = brightness;
            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield) {
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
        gray_ub_gray_bitmap_part(graybuffer, 0, py_min, 1,
                                 p_x, py_min, 1, py_max-py_min);
    }
}

void calc_mandelbrot_high_prec(void)
{
    long start_tick, last_yield;
    unsigned n_iter;
    long x, x2, y, y2, a, b;
    int p_x, p_y;
    int brightness;
    
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

            // "coloring"
            if  (n_iter > max_iter){
                brightness = 0; // black
            } else {
                brightness = 255 - (32 * (n_iter & 7));
            }
            graybuffer[p_y] = brightness;
            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield) {
                rb->yield();
                last_yield = *rb->current_tick;
            }
        }
        gray_ub_gray_bitmap_part(graybuffer, 0, py_min, 1,
                                 p_x, py_min, 1, py_max-py_min);
    }
}

void cleanup(void *parameter)
{
    (void)parameter;
    
    gray_release();
}

#define REDRAW_NONE    0
#define REDRAW_PARTIAL 1
#define REDRAW_FULL    2

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;
    int lastbutton = BUTTON_NONE;
    int grayscales;
    int redraw = REDRAW_FULL;

    TEST_PLUGIN_API(api);
    rb = api;
    (void)parameter;

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the grayscale buffer:
     * 8 bitplanes for 9 shades of gray.*/
    grayscales = gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH, LCD_HEIGHT/8,
                           8, NULL) + 1;
    if (grayscales != 9) {
        rb->snprintf(buff, sizeof(buff), "%d", grayscales);
        rb->lcd_puts(0, 1, buff);
        rb->lcd_update();
        rb->sleep(HZ*2);
        return(0);
    }

    gray_show(true); /* switch on grayscale overlay */

    init_mandelbrot_set();

    /* main loop */
    while (true) {
        if (redraw > REDRAW_NONE) {
#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(true);
#endif
            if (redraw == REDRAW_FULL)
                gray_ub_clear_display();

            if (step_log2 <= -10) /* select precision */
                calc_mandelbrot_high_prec();
            else
                calc_mandelbrot_low_prec();

#if !defined(SIMULATOR) && defined(HAVE_ADJUSTABLE_CPU_FREQ)
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
        case MANDELBROT_QUIT:
            gray_release();
            return PLUGIN_OK;

        case MANDELBROT_ZOOM_OUT:
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

        case BUTTON_UP:
            y_min += y_delta;
            y_max += y_delta;
            gray_ub_scroll_down(LCD_HEIGHT/8);
            py_max = (LCD_HEIGHT/8);
            redraw = REDRAW_PARTIAL;
            break;

        case BUTTON_DOWN:
            y_min -= y_delta;
            y_max -= y_delta;
            gray_ub_scroll_up(LCD_HEIGHT/8);
            py_min = (LCD_HEIGHT-LCD_HEIGHT/8);
            redraw = REDRAW_PARTIAL;
            break;

        case BUTTON_LEFT:
            x_min -= x_delta;
            x_max -= x_delta;
            gray_ub_scroll_right(LCD_WIDTH/8);
            px_max = (LCD_WIDTH/8);
            redraw = REDRAW_PARTIAL;
            break;

        case BUTTON_RIGHT:
            x_min += x_delta;
            x_max += x_delta;
            gray_ub_scroll_left(LCD_WIDTH/8);
            px_min = (LCD_WIDTH-LCD_WIDTH/8);
            redraw = REDRAW_PARTIAL;
            break;

        case MANDELBROT_MAXITER_DEC:
            if (max_iter >= 15) {
                max_iter -= max_iter / 3;
                redraw = REDRAW_FULL;
            }
            break;

        case MANDELBROT_MAXITER_INC:
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
    gray_release();
    return PLUGIN_OK;
}
#endif
#endif
