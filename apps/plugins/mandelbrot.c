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

/* Fixed point format: 6 bits integer part incl. sign, 58 bits fractional part */
static long long x_min;
static long long x_max;
static long long x_step;
static long long x_delta;
static long long y_min;
static long long y_max;
static long long y_step;
static long long y_delta;

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
long long mul64(long long f1, long long f2);

asm (
    /* 64bit * 64bit -> 64bit multiplication. Works for both signed and unsigned */
    ".global _mul64      \n"  /* Notation: abcd * efgh, where each letter */
    ".type   _mul64,@function\n" /* represents 16 bits. Called with: */
"_mul64:                 \n"  /* r4 = ab, r5 = cd, r6 = ef, r7 = gh */
    "swap.w  r4,r2       \n"  /* r2 = ba */
    "mulu    r2,r7       \n"  /* a * h */
    "swap.w  r6,r3       \n"  /* r3 = fe */
    "sts     macl,r0     \n"  /* r0 = a * h */
    "mulu    r5,r3       \n"  /* d * e */
    "swap.w  r7,r3       \n"  /* r3 = hg */
    "sts     macl,r1     \n"  /* r1 = d * e */
    "mulu    r4,r3       \n"  /* b * g */
    "add     r1,r0       \n"  /* r0 += r1 */
    "swap.w  r5,r2       \n"  /* r2 = dc */
    "sts     macl,r1     \n"  /* r1 = b * g */
    "mulu    r2,r6       \n"  /* c * f */
    "add     r1,r0       \n"  /* r0 += r1 */
    "sts     macl,r1     \n"  /* r1 = c * f */
    "mulu    r4,r7       \n"  /* b * h */
    "add     r1,r0       \n"  /* r0 += r1 */
    "shll16  r0          \n"  /* r0 <<= 16 */
    "sts     macl,r1     \n"  /* r1 = b * h */
    "mulu    r2,r3       \n"  /* c * g */
    "add     r1,r0       \n"  /* r0 += r1 */
    "sts     macl,r1     \n"  /* r1 = c * g */
    "mulu    r5,r6       \n"  /* d * f */
    "add     r1,r0       \n"  /* r0 += r1 */
    "sts     macl,r1     \n"  /* r1 = d * f */
    "mulu    r2,r7       \n"  /* c * h */
    "add     r1,r0       \n"  /* r0 += r1 */
    "sts     macl,r2     \n"  /* r2 = c * h */
    "mulu    r5,r3       \n"  /* d * g */
    "clrt                \n"
    "sts     macl,r3     \n"  /* r3 = d * g */
    "addc    r2,r3       \n"  /* r3 += r2, carry->r2 */
    "movt    r2          \n"
    "mulu    r5,r7       \n"  /* d * h */
    "mov     r3,r1       \n"  /* r2r3 <<= 16 */
    "xtrct   r2,r1       \n"
    "mov     r1,r2       \n"
    "shll16  r3          \n"
    "sts     macl,r1     \n"  /* r1 = d * h */
    "clrt                \n"  /* r0r1 += r2r3 */
    "addc    r3,r1       \n"
    "rts                 \n"
    "addc    r2,r0       \n"
);
#define MUL64(a, b) mul64(a, b)

#elif defined CPU_COLDFIRE
long long mul64(long long f1, long long f2);

asm (
    /* 64bit * 64bit -> 64bit multiplication. Works for both signed and unsigned */
    ".section .text,\"ax\",@progbits\n"
    ".global mul64       \n"    /* Notation: abcd * efgh, where each letter */
    ".type   mul64,@function\n" /* represents 16 bits. */
"mul64:                  \n"
    "lea.l   (-16,%sp),%sp   \n"
    "movem.l %d2-%d5,(%sp)   \n"

    "movem.l (20,%sp),%d0-%d3\n" /* %d0%d1 = abcd, %d2%d3 = efgh */
    "mulu.l  %d3,%d0     \n"  /* %d0 = ab * gh */
    "mulu.l  %d1,%d2     \n"  /* %d2 = cd * ef */
    "add.l   %d2,%d0     \n"  /* %d0 += %d2 */
    "move.l  %d1,%d4     \n"  
    "swap    %d4         \n"  /* %d4 = dc */
    "move.l  %d3,%d5     \n"
    "swap    %d5         \n"  /* %d5 = hg */
    "move.l  %d4,%d2     \n"
    "mulu.w  %d5,%d2     \n"  /* %d2 = c * g */
    "add.l   %d2,%d0     \n"  /* %d0 += %d2 */
    "mulu.w  %d3,%d4     \n"  /* %d4 = c * h */
    "mulu.w  %d1,%d5     \n"  /* %d5 = d * h */
    "add.l   %d4,%d5     \n"  /* %d5 += %d4 */
    "subx.l  %d4,%d4     \n"
    "neg.l   %d4         \n"  /* carry -> %d4 */
    "swap    %d4         \n"
    "clr.w   %d4         \n"
    "swap    %d5         \n" 
    "move.w  %d5,%d4     \n"
    "clr.w   %d5         \n"  /* %d4%d5 <<= 16 */
    "mulu.w  %d3,%d1     \n"  /* %d1 = d * h */
    "add.l   %d5,%d1     \n"
    "addx.l  %d4,%d0     \n"  /* %d0%d1 += %d4%d5 */

    "movem.l (%sp),%d2-%d5   \n"
    "lea.l   (16,%sp),%sp\n"
    "rts                 \n"
);
#define MUL64(a, b) mul64(a, b)

