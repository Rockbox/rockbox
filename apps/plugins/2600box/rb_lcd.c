/*
   The RockBOX display code.
 */

/* Standard Includes */
#include "rbconfig.h"
#include "rb_test.h"

/* 2600 includes */
#include "vmachine.h"
#include "address.h"
#include "keyboard.h"
#include "lcd.h"
#include "palette.h"

#include "rb_lcd.h"


/* Screen options */
struct rb_screen screen =
    {   .show_fps=1,
        .lcd_mode=DISPLAY_MODE_NATIVE,
        /*.frameskip=0,*/
#ifdef SIMULATOR
        .lockfps=60,
        .max_frameskip=0,
#else
        .lockfps=0,
        .max_frameskip=5,
#endif
    };


// used inside tv_display()
static unsigned long fps_end;           /* ending tick for fps measurement */
static unsigned int  frame_count;       /* counted over 1 second */
static unsigned int  fps = 0;
static unsigned int  last_tick;
#define FPS_MULTIPLIER 8 // shift value/ fixed point fractional tick part
static unsigned int  skip_cnt = 0;
static int fps_ticks;    /* exact time for each frame in (fractional) ticks */
static int ft_deviation; /* summed up difference between exact and real tick time */
#ifdef IS_FAST_TARGET
static unsigned int  waitcycles = 0;     /* cycles to wait, if screenlockfps is used */
#endif
static unsigned int  frameskip = 0;      /* actually used frameskip value */


// -----------------------------------------------

// ATM only for LCD_WIDTH==LCD_FBWIDTH && LCD_HEIGHT==LCD_FBHEIGHT
#if LCD_WIDTH >= 320 && LCD_HEIGHT >= 200

/* large screens: use every line of VCS buffer, each pixel is doubled horizontally */
#   define LCD_NATIVE_XCOUNT        160  /* TV pixel to draw */
#   define LCD_NATIVE_XOFFS         ((LCD_WIDTH - 320) / 2)
#   define LCD_NATIVE_X_NOM         2    /* TV pixel to draw per x coordinate */
#   define LCD_NATIVE_X_DENOM       1    /* keep nominator/denominator 1 if unused */
#   define LCD_NATIVE_NEXTLINE      (LCD_WIDTH - 320)
#   define LCD_NATIVE_Y_STEP        1

#elif LCD_WIDTH >= 160 && LCD_HEIGHT >= 100

#   define LCD_NATIVE_XCOUNT        160  /* TV pixel to draw */
#   define LCD_NATIVE_XOFFS         ((LCD_WIDTH - 160) / 2)
#   define LCD_NATIVE_X_NOM         1    /* TV pixel to draw per x coordinate */
#   define LCD_NATIVE_X_DENOM       1    /* keep nominator/denominator 1 if unused */
#   define LCD_NATIVE_NEXTLINE      (LCD_WIDTH - 160)
#   define LCD_NATIVE_Y_STEP        2

#elif LCD_WIDTH >= 80 && LCD_HEIGHT >= 64

#   define LCD_NATIVE_XCOUNT        80  /* TV pixel to draw */
#   define LCD_NATIVE_XOFFS         ((LCD_WIDTH - 80) / 2)
#   define LCD_NATIVE_X_NOM         1    /* TV pixel to draw per x coordinate */
#   define LCD_NATIVE_X_DENOM       2    /* keep nominator/denominator 1 if unused */
#   define LCD_NATIVE_NEXTLINE      (LCD_WIDTH - 80)
#   define LCD_NATIVE_Y_STEP        3

#else
#   warning Screen size not supported!
#endif


/* =========================================================================
 * write the TV buffer (the Atari screen) to the rockbox LCD
 * A number of distinct functions are provided for different purposes:
 *   -  put_image_native()      should be the most easy and/or fast way to
 * put the picture to the screen.
 *   -  put_image()             a more generic function that master zooming
 * and moving the TV screen around
 *
 * Some more functions are imaginable for the future:
 *   -  rotate the image
 *   -  various ways to improve the image on small screens or slow machines,
 *      e.g. ways to make single line or single column items visible or
 *      to blend subsequent frames together (it's a common practive for
 *      Atari games to display sprites alternating, and if frameskipping
 *      is used some sprites will be invisible).
 */


/**
 * generic put_image function that handles all possible LCD peculiarities.
 * Should be replaced by specialised/optimized functions and then be removed.
 */
