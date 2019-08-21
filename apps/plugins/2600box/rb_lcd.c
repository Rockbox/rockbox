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

#include "rb_lcd.h"
#include "lcd.h"

#if COLOUR_HANDLING==0
#elif COLOUR_HANDLING==1
#else
#define Palette_GetR(x) ((vcs_palette[x] >> 16) & 0xff)
#define Palette_GetG(x) ((vcs_palette[x] >> 8) & 0xff)
#define Palette_GetB(x) ( vcs_palette[x] & 0xff)
#endif
#define Palette_GetY(x) ((30 * Palette_GetR(x) + 59 * Palette_GetG(x) + 11 * Palette_GetB(x))/100)


#define NUMCOLS 256
#if COLOUR_HANDLING==0
fb_data vcs_palette[NUMCOLS];
#elif COLOUR_HANDLING==1
fb_data vcs_palette[NUMCOLS];
#else
UINT32 vcs_palette[NUMCOLS];
#endif



/* Screen options */
struct rb_screen lcd =
    {   .show_fps=1, .show_fullscreen = 0, .frameskip=0,
#ifdef SIMULATOR
        .lockfps=50,
        .max_frameskip=0,
#else
        .lockfps=0,
        .max_frameskip=5,
#endif
        .y_start=(37+192/2)-LCD_HEIGHT/2,
        .y_end=(37+192/2)+LCD_HEIGHT/2,
        .render_start=0, .render_end=320,
    };


// used inside tv_display()
static unsigned long last_tick;
static unsigned int  frames;             /* counted over 1 second */
static unsigned int  fps = 0;
static unsigned int  frames10;           /* frames in 1/10 sec (for fixed fps) */
static unsigned int  last_tick10;
static unsigned int  skip_cnt = 0;
#ifdef IS_FAST_TARGET
static unsigned int  waitcycles = 0;     /* cycles to wait, if lcd.lockfps is used */
#endif
static unsigned int  frameskip = 0;      /* actually used frameskip value */



struct palette palette =
    {
        .type = TV_NTSC,
        .brightness = 5,
        .contrast = 0,
        .saturation = 35,
        .c_shift = 12,
        .col_step = 30,
    };


/* ========================================================= */

/*
 * show palette on lcd screen
 *
 */
void show_palette(void) COLD_ATTR;
void show_palette(void)
{
    //rb->lcd_clear_display();
    //rb->lcd_set_drawmode(DRMODE_SOLID);
    for (int y = 0; y < LCD_HEIGHT; ++y) {
        for (int x = 0; x < LCD_WIDTH; ++x) {
            int vcs_colour = (((x * 16 / LCD_WIDTH) << 4)
                            | (y * 16 / LCD_HEIGHT));
#if NUMCOLS < 256
            vcs_colour >>= 1;
#endif
#if COLOUR_HANDLING==0
            rb->lcd_framebuffer[y*LCD_FBWIDTH+x] = (vcs_palette[vcs_colour]);
#elif COLOUR_HANDLING==1
            rb->lcd_framebuffer[y*LCD_FBWIDTH+x] = (vcs_palette[vcs_colour]);
#else
#  ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(LCD_RGBPACK(
                Palette_GetR(vcs_colour),Palette_GetG(vcs_colour), Palette_GetB(vcs_colour) ));
#  else
            rb->lcd_set_foreground(Palette_GetY(vcs_colour));
#  endif
            rb->lcd_drawpixel(x, y);
#endif
        }
    }
    rb->lcd_update();
}


#include "fixedpoint.h"

// YCbCr/YPbPr/YUV - colour scheme conversion
/*
 * yuv_mode: experimental
 * h=hue -> 0..3600 (10th degree)
 * s=saturation 0..1
 * l=lightness 0..1
 * rr,gg,bb= int rgb values
 */
