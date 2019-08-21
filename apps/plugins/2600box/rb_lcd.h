


#ifndef RB_LCD_H
#define RB_LCD_H


/* Screen options */
struct rb_screen {
    int show_fps;
    int show_fullscreen;
    int frameskip;
    int max_frameskip;
    int lockfps;        /* frames per second to display; 0=as fast as possible */
    int y_start, y_end; /* lines in vcs-framebuffer to display */
    int render_start,   /* TODO: woanders definieren, hier nur sachen für Anzeige! */
        render_end;     /* welche Zeilen sollen überhaupt in den vcs framebuffer */
    // to be set whenever display settings change (zoom, render_start, ...)
    int y_offs;         // native offs, height to draw; render und y_start beachtet.
    int y_lines;
    int y_step;         // as 16.16 fixed point number; (?) not used for "native" display
    int x_offs;
    int x_columns;
    int x_step;
};
extern struct rb_screen lcd;

#include "vmachine.h"
struct palette {
    enum tv_system type;
    int brightness;
    int contrast;
    int saturation;
    int c_shift;
    int col_step;
};
extern struct palette palette;


void set_picture_size(int sz);
void set_frameskip(int value);

void init_rb_screen(void);

void show_palette(void);
void make_palette(int yuv_mode, int colour_shift, int saturation,
    int contrast, int brightness, int pr_displacement, int pb_displacement, int col_step);

void tv_display(void);

#if COLOUR_HANDLING==0
extern fb_data vcs_palette[256];
#elif COLOUR_HANDLING==1
extern fb_data vcs_palette[256];
#else
extern UINT32 vcs_palette[256];
#endif

#endif /* RB_LCD_H */