void put_image_generic(void)
{
    BYTE *src;
    int srcy, srcx;

    srcy = screen.tv_startline << 16;

    for (int y = screen.y_offs; y < screen.y_offs+screen.y_lines; ++y) {
        srcx = 0;
        for (int x = screen.x_offs; x < screen.x_offs+screen.x_columns; ++x) {
            src = &tv_screen[srcy >> 16][srcx >> 16];
#if LCD_DEPTH>=8
            rb->lcd_set_foreground(FB_UNPACK_SCALAR_LCD(vcs_palette[*src]));
            rb->lcd_drawpixel(x, y);
#else /* greyscale & mono */
            put_pixel(vcs_palette[*src], x, y);
#endif
            srcx += screen.x_step;
        }
        srcy += screen.y_step;
    }
}


#if LCD_DEPTH>=8
/**
 * write TV screen to target LCD in a "native" (or rather optimized) way.
 */
void put_image_native(void)
{
    BYTE *src;    /* use VCS 8bit colours until rendering to RB frame buffer */

    fb_data *dest;

    dest = screen.lcd_start_addr;
    src = screen.tv_start_read_addr;
    for (int y = screen.y_lines; y; --y) {
        for (int x = LCD_NATIVE_XCOUNT; x; --x) {
            *dest++ = vcs_palette[*src];
#    if LCD_NATIVE_X_NOM==2
            *dest++ = vcs_palette[*src]; /* double current pixel */
#    endif
            src++;
#  if LCD_NATIVE_X_DENOM==2
            src++;  /* skip one TV pixel */
#  endif
        }
#  if LCD_NATIVE_Y_STEP==2
        src += 160;     /* skip 1 line */
#  elif LCD_NATIVE_Y_STEP==3
        src += 160 * 2; /* skip 2 lines */
#  endif
        dest += LCD_NATIVE_NEXTLINE;
    }
}
#endif /* LCD_DEPTH>=8 */

// ----------------------------

/*
 * Greyscale: experimental
 */
#if LCD_DEPTH == 2 && LCD_PIXELFORMAT == HORIZONTAL_PACKING
// TODO: this doesn't work with dither yet!
void put_image_native_2bpp_h(void)
{
    BYTE *src;    /* use VCS 8bit colours until rendering to RB frame buffer */

    fb_data *dest;

    dest = screen.lcd_start_addr;
    src = screen.tv_start_read_addr;
    for (int y = screen.y_lines; y; --y) {
        for (int x = LCD_NATIVE_XCOUNT>>2; x; --x) {
            fb_data px;
            px = vcs_palette[*src];
# if LCD_NATIVE_X_NOM==2
            ++src;
            px <<= 4;
            px |= vcs_palette[*src++];
            px = px | (px << 2);
# else
            src += LCD_NATIVE_X_DENOM;
            px = (px << 2) | vcs_palette[*src];
            src += LCD_NATIVE_X_DENOM;
            px = (px << 2) | vcs_palette[*src];
            src += LCD_NATIVE_X_DENOM;
            px = (px << 2) | vcs_palette[*src];
            src += LCD_NATIVE_X_DENOM;
# endif
            *dest++ = px;
        }
#  if LCD_NATIVE_Y_STEP==2
        src += 160;     /* skip 1 line */
#  elif LCD_NATIVE_Y_STEP==3
        src += 160 * 2; /* skip 2 lines */
#  endif
        dest += (LCD_NATIVE_NEXTLINE+3) >> 2;
    }
}
#endif /* LCD_DEPTH == 2 && LCD_PIXELFORMAT == HORIZONTAL_PACKING */


#if LCD_DEPTH == 2 && LCD_PIXELFORMAT == VERTICAL_PACKING
void put_image_native_2bpp_v(void)
{
    BYTE *src;    /* use VCS 8bit colours until rendering to RB frame buffer */

    fb_data *dest;

    src = screen.tv_start_read_addr;
    int y = screen.y_offs;
    for (int yy = screen.y_lines; yy; --yy, ++y) {
        unsigned shift = (y & 0x03) << 1;
        unsigned mask = 0x03 << shift;
        dest = rb->lcd_framebuffer + (y>>2) * LCD_FBWIDTH + LCD_NATIVE_XOFFS;
        for (int x = LCD_NATIVE_XCOUNT; x; --x) {
            uint32_t px;
            px = vcs_palette[*src];
            px >>= (x&0x3)<<3;
            fb_data tmp = *dest;
            *dest++ = tmp ^ ((px ^ tmp) & mask);
#    if LCD_NATIVE_X_NOM==2
            *dest++ = tmp ^ (px ^ tmp);
#    endif
            src += LCD_NATIVE_X_DENOM;
        }

        src += 160 * (LCD_NATIVE_Y_STEP-1);
    }
}
#endif /* LCD_DEPTH == 2 && LCD_PIXELFORMAT == VERTICAL_PACKING */