static void hsl2rgb(int yuv_mode, int h, float s, float l,
        int pr_displacement, int pb_displacement,
        int *rr, int *gg, int *bb)
{
    float r, g, b;

    float y = l;

    float pb = (float)fp14_cos((h%3600)/10)/16384.0;
    float pr = (float)fp14_sin((h%3600)/10)/16384.0;

    if (yuv_mode!=2) {
        pb = pb*s+((float)pb_displacement/100.0);
        pr = pr*s+((float)pr_displacement/100.0);
    }
    else {
        pb = pb*s;
        pr = pr*s;
    }

    // YPbPr to rgb according to Wikipedia
    if (yuv_mode==0) {
        r = y + 1.140*pr;
        b = y + 2.028*pb;
        g = y - 0.394*pb - 0.581*pr;
    }
    else if (yuv_mode==1) {
        r = y + 1.000*pr;
        b = y + 1.000*pb;
        g = y - 0.5*pb - 0.5*pr;
    }
    else if (yuv_mode==2) {
        r = y + pr*1.403;
        b = y + pb*1.770;
        g = y - 0.344*pb - 0.714*pr;
    }
    else if (yuv_mode==3) { // looks exactly like standard mode (0)
        float i = (float)fp14_cos((480-h/10)%360)/16384.0;
        float q = (float)fp14_sin((480-h/10)%360)/16384.0;
        i = i*s;
        q = q*s;
        r = y + 0.947*i + 0.624*q;
        g = y - 0.275*i - 0.636*q;
        b = y - 1.1*i + 1.7*q;
    }
    else if (yuv_mode==4) {
        g = y - (float)pb_displacement/100.0*pb - (float)pr_displacement/100.0*pr;
        r = y + pr/((float)pr_displacement/100.0);
        b = y + pb/((float)pb_displacement/100.0);
    }

    *rr = 256 * r;
    *gg = 256 * g;
    *bb = 256 * b;
    if (*rr < 0)   *rr = 0;
    if (*gg < 0)   *gg = 0;
    if (*bb < 0)   *bb = 0;
    if (*rr > 255) *rr = 255;
    if (*gg > 255) *gg = 255;
    if (*bb > 255) *bb = 255;
}



/* create palette */
// saturation as % value 0..100
// min and max_l lightness in % 0..100
// colour shift -50..50 of 1/6 colour wheel
void make_palette(int yuv_mode, int colour_shift, int saturation,
    int contrast, int brightness, int pr_displacement, int pb_displacement, int col_step) COLD_ATTR;
void make_palette(int yuv_mode, int colour_shift, int saturation,
    int contrast, int brightness, int pr_displacement, int pb_displacement, int col_step)
{
    int h; // 0..3600 (10th degree)
    float l;
    float s = saturation/100.0f;
    int r, g, b;

    float min_l = 0.0 + (float)brightness / 100.0;
    float max_l = 1.0 + (float)contrast / 100.0;

    h = 1800 + colour_shift*10/2; // we start at ~yellow
    // h-step in 10th degree
    int h_step = -3600 / 13 + col_step;
    float l_step = (max_l - min_l) / 7.0f;
    for (int i=0; i<16; ++i) {
        l = min_l;
        for (int j = 0; j < 8; ++j) {
            if (i==0) {
                // special case grey: saturation=0
                hsl2rgb(yuv_mode, h, 0.0, l, pr_displacement, pb_displacement, &r, &g, &b);
            }
            else {
                hsl2rgb(yuv_mode, h, s, l, pr_displacement, pb_displacement, &r, &g, &b);
            }
#if COLOUR_HANDLING==0
            vcs_palette[i*16+j*2] = FB_RGBPACK(r, g, b);
            vcs_palette[i*16+j*2+1] = FB_RGBPACK(r, g, b);
#elif COLOUR_HANDLING==1
            vcs_palette[i*16+j*2] = FB_RGBPACK(r, g, b);
            vcs_palette[i*16+j*2+1] = FB_RGBPACK(r, g, b);
#else
            vcs_palette[i*16+j*2] = (r<<16) | (g<<8) | (b);
            vcs_palette[i*16+j*2+1] = (r<<16) | (g<<8) | (b);
#endif
            l += l_step;
        }
        h += h_step;
        if (h >= 3600) h -= 3600;
        if (h < 0)    h += 3600;
    }
}


/* create palette according to struct palette data */
void init_palette(void) COLD_ATTR;
void init_palette(void)
{
    // TODO: TV-System
    make_palette(   0,
                    palette.c_shift,
                    palette.saturation,
                    palette.contrast,
                    palette.brightness,
                    0,
                    0,
                    palette.col_step );
}

/* ============================================================ */


/*
 * set parameters in lcd struct for display.
 * parameter: new size. 0=="native", 1==fullscreen, 2==custom zoom (to be done)
 */