#else
#define MUL64(a, b) ((a)*(b))
#endif

int ilog2_fp(long long value) /* calculate integer log2(value_fp_6.58) */
{
    int i = 0;

    if (value <= 0) {
        return -32767;
    } else if (value > (1LL<<58)) {
        while (value >= (2LL<<58)) {
            value >>= 1;
            i++;
        }
    } else {
        while (value < (1LL<<58)) {
            value <<= 1;
            i--;
        }
    }
    return i;
}

void recalc_parameters(void)
{
    x_step = (x_max - x_min) / LCD_WIDTH;
    x_delta = MUL64(x_step, (LCD_WIDTH/8));
    y_step = (y_max - y_min) / LCD_HEIGHT;
    y_delta = MUL64(y_step, (LCD_HEIGHT/8));
    step_log2 = MIN(ilog2_fp(x_step), ilog2_fp(y_step));
    max_iter = MAX(10, -15 * step_log2 - 50);
}

void init_mandelbrot_set(void)
{
#if CONFIG_LCD == LCD_SSD1815 /* Recorder, Ondio. */
    x_min = -38LL<<54;  // -2.375<<58
    x_max =  15LL<<54;  //  0.9375<<58
#else  /* Iriver H1x0 */
    x_min = -36LL<<54;  // -2.25<<58
    x_max =  12LL<<54;  //  0.75<<58
#endif
    y_min = -19LL<<54;  // -1.1875<<58
    y_max =  19LL<<54;  //  1.1875<<58
    recalc_parameters();
}

void calc_mandelbrot_32(void)
{
    long start_tick, last_yield;
    unsigned n_iter;
    long long a64, b64;
    long x, x2, y, y2, a, b;
    int p_x, p_y;
    int brightness;

    start_tick = last_yield = *rb->current_tick;
    
    for (p_x = 0, a64 = x_min; p_x <= px_max; p_x++, a64 += x_step) {
        if (p_x < px_min)
            continue;
        a = a64 >> 32;
        for (p_y = LCD_HEIGHT-1, b64 = y_min; p_y >= py_min; p_y--, b64 += y_step) {
            if (p_y >= py_max)
                continue;
            b = b64 >> 32;
            x = 0;
            y = 0;
            n_iter = 0;

            while (++n_iter <= max_iter) {
                x >>= 13;
                y >>= 13;
                x2 = x * x;
                y2 = y * y;
                
                if (x2 + y2 > (4<<26)) break;
                
                y = 2 * x * y + b;
                x = x2 - y2 + a;
            }

            // "coloring"
            if  (n_iter > max_iter){
                brightness = 0; // black
            } else {
                brightness = 255 - (32 * (n_iter & 7));
            }
            graybuffer[p_y]=brightness;
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

void calc_mandelbrot_64(void)
{
    long start_tick, last_yield;
    unsigned n_iter;
    long long x, x2, y, y2, a, b;
    int p_x, p_y;
    int brightness;

    start_tick = last_yield = *rb->current_tick;
    
    for (p_x = 0, a = x_min; p_x < px_max; p_x++, a += x_step) {
        if (p_x < px_min)
            continue;
        for (p_y = LCD_HEIGHT-1, b = y_min; p_y >= py_min; p_y--, b += y_step) {
            if (p_y >= py_max)
                continue;
            x = 0;
            y = 0;
            n_iter = 0;

            while (++n_iter<=max_iter) {
                x >>= 29;
                y >>= 29;
                x2 = MUL64(x, x);
                y2 = MUL64(y, y);
                
                if (x2 + y2 > (4LL<<58)) break;
                
                y = 2 * MUL64(x, y) + b;
                x = x2 - y2 + a;
            }

            // "coloring"
            if  (n_iter > max_iter){
                brightness = 0; // black
            } else {
                brightness = 255 - (32 * (n_iter & 7));
            }
            graybuffer[p_y]=brightness;
            /* be nice to other threads:
             * if at least one tick has passed, yield */
            if  (*rb->current_tick > last_yield){
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
    grayscales = gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH,
                           (LCD_HEIGHT*LCD_DEPTH/8), 8, NULL) + 1;
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
            if (redraw == REDRAW_FULL)
                gray_ub_clear_display();

            if (step_log2 <= -13) /* select precision */
                calc_mandelbrot_64();
            else
                calc_mandelbrot_32();

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