#if LCD_DEPTH>=8
/**
 * Helper function to place the tv image. Supports zoomed output.
 */
void put_image (void)
{
    fb_data *dest;
    BYTE *src;
    int srcy, srcx;

    dest = screen.lcd_start_addr;
    srcy = screen.tv_startline << 16;

    for (int y = screen.y_lines; y; --y) {
        srcx = 0;
        for (int x = screen.x_columns; x; --x) {
            src = &tv_screen[srcy >> 16][srcx >> 16];
            *dest++ = vcs_palette[*src];
            srcx += screen.x_step;
        }
        srcy += screen.y_step;
        dest += LCD_FBWIDTH - screen.x_columns;
    }
}
#endif


/* ------------------------------------------------------------------------- */



/*
 * Helper fuction for frame time measurement, to keep it out of main code.
 * Only used if corresponding option in rb_test is set.
 */

#if TST_TIME_MEASURE
// returns 1 if display function should be skipped
static inline bool measure_frame_time(void)
{
    if (tst_time_measure) {
        if (!tst_time_measure_fc) {
            rb->splash(HZ*7/10, "Starting Time Measurement");
            tst_time_measure_fc = TST_TIME_MEASURE_FRAMES;
            tst_time_measure_time = *rb->current_tick;
        }
        else {
            --tst_time_measure_fc;
            if (!tst_time_measure_fc) {
                /* time measurement ended: print result */
                tst_time_measure = 0;
                sync_state &= ~SYNC_STATE_VBLANK;
                tst_time_measure_time = *rb->current_tick - tst_time_measure_time;
                int ms = tst_time_measure_time*1000/HZ / TST_TIME_MEASURE_FRAMES;
                int frac = (tst_time_measure_time*1000/HZ - ms*TST_TIME_MEASURE_FRAMES)
                            * 1000 / TST_TIME_MEASURE_FRAMES;
                rb->splashf(HZ*2, "Time: %d.%03d ms/frame", ms, frac);
                rb->button_clear_queue();
                while (!rb->button_get(10))
                    ;
            }
        }
        if (tst_time_measure >= 2) {
            return true; /* don't display anything */
        }
    }
    return false;
}
#endif /* TST_TIME_MEASURE */



/**
 * Display the Atari TV screen
 */
void tv_display(void)
{
    TST_PROFILE(PRF_DRAW_FRAME, "tv_display()");
#if TST_TIME_MEASURE
    if (measure_frame_time())
        return;
#endif

    /* uncomment to simulate slow targets */
#if 0 // defined(SIMULATOR)
    rb->sleep(1);
#endif

    /* measure fps */
    ++frame_count;
    long tick = *rb->current_tick;
    if (TIME_AFTER(tick, fps_end)) {
        fps_end = tick + HZ - 1;
        fps = frame_count;
        frame_count = 0;
    }

    /* update frame skips or wait cycles to lock to specified fps value */
    if (screen.lockfps) {
        int tmp = fps_ticks - ((tick - last_tick)<<FPS_MULTIPLIER);
        last_tick = tick;
        ft_deviation += tmp;
        if (ft_deviation+tmp < 0) {
# ifdef IS_FAST_TARGET
            if (waitcycles)
                --waitcycles;
            else
# endif
                if (frameskip < (unsigned) screen.max_frameskip)
                    ++frameskip;
        }
        else if (ft_deviation+tmp > 0) {
            if (frameskip)
                --frameskip;
# ifdef IS_FAST_TARGET
            else
                ++waitcycles;
# endif
        }
# ifdef IS_FAST_TARGET
        if (waitcycles)
            rb->sleep(waitcycles-1);
# endif
    }


    if (!skip_cnt) {
        screen.put_image();
    }

    if (!skip_cnt) {
        skip_cnt = frameskip;
        if (screen.show_fps) {
#ifdef IS_FAST_TARGET
            rb->lcd_putsxyf(0,0,"%d fps/%d skip/%d wait", fps, frameskip, waitcycles);
#else
            rb->lcd_putsxyf(0,0,"%d fps/%d skip", fps, frameskip);
#endif
        }
#if TST_DISPLAY_ALERTS
        if (tst_alert_timer) {
            --tst_alert_timer;
            rb->lcd_putsxyf(0,LCD_HEIGHT-30, tst_get_alert_tokens());
        }
#endif
#if TST_FRAME_STAT_ENABLE
        if (tst_frame_stat.show) {
            rb->lcd_putsxyf(0,LCD_HEIGHT-15,"v=%d+%d+%d+%d=%d (%d wsynced)",
                    tst_frame_stat.vsynced, tst_frame_stat.vblanked0,
                    tst_frame_stat.drawn, tst_frame_stat.vblanked1,
                    tst_frame_stat.vsynced+tst_frame_stat.vblanked0
                      +tst_frame_stat.drawn+tst_frame_stat.vblanked1,
                    tst_frame_stat.wsynced);
        }
#endif
        rb->lcd_update();
    }
    else {
        --skip_cnt;
    }
}