// TODO: must also be called when changing NTSC/PAL!
// and when "render_start" is changed!
void set_picture_size(int sz)
{
#if LCD_HEIGHT >= 200
            int factor=1;
#elif LCD_HEIGHT >= 100
            int factor=2;
#else
            int factor=3;
#endif

    switch (sz) {
        case 0: // native size
            lcd.y_start = (tv.vblank + tv.height/2) - LCD_HEIGHT*factor/2;
            if (lcd.y_start < 0) lcd.y_start = 0;
            lcd.y_end = lcd.y_start + LCD_HEIGHT*factor;
            if (lcd.y_start < lcd.render_start)
                lcd.y_start = lcd.render_start;
            if (lcd.y_end > lcd.render_end)
                lcd.y_end = lcd.render_end;
            lcd.y_lines = (lcd.y_end-lcd.y_start) / factor;
            lcd.y_offs = (LCD_HEIGHT - lcd.y_lines) / 2;
            break;
        case 1: // full screen
            lcd.y_start = tv.vblank;
            lcd.y_end = lcd.y_start + tv.height;
            lcd.y_offs = 0; // TODO: change this on portrait mode displays
            lcd.y_lines = LCD_HEIGHT;
            lcd.y_step = ((lcd.y_end-lcd.y_start) << 16) / LCD_HEIGHT;
            lcd.x_offs = 0;
            lcd.x_columns = LCD_WIDTH;
            lcd.x_step = (tv.width << 16) / LCD_WIDTH;
            break;
        default:
            break;
    }

}


void set_frameskip(int value)
{
    lcd.max_frameskip = value;
    if ((int)frameskip > value)
        frameskip = value;
}

/* ================================================================== */


/*
 * write vcs framebuffer to target LCD
 * in a "native" or "optimized" (mainly for speed) size
 */
#if LCD_WIDTH >= 320 && LCD_HEIGHT >= 200

/* large screens: use every line of VCS buffer, each pixel is doubled horizontally */
void put_image_native(void)
{
#  if COLOUR_HANDLING==0
    /* use VCS 8bit colours until rendering to RB frame buffer */
    BYTE * src;
#  elif COLOUR_HANDLING==1
    fb_data *src;
#  else
/* old version */
    unsigned int * src;
#  endif

    fb_data *dest;
    int y;

    dest = rb->lcd_framebuffer;
    for(y = 0; y < LCD_HEIGHT; y++) {
        src = &tv_screen[lcd.y_start + y][0];
        // TODO: this function should also work with targets >320 pixel width!!
        // TODO: set frame buffer dest for each line
        for(unsigned int x = 160; x; --x) {
#  if COLOUR_HANDLING==0
            /* use VCS 8bit colours until rendering to RB frame buffer */
            *dest++ = vcs_palette[*src];
            *dest++ = vcs_palette[*src];
#  elif COLOUR_HANDLING==1
            *dest++ = *src;
            *dest++ = *src;
#  else
            /* old version */
            *dest++ = FB_SCALARPACK(*src);
            *dest++ = FB_SCALARPACK(*src);
#  endif
            src++;
        }
    }
}

#elif LCD_WIDTH >=160 && LCD_HEIGHT >= 100

/* medium screens: use every 2nd line of VCS buffer */
void put_image_native(void)
{
    fb_data *dest;
    int y;
#  if COLOUR_HANDLING==0
    BYTE *src;
#  elif COLOUR_HANDLING==1
    fb_data *src;
#  else
    unsigned int *src;
#  endif

    dest = rb->lcd_framebuffer;
    for(y = 0; y < LCD_HEIGHT; y++) {
        src = &tv_screen[lcd.y_start + y*2][0];
        for(unsigned int x = 160; x; --x) {
#  if COLOUR_HANDLING==0
            /* use VCS 8bit colours until rendering to RB frame buffer */
            *dest++ = vcs_palette[*src];
#  elif COLOUR_HANDLING==1
            *dest++ = *src;
#  else
            /* old version */
            *dest++ = FB_SCALARPACK(*src);
#  endif
            src++;
        }
        //frameb += LCD_WIDTH;
        // TODO: make this work also for framebuffers x>160 and non-continuous FB
    }
}

#else /* LCD_WIDTH < 160 || LCD_HEIGHT < 100 */

/* small screens: width >= 80, height >= 64 */
/* horizontal: write every 2nd pixel. vertical: write every 3rd line */
void put_image_native(void)
{
    fb_data *dest;
    int y;
#  if COLOUR_HANDLING==0
    BYTE *src;
#  elif COLOUR_HANDLING==1
    fb_data *src;
#  else
    unsigned int *src;
#  endif

    dest = rb->lcd_framebuffer;
    for(y = 0; y < LCD_HEIGHT; y++) {
        src = &tv_screen[lcd.y_start + y*3][0];
        for(unsigned int x = 80; x; --x) {
#  if COLOUR_HANDLING==0
            /* use VCS 8bit colours until rendering to RB frame buffer */
            *dest++ = vcs_palette[*src];
#  elif COLOUR_HANDLING==1
            *dest++ = *src;
#  else
            /* old version */
            *dest++ = FB_SCALARPACK(*src);
#  endif
            src += 2;
        }
    }
}
#endif /* LCD screen size */


