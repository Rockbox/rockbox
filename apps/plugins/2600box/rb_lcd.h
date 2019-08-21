


#ifndef RB_LCD_H
#define RB_LCD_H

#include "rb_test.h"

enum display_mode { DISPLAY_MODE_UNCHANGED = -1,    /* keep old setting */
                    DISPLAY_MODE_NATIVE = 0,
                    DISPLAY_MODE_FULLSCREEN,
                    DISPLAY_MODE_ZOOM,
                    DISPLAY_MODE_GENERIC };


/* Screen options */
struct rb_screen {
    enum display_mode lcd_mode;
    bool show_fps;
    /*int rotate;*/  /* TODO */
    int max_frameskip;  /* maximum frameskip setting */
    int lockfps;        /* frames per second to display; 0=as fast as possible */
    int tv_startline,   /* first and last line of TV framebuffer to display */
        tv_endline;
    BYTE *tv_start_read_addr; /* use VCS 8bit colours until rendering to RB frame buffer */
    int render_start,   /* TODO: move out of struct */
        render_end;
    // to be set whenever display settings change (zoom, render_start, ...)
    int x_step;         // what has to be added to "guest" (Atari) coordinate
    int y_step;         // as 16.16 fixed point number
    // "Host" (rockbox device) coordinates
    fb_data *lcd_start_addr; // TODO
    int y_offs;         // native offs, height to draw
    int y_lines;    // number of lines to draw
    int x_offs;
    int x_columns;  // number of columns to draw
    void (*put_image)(void);
};
extern struct rb_screen screen;

#include "vmachine.h"


/*
 * Write pixel on LCD, using dither pattern.
 * Patterns are defined in palette.c and stored in the palette array.
 * Note that the dither patterns are drawn in native LCD packing orientation.
 */
#if LCD_DEPTH==1
# define put_pixel(pattern, x, y)  do { \
    unsigned p = pattern >> ((x&0x3)<<3); \
    if (p & (1<<(y&0x7))) \
        rb->lcd_set_drawmode(DRMODE_SOLID); \
    else \
        rb->lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID); \
    rb->lcd_drawpixel(x, y); \
} while(0)
#elif LCD_DEPTH==2
# define put_pixel(pattern, x, y)  do { \
    unsigned p = pattern >> ((x&0x3)<<3); \
    rb->lcd_set_foreground((p>>((y&0x3)<<1)) & 0x3); \
    rb->lcd_drawpixel(x, y); \
} while(0)
#endif


void set_picture_size(const enum display_mode mode);
void set_frameskip(const int value);
/* needed to recalculate internal variables */
void set_framerate(const int value);
enum display_mode get_screen_mode(void);
void set_fps_timer(void);
void init_rb_screen(void);

void tv_display(void);

char *zoom_chg_parameter(const int s, const int h, const int v);


#endif /* RB_LCD_H */