/* -------------------------------------------------------------------------
 */

enum display_mode get_screen_mode(void)
{
    return screen.lcd_mode;
}

/* =========================================================================
 * set parameters in screen struct for display.
 * parameter: new size. 0=="native", 1==fullscreen, 2==custom zoom (to be done)
 */

// TODO: must also be called when changing NTSC/PAL!
// and when "render_start" is changed!
void set_picture_size(const enum display_mode mode)
{
    if (mode != DISPLAY_MODE_UNCHANGED)
        screen.lcd_mode = mode;
    switch (screen.lcd_mode) {
        case DISPLAY_MODE_NATIVE:
            screen.tv_startline = (tv.vblank + tv.height/2) - LCD_HEIGHT*LCD_NATIVE_Y_STEP/2;
            screen.tv_endline = screen.tv_startline + LCD_HEIGHT*LCD_NATIVE_Y_STEP;
            if (screen.tv_startline < 0) screen.tv_startline = 0;
            if (screen.tv_startline < screen.render_start)
                screen.tv_startline = screen.render_start;
            if (screen.tv_endline > screen.render_end)
                screen.tv_endline = screen.render_end;
            screen.y_lines = (screen.tv_endline-screen.tv_startline) / LCD_NATIVE_Y_STEP;
            screen.y_offs = (LCD_HEIGHT - screen.y_lines) / 2;
            screen.y_step = LCD_NATIVE_Y_STEP << 16;
            screen.x_columns = LCD_NATIVE_XCOUNT * LCD_NATIVE_X_NOM;
            screen.x_offs = LCD_NATIVE_XOFFS; //(LCD_WIDTH - screen.x_columns) / 2;
            screen.x_step = (LCD_NATIVE_X_DENOM << 16) / LCD_NATIVE_X_NOM;
#if LCD_DEPTH>=8 && !(defined(LCD_STRIDEFORMAT) && (LCD_STRIDEFORMAT==VERTICAL_STRIDE))
            screen.put_image = put_image_native;
#elif LCD_DEPTH==2 && LCD_PIXELFORMAT == HORIZONTAL_PACKING
            screen.put_image = put_image_native_2bpp_h;
#elif LCD_DEPTH==2 && LCD_PIXELFORMAT == VERTICAL_PACKING
            screen.put_image = put_image_native_2bpp_v;
#else
            screen.put_image = put_image_generic;
#endif
            break;
        case DISPLAY_MODE_FULLSCREEN:
            screen.tv_startline = tv.vblank;
            screen.tv_endline = screen.tv_startline+tv.height;
            screen.y_offs = 0; // TODO: change this on portrait mode displays
            screen.y_lines = LCD_HEIGHT;
            screen.y_step = ((screen.tv_endline-screen.tv_startline) << 16) / LCD_HEIGHT;
            screen.x_offs = 0;
            screen.x_columns = LCD_WIDTH;
            screen.x_step = (TV_SCREEN_WIDTH << 16) / LCD_WIDTH;
#if LCD_DEPTH>=8
            screen.put_image = put_image;
#else
            /* TODO: write optimized functions and replace the following */
            screen.put_image = put_image_generic;
#endif
            break;
        case DISPLAY_MODE_ZOOM:
#if 0 // LCD_DEPTH>=8
            screen.put_image = put_image;
#else
            screen.put_image = put_image_generic;
#endif
            break;
        case DISPLAY_MODE_GENERIC:
        default:
            screen.put_image = put_image_generic;
            break;
    }
    screen.tv_start_read_addr = &tv_screen[screen.tv_startline][0];
#if LCD_DEPTH>=8
    screen.lcd_start_addr = rb->lcd_framebuffer + screen.y_offs*LCD_FBWIDTH + screen.x_offs;
#elif LCD_DEPTH==2 && LCD_PIXELFORMAT==HORIZONTAL_PACKING
    screen.lcd_start_addr = rb->lcd_framebuffer + screen.y_offs*LCD_FBWIDTH + (screen.x_offs>>2);
#else // TODO
    screen.lcd_start_addr = rb->lcd_framebuffer + screen.y_offs*LCD_FBWIDTH;
#endif
}