/* Inline helper to place the tv image */
void put_image (void)
//static inline void put_image (void)
{
    fb_data *dest;
#if COLOUR_HANDLING==0
    BYTE *src;
#elif COLOUR_HANDLING==1
    fb_data *src;
#else
    unsigned int *src;
#endif
    int srcy, srcx;

    dest = rb->lcd_framebuffer + lcd.y_offs*LCD_FBWIDTH + lcd.x_offs;
    //dest = FBADDR(lcd.x_offs, lcd.y_offs);
    srcy = lcd.y_start << 16;
    srcx = 0;

    for (int y = lcd.y_lines; y; --y) {
        src = &tv_screen[srcy >> 16][srcx];
        for (int x = lcd.x_columns; x; --x) {
#  if LCD_DEPTH >= 24
#    if COLOUR_HANDLING==0
            *dest++ = vcs_palette[*src];
#    elif COLOUR_HANDLING==1
            *dest++ = *src;
#    else
            *dest++ = FB_SCALARPACK(src[srcx>>16]);
#    endif
#  else
#    if COLOUR_HANDLING==0
            *dest++ = vcs_palette[*src];
#    elif COLOUR_HANDLING==1
            *dest++ = *src;
#    else
            *dest++ = FB_RGBPACK( Palette_GetR(src[srcx>>16]),
                    Palette_GetG(src[srcx>>16]), Palette_GetB(src[srcx>>16]));
#    endif
            //TODO: elif greyscale...
#  endif
            srcx += lcd.x_step;
        }
        srcy += lcd.y_step;
        dest += LCD_FBWIDTH - lcd.x_columns;
    }
}


/*
 * Helper fuction for frame time measurement, to keep it out of main code.
 * Only used if corresponding option in rb_test is set.
 */
#if TST_TIME_MEASURE
#  define TST_MEASURE_TIME()  measure_frame_time()
#else
#  define TST_MEASURE_TIME()
#endif

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
        if (tst_time_measure >= 5) {
            return true; /* don't display anything */
        }
    }
    return false;
}
#endif /* TST_TIME_MEASURE */



/* Displays the tv screen */
void tv_display(void)
{
    TST_PROFILE(PRF_DRAW_FRAME, "tv_display()");
    if (TST_MEASURE_TIME())
        return;

    if (!skip_cnt) {
        if (lcd.show_fullscreen)
            put_image ();
        else
            put_image_native ();
    }

    ++frames;
    if (*rb->current_tick - last_tick >= HZ) {
        last_tick = *rb->current_tick;
        fps = frames;
        frames = 0;
    }

    if (!skip_cnt) {
        skip_cnt = frameskip;
        if (lcd.show_fps) {
#ifdef IS_FAST_TARGET
            rb->lcd_putsxyf(0,0,"%d fps/%d skip/%d wait", fps, frameskip, waitcycles);
#else
            rb->lcd_putsxyf(0,0,"%d fps/%d skip", fps, frameskip);
#endif
        }
#if TST_FRAME_STAT_ENABLE
        if (tst_frame_stat.show) {
            rb->lcd_putsxyf(0,LCD_HEIGHT-20,"v=%d+%d+%d+%d=%d (%d unsynced)",
                    tst_frame_stat.vsynced, tst_frame_stat.vblanked0,
                    tst_frame_stat.drawn, tst_frame_stat.vblanked1,
                    tst_frame_stat.vsynced+tst_frame_stat.vblanked0
                      +tst_frame_stat.drawn+tst_frame_stat.vblanked1,
                    tst_frame_stat.unsynced);
        }
#endif
        rb->lcd_update();
    }
    else {
        --skip_cnt;
    }

    if (lcd.lockfps) {
        ++frames10;
        if (*rb->current_tick - last_tick10 >= HZ/10) {
            last_tick10 = *rb->current_tick;
            int fps10 = frames10 * 10;
            frames10 = 0;
            if (fps10 < lcd.lockfps) {
#ifdef IS_FAST_TARGET
                if (waitcycles)
                    --waitcycles;
                else
#endif
                    if (frameskip < (unsigned) lcd.max_frameskip)
                        ++frameskip;
            }
            else if (fps10 > lcd.lockfps) {
                if (frameskip)
                    --frameskip;
#ifdef IS_FAST_TARGET
                else
                    ++waitcycles;
#endif
            }
        }
#ifdef IS_FAST_TARGET
        if (waitcycles)
            rb->sleep(waitcycles-1);
#endif
    }
}


void init_rb_screen(void) COLD_ATTR;
void init_rb_screen(void)
{
    init_palette();
    set_picture_size(0);
}

