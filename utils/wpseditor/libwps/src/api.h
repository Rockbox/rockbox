#ifndef API_H_INCLUDED
#define API_H_INCLUDED
#include <stdbool.h>
#include <stddef.h>
#include "defs.h"
#include "wpsstate.h"
#ifdef __PCTOOL__
#include "dummies.h"
#endif

struct viewport_api {
    int x;
    int y;
    int width;
    int height;
    int font;
    int drawmode;
    unsigned fg_pattern;
    unsigned bg_pattern;
    unsigned lss_pattern;
    unsigned lse_pattern;
    unsigned lst_pattern;
    
    //TODO: ??
    int fontheight;
    int fontwidth;
};

struct proxy_api
{
    bool        (*load_remote_wps_backdrop)(char* file_name);
    bool        (*load_wps_backdrop)(char* file_name);

    unsigned    (*get_foreground)(void);
    unsigned    (*get_background)(void);
    int         (*getwidth)(void);
    int         (*getheight)(void);

    void        (*puts_scroll)(int x, int y, const unsigned char *string);
    void        (*putsxy)(int x, int y, const unsigned char *str);
    int         (*getfont)();
    int         (*getstringsize)(const unsigned char *str, int *w, int *h);
    void        (*stop_scroll)();

    void        (*transparent_bitmap_part)(const void *src, int src_x, int src_y,
                                        int stride, int x, int y, int width, int height);
    void        (*bitmap_part)(const void *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height);
    void        (*hline)(int x1, int x2, int y);
    void        (*vline)(int x, int y1, int y2);
    void        (*drawpixel)(int x, int y);
    void        (*set_drawmode)(int mode);
    void        (*fillrect)(int x, int y, int width, int height);


    void        (*update)();
    void        (*set_viewport)(struct viewport* vp);
    void        (*clear_display)(void);
    void        (*clear_viewport)(int x,int y,int w,int h, int color);

    void*       (*plugin_get_buffer)(size_t *buffer_size);
    int         (*read_bmp_file)(const char* filename,int *width, int *height);
    void        (*set_wpsstate)(struct wpsstate state);
    void        (*set_trackstate)(struct trackstate state);
    void        (*set_next_trackstate)(struct trackstate state);
    void        (*set_audio_status)(int status);
    
    pfdebugf    debugf;
    int            verbose;


/**************************
*          OUT            *
**************************/
    const char* (*get_model_name)();
    void        (*get_current_vp)(struct viewport_api *avp);


};

extern struct proxy_api *xapi;

EXPORT int set_api(struct proxy_api* api);

#endif // API_H_INCLUDED