void set_frameskip(const int value)
{
    screen.max_frameskip = value;
    if ((int)frameskip > value)
        frameskip = value;
}

/**
 * @brief This function needs to be called whenever screen.lockfps is set to
 *        something different than 0 (recalculating internal variables)
 */
void set_framerate(const int value)
{
    /* Set internal wait/skip variables closer to needed values */
    /* we can only estimate values if we have a valid old fps setting */
    if (value > 0 && screen.lockfps > 0) {
        int diff = screen.lockfps - value;
#ifdef IS_FAST_TARGET
        int wait = (int) waitcycles;
#endif
        int skip = (int) frameskip;
        if (diff < 0) {
#ifdef IS_FAST_TARGET
            if (wait > 0) {
                wait += diff;
                diff = 0;
                if (wait < 0) {
                    diff = wait;
                    wait = 0;
                }
            }
#endif
            skip = -diff;
            if (skip > screen.max_frameskip)
                skip = screen.max_frameskip;
        }
        else if (diff > 0) {
            if (skip > 0) {
                skip -= diff;
                diff = 0;
                if (skip < 0) {
                    diff = -skip;
                    skip = 0;
                }
#ifdef IS_FAST_TARGET
                wait += diff;
#endif
            }
        }
#ifdef IS_FAST_TARGET
        waitcycles = (unsigned) wait;
#endif
        frameskip = (unsigned) skip;
    }
    screen.lockfps = value; /* set new value */
    /* calculate exact time for each frame in (fractional) ticks (fixed point) */
    fps_ticks = (HZ<<FPS_MULTIPLIER) / screen.lockfps;
}


void init_rb_screen(void) COLD_ATTR;
void init_rb_screen(void)
{
    set_picture_size(DISPLAY_MODE_NATIVE);
    set_framerate(screen.lockfps);  // TODO: pass real value
}

/**
 * @brief This function should be called to update the internal frame time
 *        variables of the display code. Without calling the display code needs
 *        quite a long time to lock to the specified framerate.
 */
void set_fps_timer(void)
{
    /* reset fps display */
    fps_end = *rb->current_tick + HZ - 1;
    frame_count = 0;
    /* reset frametime for calculating wait/skip */
    skip_cnt = 0;
    last_tick = *rb->current_tick; // - fps_ticks;
}

char *zoom_chg_parameter(const int s, const int h, const int v)
{
    static int set;
    static char output[40];

    if (s<0) {
        set = 0;
    }
    else if (s>0) {
        ++set;
        if (set>1) set = 0;
    }
    switch (set) {
        case 0:
            if (h) {
                screen.x_columns += h;
                screen.x_step = (TV_SCREEN_WIDTH << 16) / screen.x_columns;
                if (screen.x_columns & 1)   /* try to keep picture centred */
                    screen.x_offs -= h;
            }
            if (v) {
                screen.y_lines += v;
                screen.y_step = ((screen.tv_endline-screen.tv_startline) << 16)
                                  / screen.y_lines;
                if (screen.y_lines & 1)   /* try to keep picture centred */
                    screen.y_offs -= v;
            }
            rb->snprintf(output, sizeof(output),
                "width=%d, height=%d", screen.x_columns, screen.y_lines);
            break;
        case 1:
            if (h) {
                screen.x_offs += h;
            }
            if (v) {
                screen.y_offs += v;
            }
            rb->snprintf(output, sizeof(output),
                "offset x=%d, y=%d", screen.x_offs, screen.y_offs);
            break;
    }
    return output;
}
