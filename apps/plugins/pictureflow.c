/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 Jonas Hurrelmann (j@outpo.st)
* Copyright (C) 2007 Nicolas Pennequin
* Copyright (C) 2007 Ariya Hidayat (ariya@kde.org) (original Qt Version)
*
* Original code: http://code.google.com/p/pictureflow/
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
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"
#include "lib/picture.h"
#include "pluginbitmaps/pictureflow_logo.h"
#include "lib/grey.h"

PLUGIN_HEADER

/******************************* Globals ***********************************/

static const struct plugin_api *rb;   /* global api struct pointer */

const struct button_mapping *plugin_contexts[]
= {generic_actions, generic_directions};

#define NB_ACTION_CONTEXTS sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

#if LCD_DEPTH < 8
#define USEGSLIB
GREY_INFO_STRUCT
#define FPLUGIN &format_grey
#define LCD_BUF _grey_info.buffer
#define MYLCD(fn) grey_ ## fn
#define G_PIX(r,g,b) \
    (77 * (unsigned)(r) + 150 * (unsigned)(g) + 29 * (unsigned)(b)) / 256
#define N_PIX(r,g,b) LCD_BRIGHTNESS(G_PIX(r,g,b))
#define G_BRIGHT(y) (y)
#define N_BRIGHT(y) LCD_BRIGHTNESS(y)
#define BUFFER_WIDTH _grey_info.width
#define BUFFER_HEIGHT _grey_info.height
typedef unsigned char pix_t;
#else
#define FPLUGIN NULL
#define LCD_BUF rb->lcd_framebuffer
#define MYLCD(fn) rb->lcd_ ## fn
#define G_PIX LCD_RGBPACK
#define N_PIX LCD_RGBPACK
#define G_BRIGHT(y) LCD_RGBPACK(y,y,y)
#define N_BRIGHT(y) LCD_RGBPACK(y,y,y)
#define BUFFER_WIDTH LCD_WIDTH
#define BUFFER_HEIGHT LCD_HEIGHT
typedef fb_data pix_t;
#endif

#define WRNDUP(w, size) (((w)+(size)-1) & (~(size - 1)))
#ifdef HAVE_SCROLLWHEEL
#define PICTUREFLOW_NEXT_ALBUM          PLA_DOWN
#define PICTUREFLOW_NEXT_ALBUM_REPEAT   PLA_DOWN_REPEAT
#define PICTUREFLOW_PREV_ALBUM          PLA_UP
#define PICTUREFLOW_PREV_ALBUM_REPEAT   PLA_UP_REPEAT
#else
#define PICTUREFLOW_NEXT_ALBUM          PLA_RIGHT
#define PICTUREFLOW_NEXT_ALBUM_REPEAT   PLA_RIGHT_REPEAT
#define PICTUREFLOW_PREV_ALBUM          PLA_LEFT
#define PICTUREFLOW_PREV_ALBUM_REPEAT   PLA_LEFT_REPEAT
#define PICTUREFLOW_NEXT_TRACK          PLA_DOWN
#define PICTUREFLOW_NEXT_TRACK_REPEAT   PLA_DOWN_REPEAT
#define PICTUREFLOW_PREV_TRACK          PLA_UP
#define PICTUREFLOW_PREV_TRACK_REPEAT   PLA_UP_REPEAT
#endif
#define PICTUREFLOW_MENU                PLA_MENU
#define PICTUREFLOW_QUIT                PLA_QUIT
#define PICTUREFLOW_SELECT_ALBUM        PLA_FIRE


/* for fixed-point arithmetic, we need minimum 32-bit long
   long long (64-bit) might be useful for multiplication and division */
#define PFreal long
#define PFREAL_SHIFT 10
#define PFREAL_FACTOR (1 << PFREAL_SHIFT)
#define PFREAL_ONE (1 << PFREAL_SHIFT)
#define PFREAL_HALF (PFREAL_ONE >> 1)


#define IANGLE_MAX 1024
#define IANGLE_MASK 1023

#define DISPLAY_SIZE (LCD_HEIGHT/2)
#define REFLECT_TOP (DISPLAY_SIZE * 4 / 3)
#define REFLECT_HEIGHT ((DISPLAY_SIZE * 2) - REFLECT_TOP)
#define REFLECT_SC ((0x10000U * 3 + (REFLECT_HEIGHT * 5 - 1)) / \
    (REFLECT_HEIGHT * 5))

#define SLIDE_CACHE_SIZE 100

#define MAX_SLIDES_COUNT 10

#define SPACING_BETWEEN_SLIDE 40
#define EXTRA_SPACING_FOR_CENTER_SLIDE 0

#define THREAD_STACK_SIZE DEFAULT_STACK_SIZE + 0x200
#define CACHE_PREFIX PLUGIN_DEMOS_DIR "/pictureflow"

#define EV_EXIT 9999
#define EV_WAKEUP 1337

/* maximum number of albums */
#define MAX_ALBUMS 1024
#define AVG_ALBUM_NAME_LENGTH 20

#define MAX_TRACKS 50
#define AVG_TRACK_NAME_LENGTH 20


#define UNIQBUF_SIZE (64*1024)

#define EMPTY_SLIDE CACHE_PREFIX "/emptyslide.pfraw"
#define EMPTY_SLIDE_BMP PLUGIN_DEMOS_DIR "/pictureflow_emptyslide.bmp"
#define CONFIG_FILE CACHE_PREFIX "/pictureflow.config"

/* Error return values */
#define ERROR_NO_ALBUMS     -1
#define ERROR_BUFFER_FULL   -2


/** structs we use */

struct slide_data {
    int slide_index;
    int angle;
    PFreal cx;
    PFreal cy;
    PFreal distance;
};

struct slide_cache {
    int index;      /* index of the cached slide */
    int hid;        /* handle ID of the cached slide */
    long touched;   /* last time the slide was touched */
};

struct album_data {
    int name_idx;
    long seek;
};

struct track_data {
    int name_idx;
    long seek;
};

struct rect {
    int left;
    int right;
    int top;
    int bottom;
};

struct load_slide_event_data {
    int slide_index;
    int cache_index;
};


struct pfraw_header {
    int32_t width;          /* bmap width in pixels */
    int32_t height;         /* bmap height in pixels */
};

const struct picture logos[]={
    {pictureflow_logo, BMPWIDTH_pictureflow_logo, BMPHEIGHT_pictureflow_logo},
};

enum show_album_name_values { album_name_hide = 0, album_name_bottom ,
    album_name_top };

struct config_data {
    long avg_album_width;
    int spacing_between_slides;
    int extra_spacing_for_center_slide;
    int show_slides;
    int zoom;
    bool show_fps;
    bool resize;
    enum show_album_name_values show_album_name;
};

/** below we allocate the memory we want to use **/

static pix_t *buffer; /* for now it always points to the lcd framebuffer */
static PFreal rays[LCD_WIDTH];
static struct slide_data center_slide;
static struct slide_data left_slides[MAX_SLIDES_COUNT];
static struct slide_data right_slides[MAX_SLIDES_COUNT];
static int slide_frame;
static int step;
static int target;
static int fade;
static int center_index; /* index of the slide that is in the center */
static int itilt;
static PFreal offsetX;
static PFreal offsetY;
static int number_of_slides;

static struct slide_cache cache[SLIDE_CACHE_SIZE];
static int  slide_cache_in_use;

/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE / sizeof(long)];
/* queue (as array) for scheduling load_surface */
static int slide_cache_stack[SLIDE_CACHE_SIZE];
static int slide_cache_stack_index;
struct mutex slide_cache_stack_lock;

static int empty_slide_hid;

unsigned int thread_id;
struct event_queue thread_q;

static long uniqbuf[UNIQBUF_SIZE];
static struct tagcache_search tcs;

static struct album_data album[MAX_ALBUMS];
static char album_names[MAX_ALBUMS*AVG_ALBUM_NAME_LENGTH];
static int album_count;

static char track_names[MAX_TRACKS * AVG_TRACK_NAME_LENGTH];
static struct track_data tracks[MAX_TRACKS];
static int track_count;
static int track_index;
static int selected_track;
static int selected_track_pulse;
void reset_track_list(void);

#define INPUT_SIZE BM_SIZE(DISPLAY_SIZE, DISPLAY_SIZE, FORMAT_NATIVE, 0)
#define SC_BUF_SIZE BM_SCALED_SIZE(DISPLAY_SIZE, 0, FORMAT_NATIVE, 0)
#define OUTPUT_SIZE BM_SIZE(DISPLAY_SIZE * 2, DISPLAY_SIZE, FORMAT_NATIVE, 0)

void * plugin_buf;
size_t plugin_buf_size;
static struct config_data config;

static int old_drawmode;

static bool thread_is_running;

static int cover_animation_keyframe;
static int extra_fade;

static int albumtxt_x = 0;
static int albumtxt_dir = -1;
static int prev_center_index = -1;

static int start_index_track_list = 0;
static int track_list_visible_entries = 0;
static int track_list_y;
static int track_list_h;
static int track_scroll_index = 0;
static int track_scroll_dir = 1;

/*
    Proposals for transitions:

    pf_idle -> pf_scrolling : NEXT_ALBUM/PREV_ALBUM pressed
            -> pf_cover_in -> pf_show_tracks : SELECT_ALBUM clicked

    pf_scrolling -> pf_idle : NEXT_ALBUM/PREV_ALBUM released

    pf_show_tracks -> pf_cover_out -> pf_idle : SELECT_ALBUM pressed

    TODO:
    pf_show_tracks -> pf_cover_out -> pf_idle : MENU_PRESSED pressed
    pf_show_tracks -> play_track() -> exit() : SELECT_ALBUM pressed

    pf_idle, pf_scrolling -> show_menu(): MENU_PRESSED
*/
enum pf_states {
    pf_idle = 0,
    pf_scrolling,
    pf_cover_in,
    pf_show_tracks,
    pf_cover_out
};

static int pf_state;

/** code */

bool create_bmp(struct bitmap* input_bmp, void *buf, size_t buf_size,
                char *target_path);
int load_surface(int);

static inline PFreal fmul(PFreal a, PFreal b)
{
    return (a*b) >> PFREAL_SHIFT;
}

/* There are some precision issues when not using (long long) which in turn
   takes very long to compute... I guess the best solution would be to optimize
   the computations so it only requires a single long */
static inline PFreal fdiv(PFreal num, PFreal den)
{
    long long p = (long long) (num) << (PFREAL_SHIFT * 2);
    long long q = p / (long long) den;
    long long r = q >> PFREAL_SHIFT;

    return r;
}

#define fmin(a,b) (((a) < (b)) ? (a) : (b))
#define fmax(a,b) (((a) > (b)) ? (a) : (b))
#define fbound(min,val,max) (fmax((min),fmin((max),(val))))


#if 0
#define fmul(a,b) ( ((a)*(b)) >> PFREAL_SHIFT )
#define fdiv(n,m) ( ((n)<< PFREAL_SHIFT ) / m )

#define fconv(a, q1, q2) (((q2)>(q1)) ? (a)<<((q2)-(q1)) : (a)>>((q1)-(q2)))
#define tofloat(a, q) ( (float)(a) / (float)(1<<(q)) )

static inline PFreal fmul(PFreal a, PFreal b)
{
    return (a*b) >> PFREAL_SHIFT;
}

static inline PFreal fdiv(PFreal n, PFreal m)
{
    return (n<<(PFREAL_SHIFT))/m;
}
#endif

/* warning: regenerate the table if IANGLE_MAX and PFREAL_SHIFT are changed! */
static const PFreal sin_tab[] = {
    3,    103,    202,    300,    394,    485,    571,    652,
    726,    793,    853,    904,    947,    980,   1004,   1019,
    1023,   1018,   1003,    978,    944,    901,    849,    789,
    721,    647,    566,    479,    388,    294,    196,     97,
    -4,   -104,   -203,   -301,   -395,   -486,   -572,   -653,
    -727,   -794,   -854,   -905,   -948,   -981,  -1005,  -1020,
    -1024,  -1019,  -1004,   -979,   -945,   -902,   -850,   -790,
    -722,   -648,   -567,   -480,   -389,   -295,   -197,    -98,
    3
};

static inline PFreal fsin(int iangle)
{
    while(iangle < 0)
        iangle += IANGLE_MAX;
    iangle &= IANGLE_MASK;

    int i = (iangle >> 4);
    PFreal p = sin_tab[i];
    PFreal q = sin_tab[(i+1)];
    PFreal g = (q - p);
    return p + g * (iangle-i*16)/16;
}

static inline PFreal fcos(int iangle)
{
    return fsin(iangle + (IANGLE_MAX >> 2));
}

/**
  Create an index of all albums from the database.
  Also store the album names so we can access them later.
 */
int create_album_index(void)
{
    rb->memset(&tcs, 0, sizeof(struct tagcache_search) );
    album_count = 0;
    rb->tagcache_search(&tcs, tag_album);
    rb->tagcache_search_set_uniqbuf(&tcs, uniqbuf, UNIQBUF_SIZE);
    int l, old_l = 0;
    album[0].name_idx = 0;
    while (rb->tagcache_get_next(&tcs) && album_count  < MAX_ALBUMS)
    {
        l = rb->strlen(tcs.result) + 1;
        if ( album_count > 0 )
            album[album_count].name_idx = album[album_count-1].name_idx + old_l;

        if ( (album[album_count].name_idx + l) >
            MAX_ALBUMS*AVG_ALBUM_NAME_LENGTH )
            /* not enough memory */
            return ERROR_BUFFER_FULL;

        rb->strcpy(album_names + album[album_count].name_idx, tcs.result);
        album[album_count].seek = tcs.result_seek;
        old_l = l;
        album_count++;
    }
    rb->tagcache_search_finish(&tcs);

    return (album_count > 0) ? 0 : ERROR_NO_ALBUMS;
}

/**
 Return a pointer to the album name of the given slide_index
 */
char* get_album_name(const int slide_index)
{
    return album_names + album[slide_index].name_idx;
}

/**
 Return a pointer to the track name of the active album
 create_track_index has to be called first.
 */
char* get_track_name(const int track_index)
{
    if ( track_index < track_count )
        return track_names + tracks[track_index].name_idx;
    return 0;
}

/**
  Create the track index of the given slide_index.
 */
int create_track_index(const int slide_index)
{
    if ( slide_index == track_index ) {
        return -1;
    }

    if (!rb->tagcache_search(&tcs, tag_title))
        return -1;

    int ret = 0;
    char temp_titles[MAX_TRACKS][AVG_TRACK_NAME_LENGTH*4];
    int temp_seeks[MAX_TRACKS];

    rb->tagcache_search_add_filter(&tcs, tag_album, album[slide_index].seek);
    track_count=0;
    int string_index = 0;
    int l, track_num, heighest_index = 0;

    for(l=0;l<MAX_TRACKS;l++)
        temp_titles[l][0] = '\0';
    while (rb->tagcache_get_next(&tcs) && track_count  < MAX_TRACKS)
    {
        track_num = rb->tagcache_get_numeric(&tcs, tag_tracknumber) - 1;
        if (track_num >= 0)
        {
            rb->snprintf(temp_titles[track_num],sizeof(temp_titles[track_num]),
                         "%d:  %s", track_num+1, tcs.result);
            temp_seeks[track_num] = tcs.result_seek;
        }
        else
        {
            track_num = 0;
            while (temp_titles[track_num][0] != '\0')
                track_num++;
            rb->strcpy(temp_titles[track_num], tcs.result);
            temp_seeks[track_num] = tcs.result_seek;
        }
        if (track_num > heighest_index)
            heighest_index = track_num;
        track_count++;
    }

    rb->tagcache_search_finish(&tcs);
    track_index = slide_index;

    /* now fix the track list order */
    l = 0;
    track_count = 0;
    while (l <= heighest_index && 
           string_index <  MAX_TRACKS*AVG_TRACK_NAME_LENGTH)
    {
        if (temp_titles[l][0] != '\0')
        {
            rb->strcpy(track_names + string_index, temp_titles[l]);
            tracks[track_count].name_idx = string_index;
            tracks[track_count].seek = temp_seeks[l];
            string_index += rb->strlen(temp_titles[l]) + 1;
            track_count++;
        }
        l++;
    }
    if (ret != 0)
        return ret;
    else
        return (track_count > 0) ? 0 : -1;
}


/**
  Determine filename of the album art for the given slide_index and
  store the result in buf.
  The algorithm looks for the first track of the given album uses
  find_albumart to find the filename.
 */
bool get_albumart_for_index_from_db(const int slide_index, char *buf,
                                    int buflen)
{
    if ( slide_index == -1 )
    {
        rb->strncpy( buf, EMPTY_SLIDE, buflen );
    }

    if (!rb->tagcache_search(&tcs, tag_filename))
        return false;

    bool result;
    /* find the first track of the album */
    rb->tagcache_search_add_filter(&tcs, tag_album, album[slide_index].seek);

    if ( rb->tagcache_get_next(&tcs) ) {
        struct mp3entry id3;
        int fd;

        fd = rb->open(tcs.result, O_RDONLY);
        rb->get_metadata(&id3, fd, tcs.result);
        rb->close(fd);
        if ( rb->search_albumart_files(&id3, "", buf, buflen) )
            result = true;
        else
            result = false;
    }
    else {
        /* did not find a matching track */
        result = false;
    }
    rb->tagcache_search_finish(&tcs);
    return result;
}

/**
  Draw the PictureFlow logo
 */
void draw_splashscreen(void)
{
    struct screen* display = rb->screens[0];

    rb->lcd_set_background(N_BRIGHT(0));
    rb->lcd_set_foreground(N_BRIGHT(255));
    rb->lcd_clear_display();

    const struct picture* logo = &(logos[display->screen_type]);
    picture_draw(display, logo, (LCD_WIDTH - logo->width) / 2, 10);

    rb->lcd_update();
}


/**
  Draw a simple progress bar
 */
void draw_progressbar(int step)
{
    int txt_w, txt_h;
    const int bar_height = 22;
    const int w = LCD_WIDTH - 20;
    const int x = 10;

    rb->lcd_getstringsize("Preparing album artwork", &txt_w, &txt_h);

    int y = (LCD_HEIGHT - txt_h)/2;

    rb->lcd_putsxy((LCD_WIDTH - txt_w)/2, y, "Preparing album artwork");
    y += (txt_h + 5);

    rb->lcd_set_foreground(N_BRIGHT(100));
    rb->lcd_drawrect(x, y, w+2, bar_height);
    rb->lcd_set_foreground(N_PIX(165, 231, 82));

    rb->lcd_fillrect(x+1, y+1, step * w / album_count, bar_height-2);
    rb->lcd_set_foreground(N_BRIGHT(255));
    rb->lcd_update();
    rb->yield();
}

/**
 Precomupte the album art images and store them in CACHE_PREFIX.
 */
bool create_albumart_cache(bool force)
{
    number_of_slides  = album_count;
    int fh,ret;

    if ( ! force && rb->file_exists( CACHE_PREFIX "/ready" ) ) return true;

    int i, slides = 0;
    struct bitmap input_bmp;

    config.avg_album_width = 0;
    char pfraw_file[MAX_PATH];
    char albumart_file[MAX_PATH];
    unsigned int format = FORMAT_NATIVE;
    if (config.resize)
        format |= FORMAT_RESIZE|FORMAT_KEEP_ASPECT;
    for (i=0; i < album_count; i++)
    {
        rb->snprintf(pfraw_file, sizeof(pfraw_file), CACHE_PREFIX "/%d.pfraw",
                     i);
        /* delete existing cache, so it's a true rebuild */
        if(rb->file_exists(pfraw_file))
            rb->remove(pfraw_file);
        draw_progressbar(i);
        if (!get_albumart_for_index_from_db(i, albumart_file, MAX_PATH))
            continue;

        input_bmp.data = plugin_buf;
        input_bmp.width = DISPLAY_SIZE;
        input_bmp.height = DISPLAY_SIZE;
        ret = rb->read_bmp_file(albumart_file, &input_bmp,
                                plugin_buf_size, format, FPLUGIN);
        if (ret <= 0) {
            rb->splash(HZ, "Could not read bmp");
            continue; /* skip missing/broken files */
        }
        if (!create_bmp(&input_bmp, plugin_buf + WRNDUP(ret, 4),
                        plugin_buf_size - WRNDUP(ret, 4), pfraw_file))
        {
            rb->splash(HZ, "Could not write bmp");
        }
        config.avg_album_width += input_bmp.width;
        slides++;
        if ( rb->button_get(false) == PICTUREFLOW_MENU ) return false;
    }
    if ( slides == 0 ) {
        /* Warn the user that we couldn't find any albumart */
        rb->splash(2*HZ, "No album art found");
        return false;
    }
    config.avg_album_width /= slides;
    if ( config.avg_album_width == 0 ) {
        rb->splash(HZ, "album size is 0");
        return false;
    }
    fh = rb->creat( CACHE_PREFIX "/ready"  );
    rb->close(fh);
    return true;
}

/**
   Return the index on the stack of slide_index.
   Return -1 if slide_index is not on the stack.
 */
static inline int slide_stack_get_index(const int slide_index)
{
    int i = slide_cache_stack_index + 1;
    while (i--) {
        if ( slide_cache_stack[i] == slide_index ) return i;
    };
    return -1;
}

/**
   Push the slide_index on the stack so the image will be loaded.
   The algorithm tries to keep the center_index on top and the
   slide_index as high as possible (so second if center_index is
   on the stack).
 */
void slide_stack_push(const int slide_index)
{
    rb->mutex_lock(&slide_cache_stack_lock);

    if ( slide_cache_stack_index == -1 ) {
        /* empty stack, no checks at all */
        slide_cache_stack[ ++slide_cache_stack_index ] = slide_index;
        rb->mutex_unlock(&slide_cache_stack_lock);
        return;
    }

    int i = slide_stack_get_index( slide_index );

    if ( i == slide_cache_stack_index ) {
        /* slide_index is on top, so we do not change anything */
        rb->mutex_unlock(&slide_cache_stack_lock);
        return;
    }

    if ( i >= 0 ) {
        /* slide_index is already on the stack, but not on top */
        int tmp = slide_cache_stack[ slide_cache_stack_index  ];
        if ( tmp == center_index ) {
            /* the center_index is on top of the stack so do not touch that */
            if ( slide_cache_stack_index  > 0 ) {
                /* 
                   but maybe it is possible to swap the given slide_index to
                   the second place
                */
                tmp = slide_cache_stack[slide_cache_stack_index - 1];
                slide_cache_stack[slide_cache_stack_index - 1] = 
                    slide_cache_stack[i];
                slide_cache_stack[i] = tmp;
            }
        }
        else {
            /* 
               if the center_index is not on top (i.e. already loaded) bring
               the slide_index to the top
            */
            slide_cache_stack[slide_cache_stack_index] = slide_cache_stack[i];
            slide_cache_stack[i] = tmp;
        }
    }
    else {
        /* slide_index is not on the stack */
        if ( slide_cache_stack_index >= SLIDE_CACHE_SIZE-1 ) {
            /*
               if we exceeded the stack size, clear the first half of the
               stack
            */
            slide_cache_stack_index = SLIDE_CACHE_SIZE/2;
            for (i = 0; i <= slide_cache_stack_index ; i++)
                slide_cache_stack[i] = slide_cache_stack[i +
                    slide_cache_stack_index];
        }
        if ( slide_cache_stack[ slide_cache_stack_index ] == center_index ) {
            /* if the center_index is on top leave it there */
            slide_cache_stack[ slide_cache_stack_index ] = slide_index;
            slide_cache_stack[ ++slide_cache_stack_index ] = center_index;
        }
        else {
            /* usual stack case: push the slide_index on top */
            slide_cache_stack[ ++slide_cache_stack_index ] = slide_index;
        }
    }
    rb->mutex_unlock(&slide_cache_stack_lock);
}


/**
  Pop the topmost item from the stack and decrease the stack size
 */
static inline int slide_stack_pop(void)
{
    rb->mutex_lock(&slide_cache_stack_lock);
    int result;
    if ( slide_cache_stack_index >= 0 )
        result = slide_cache_stack[ slide_cache_stack_index-- ];
    else
        result = -1;
    rb->mutex_unlock(&slide_cache_stack_lock);
    return result;
}


/**
  Load the slide into the cache.
  Thus we have to queue the loading request in our thread while discarding the
  oldest slide.
 */
static inline void request_surface(const int slide_index)
{
    slide_stack_push(slide_index);
    rb->queue_post(&thread_q, EV_WAKEUP, 0);
}


/**
 Thread used for loading and preparing bitmaps in the background
 */
void thread(void)
{
    long sleep_time = 5 * HZ;
    struct queue_event ev;
    while (1) {
        rb->queue_wait_w_tmo(&thread_q, &ev, sleep_time);
        switch (ev.id) {
            case EV_EXIT:
                return;
            case EV_WAKEUP:
                /* we just woke up */
                break;
        }
        int slide_index;
        while ( (slide_index = slide_stack_pop()) != -1 ) {
            load_surface( slide_index );
            rb->queue_wait_w_tmo(&thread_q, &ev, HZ/10);
            switch (ev.id) {
                case EV_EXIT:
                    return;
            }
        }
    }
}


/**
 End the thread by posting the EV_EXIT event
 */
void end_pf_thread(void)
{
    if ( thread_is_running ) {
        rb->queue_post(&thread_q, EV_EXIT, 0);
        rb->thread_wait(thread_id);
        /* remove the thread's queue from the broadcast list */
        rb->queue_delete(&thread_q);
        thread_is_running = false;
    }

}


/**
 Create the thread an setup the event queue
 */
bool create_pf_thread(void)
{
    /* put the thread's queue in the bcast list */
    rb->queue_init(&thread_q, true);
    if ((thread_id = rb->create_thread(
                           thread,
                           thread_stack,
                           sizeof(thread_stack),
                            0,
                           "Picture load thread"
                               IF_PRIO(, PRIORITY_BACKGROUND)
                               IF_COP(, CPU)
                                      )
        ) == 0) {
        return false;
    }
    thread_is_running = true;
    return true;
}

/**
 Safe the given bitmap as filename in the pfraw format
 */
bool save_pfraw(char* filename, struct bitmap *bm)
{
    struct pfraw_header bmph;
    bmph.width = bm->width;
    bmph.height = bm->height;
    int fh = rb->creat( filename );
    if( fh < 0 ) return false;
    rb->write( fh, &bmph, sizeof( struct pfraw_header ) );
    int y;
    for( y = 0; y < bm->height; y++ )
    {
        pix_t *d = (pix_t*)( bm->data ) + (y*bm->width);
        rb->write( fh, d, sizeof( pix_t ) * bm->width );
    }
    rb->close( fh );
    return true;
}


/**
 Read the pfraw image given as filename and return the hid of the buffer
 */
int read_pfraw(char* filename)
{
    struct pfraw_header bmph;
    int fh = rb->open(filename, O_RDONLY);
    rb->read(fh, &bmph, sizeof(struct pfraw_header));
    if( fh < 0 ) {
        return empty_slide_hid;
    }

    int size =  sizeof(struct bitmap) + sizeof( pix_t ) * 
                bmph.width * bmph.height;

    int hid = rb->bufalloc(NULL, size, TYPE_BITMAP);
    if (hid < 0) {
        rb->close( fh );
        return -1;
    }

    struct bitmap *bm;
    if (rb->bufgetdata(hid, 0, (void *)&bm) < size) {
        rb->close( fh );
        return -1;
    }

    bm->width = bmph.width;
    bm->height = bmph.height;
    bm->format = FORMAT_NATIVE;
    bm->data = ((unsigned char *)bm + sizeof(struct bitmap));

    int y;
    for( y = 0; y < bm->height; y++ )
    {
        pix_t *d = (pix_t*)( bm->data ) + (y*bm->width);
        rb->read( fh, d , sizeof( pix_t ) * bm->width );
    }
    rb->close( fh );
    return hid;
}


/**
  Create the slide with its reflection for the given slide_index and filename
  and store it as pfraw in CACHE_PREFIX/[slide_index].pfraw
 */
bool create_bmp(struct bitmap *input_bmp, void * buffer, size_t buffer_len,
                char *target_path)
{
    struct bitmap output_bmp;

    output_bmp.format = input_bmp->format;
    output_bmp.data = (char *)buffer;

    output_bmp.width = 2 * DISPLAY_SIZE;
    output_bmp.height = input_bmp->width;

    pix_t *src = (pix_t *)input_bmp->data;
    pix_t *dst = (pix_t *)output_bmp.data;
    if (sizeof(pix_t) * output_bmp.width * output_bmp.height > buffer_len)
        return 0;
    /* transpose the image, this is to speed-up the rendering
       because we process one column at a time
       (and much better and faster to work row-wise, i.e in one scanline) */
    int hofs = REFLECT_TOP - input_bmp->height;
    rb->memset(dst, 0, sizeof(pix_t) * output_bmp.width * output_bmp.height);
    int x, y;
    for (x = 0; x < input_bmp->width; x++)
        for (y = 0; y < input_bmp->height; y++)
            dst[output_bmp.width * x + (hofs + y)] =
                    src[y * input_bmp->width + x];

    /* create the reflection */
    for (y = 0; y < REFLECT_HEIGHT; y++) {
        uint32_t sc = (REFLECT_HEIGHT - y) * REFLECT_SC;
        for (x = 0; x < input_bmp->width; x++) {
            pix_t color = src[x + input_bmp->width * 
                              (input_bmp->height - y - 1)];
#ifdef USEGSLIB
            pix_t out = color * sc >> 16;
#else
            int r = RGB_UNPACK_RED(color) * sc >> 16;
            int g = RGB_UNPACK_GREEN(color) * sc >> 16;
            int b = RGB_UNPACK_BLUE(color) * sc >> 16;
            pix_t out = LCD_RGBPACK(r, g, b);
#endif
            dst[input_bmp->height + hofs + y + output_bmp.width * x] = out;
        }
    }
    return save_pfraw(target_path, &output_bmp);
}


/**
  Load the surface for the given slide_index into the cache at cache_index.
 */
static inline bool load_and_prepare_surface(const int slide_index,
                                            const int cache_index)
{
    char tmp_path_name[MAX_PATH+1];
    rb->snprintf(tmp_path_name, sizeof(tmp_path_name), CACHE_PREFIX "/%d.pfraw",
                 slide_index);

    int hid = read_pfraw(tmp_path_name);
    if (hid < 0)
        return false;

    cache[cache_index].hid = hid;

    if ( cache_index < SLIDE_CACHE_SIZE ) {
        cache[cache_index].index = slide_index;
        cache[cache_index].touched = *rb->current_tick;
    }

    return true;
}


/**
 Load the surface from a bmp and overwrite the oldest slide in the cache
 if necessary.
 */
int load_surface(const int slide_index)
{
    long oldest_tick = *rb->current_tick;
    int oldest_slide = 0;
    int i;
    if ( slide_cache_in_use < SLIDE_CACHE_SIZE ) { /* initial fill */
        oldest_slide = slide_cache_in_use;
        load_and_prepare_surface(slide_index, slide_cache_in_use++);
    }
    else {
        for (i = 0; i < SLIDE_CACHE_SIZE; i++) { /* look for oldest slide */
            if (cache[i].touched < oldest_tick) {
                oldest_slide = i;
                oldest_tick = cache[i].touched;
            }
        }
        if (cache[oldest_slide].hid != empty_slide_hid) {
            rb->bufclose(cache[oldest_slide].hid);
            cache[oldest_slide].hid = -1;
        }
        load_and_prepare_surface(slide_index, oldest_slide);
    }
    return oldest_slide;
}


/**
  Get a slide from the buffer
 */
static inline struct bitmap *get_slide(const int hid)
{
    if (hid < 0)
        return NULL;

    struct bitmap *bmp;

    ssize_t ret = rb->bufgetdata(hid, 0, (void *)&bmp);
    if (ret < 0)
        return NULL;

    return bmp;
}


/**
 Return the requested surface
*/
static inline struct bitmap *surface(const int slide_index)
{
    if (slide_index < 0)
        return 0;
    if (slide_index >= number_of_slides)
        return 0;

    int i;
    for (i = 0; i < slide_cache_in_use; i++) {
        /* maybe do the inverse mapping => implies dynamic allocation? */
        if ( cache[i].index == slide_index ) {
            /* We have already loaded our slide, so touch it and return it. */
            cache[i].touched = *rb->current_tick;
            return get_slide(cache[i].hid);
        }
    }
    request_surface(slide_index);
    return get_slide(empty_slide_hid);
}

/**
 adjust slides so that they are in "steady state" position
 */
void reset_slides(void)
{
    center_slide.angle = 0;
    center_slide.cx = 0;
    center_slide.cy = 0;
    center_slide.distance = 0;
    center_slide.slide_index = center_index;

    int i;
    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx = -(offsetX + config.spacing_between_slides * i * PFREAL_ONE);
        si->cy = offsetY;
        si->slide_index = center_index - 1 - i;
        si->distance = 0;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx = offsetX + config.spacing_between_slides * i * PFREAL_ONE;
        si->cy = offsetY;
        si->slide_index = center_index + 1 + i;
        si->distance = 0;
    }
}


/**
 Updates look-up table and other stuff necessary for the rendering.
 Call this when the viewport size or slide dimension is changed.
 */
void recalc_table(void)
{
    int w = (LCD_WIDTH + 1) / 2;
    int h = (LCD_HEIGHT + 1) / 2;
    int i;
    for (i = 0; i < w; i++) {
        PFreal gg = (PFREAL_HALF + i * PFREAL_ONE) / (2 * h);
        rays[w - i - 1] = -gg;
        rays[w + i] = gg;
    }

    itilt = 70 * IANGLE_MAX / 360;      /* approx. 70 degrees tilted */

    offsetX = DISPLAY_SIZE / 2 * (fsin(itilt) + PFREAL_ONE);
    offsetY = DISPLAY_SIZE / 2 * (fsin(itilt) + PFREAL_ONE / 2);
    offsetX += config.extra_spacing_for_center_slide << PFREAL_SHIFT;
}


/**
   Fade the given color by spreading the fb_data (ushort)
   to an uint, multiply and compress the result back to a ushort.
 */
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
static inline pix_t fade_color(pix_t c, unsigned int a)
{
    c = swap16(c);
    unsigned int p = ((((c|(c<<16)) & 0x07e0f81f) * a) >> 5) & 0x07e0f81f;
    return swap16( (pix_t) (p | ( p >> 16 )) );
}
#elif LCD_PIXELFORMAT == RGB565
static inline pix_t fade_color(pix_t c, unsigned int a)
{
    unsigned int p = ((((c|(c<<16)) & 0x07e0f81f) * a) >> 5) & 0x07e0f81f;
    return (p | ( p >> 16 ));
}
#else
static inline pix_t fade_color(pix_t c, unsigned int a)
{
    return (unsigned int)c * a >> 8;
}
#endif

/**
  Render a single slide
*/
void render_slide(struct slide_data *slide, struct rect *result_rect,
                  const int alpha, int col1, int col2)
{
    rb->memset(result_rect, 0, sizeof(struct rect));
    struct bitmap *bmp = surface(slide->slide_index);
    if (!bmp) {
        return;
    }
    pix_t *src = (pix_t *)bmp->data;

    const int sw = bmp->height;
    const int sh = bmp->width;

    const int h = LCD_HEIGHT;
    const int w = LCD_WIDTH;

    if (col1 > col2) {
        int c = col2;
        col2 = col1;
        col1 = c;
    }

    col1 = (col1 >= 0) ? col1 : 0;
    col2 = (col2 >= 0) ? col2 : w - 1;
    col1 = fmin(col1, w - 1);
    col2 = fmin(col2, w - 1);

    int distance = (h + slide->distance) * 100 / config.zoom;
    if (distance < 100 ) distance = 100; /* clamp distances */
    PFreal sdx = fcos(slide->angle);
    PFreal sdy = fsin(slide->angle);
    PFreal xs = slide->cx - bmp->width * sdx / 4;
    PFreal ys = slide->cy - bmp->width * sdy / 4;
    PFreal dist = distance * PFREAL_ONE;

#ifdef USEGSLIB
    const int alphasc = alpha;
#else
    const int alphasc = alpha >> 3;
#endif

    int xi = fmax((PFreal) 0,
                  ((w * PFREAL_ONE / 2) +
                   fdiv(xs * h, dist + ys)) >> PFREAL_SHIFT);
    if (xi >= w) {
        return;
    }

    bool flag = false;
    result_rect->left = xi;
    int x;
    for (x = fmax(xi, col1); x <= col2; x++) {
        PFreal hity = 0;
        PFreal fk = rays[x];
        if (sdy) {
            fk = fk - fdiv(sdx, sdy);
            if (fk)
                hity = -fdiv(( rays[x] * distance
                       - slide->cx
                       + slide->cy * sdx / sdy), fk);
        }

        dist = distance * PFREAL_ONE + hity;
        if (dist < 0)
            continue;

        PFreal hitx = fmul(dist, rays[x]);

        PFreal hitdist = fdiv(hitx - slide->cx, sdx);

        const int column = (sw >> 1) + (hitdist >> PFREAL_SHIFT);
        if (column >= sw)
            break;

        if (column < 0)
            continue;

        result_rect->right = x;
        if (!flag)
            result_rect->left = x;
        flag = true;

        int y1 = (h >> 1);
        int y2 = y1 + 1;
        pix_t *pixel1 = &buffer[y1 * BUFFER_WIDTH + x];
        pix_t *pixel2 = &buffer[y2 * BUFFER_WIDTH + x];
        const int pixelstep = pixel2 - pixel1;

        int center = (sh >> 1);
        int dy = dist / h;
        int p1 = center * PFREAL_ONE - (dy >> 2);
        int p2 = center * PFREAL_ONE + (dy >> 2);

        const pix_t *ptr = &src[column * bmp->width];

        if (alpha == 256)
            while ((y1 >= 0) && (y2 < h) && (p1 >= 0)) {
                *pixel1 = ptr[p1 >> PFREAL_SHIFT];
                *pixel2 = ptr[p2 >> PFREAL_SHIFT];
                p1 -= dy;
                p2 += dy;
                y1--;
                y2++;
                pixel1 -= pixelstep;
                pixel2 += pixelstep;
        } else
            while ((y1 >= 0) && (y2 < h) && (p1 >= 0)) {
                *pixel1 = fade_color(ptr[p1 >> PFREAL_SHIFT], alphasc);
                *pixel2 = fade_color(ptr[p2 >> PFREAL_SHIFT], alphasc);
                p1 -= dy;
                p2 += dy;
                y1--;
                y2++;
                pixel1 -= pixelstep;
                pixel2 += pixelstep;
            }
    }
    /* let the music play... */
    rb->yield();

    result_rect->top = 0;
    result_rect->bottom = h - 1;
    return;
}


/**
  Jump the the given slide_index
 */
static inline void set_current_slide(const int slide_index)
{
    step = 0;
    center_index = fbound(slide_index, 0, number_of_slides - 1);
    target = center_index;
    slide_frame = slide_index << 16;
    reset_slides();
}

/**
  Start the animation for changing slides
 */
void start_animation(void)
{
    step = (target < center_slide.slide_index) ? -1 : 1;
    pf_state = pf_scrolling;
}

/**
  Go to the previous slide
 */
void show_previous_slide(void)
{
    if (step == 0) {
        if (center_index > 0) {
            target = center_index - 1;
            start_animation();
        }
    } else if ( step > 0 ) {
        target = center_index;
        start_animation();
    } else {
        target = fmax(0, center_index - 2);
    }
}


/**
  Go to the next slide
 */
void show_next_slide(void)
{
    if (step == 0) {
        if (center_index < number_of_slides - 1) {
            target = center_index + 1;
            start_animation();
        }
    } else if ( step < 0 ) {
        target = center_index;
        start_animation();
    } else {
        target = fmin(center_index + 2, number_of_slides - 1);
    }
}


/**
  Return true if the rect has size 0
*/
static inline bool is_empty_rect(struct rect *r)
{
    return ((r->left == 0) && (r->right == 0) && (r->top == 0)
            && (r->bottom == 0));
}


/**
  Render the slides. Updates only the offscreen buffer.
*/
void render_all_slides(void)
{
    MYLCD(set_background)(G_BRIGHT(0));
    /* TODO: Optimizes this by e.g. invalidating rects */
    MYLCD(clear_display)();

    int nleft = config.show_slides;
    int nright = config.show_slides;

    struct rect r;
    r.left = LCD_WIDTH; r.top = 0; r.bottom = 0; r.right = 0;
    render_slide(&center_slide, &r, 256, -1, -1);
#ifdef DEBUG_DRAW
    MYLCD(drawrect)(r.left, r.top, r.right - r.left, r.bottom - r.top);
#endif
    int c1 = r.left;
    int c2 = r.right;
    int index;
    if (step == 0) {
        /* no animation, boring plain rendering */
        for (index = 0; index < nleft - 1; index++) {
            int alpha = (index < nleft - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha < 0 ) alpha = 0;
            render_slide(&left_slides[index], &r, alpha, 0, c1 - 1);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                MYLCD->(drawrect)(r.left, r.top, r.right - r.left,
                                  r.bottom - r.top);
#endif
                c1 = r.left;
            }
        }
        for (index = 0; index < nright - 1; index++) {
            int alpha = (index < nright - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha < 0 ) alpha = 0;
            render_slide(&right_slides[index], &r, alpha, c2 + 1,
                         LCD_WIDTH);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                MYLCD(drawrect)(r.left, r.top, r.right - r.left,
                                r.bottom - r.top);
#endif
                c2 = r.right;
            }
        }
    } else {
        if ( step < 0 ) c1 = LCD_WIDTH;
        /* the first and last slide must fade in/fade out */
        for (index = 0; index < nleft; index++) {
            int alpha = 256;
            if (index == nleft - 1)
                alpha = (step > 0) ? 0 : 128 - fade / 2;
            if (index == nleft - 2)
                alpha = (step > 0) ? 128 - fade / 2 : 256 - fade / 2;
            if (index == nleft - 3)
                alpha = (step > 0) ? 256 - fade / 2 : 256;
            render_slide(&left_slides[index], &r, alpha, 0, c1 - 1);

            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                MYLCD(drawrect)(r.left, r.top, r.right - r.left,
                                r.bottom - r.top);
#endif
                c1 = r.left;
            }
        }
        if ( step > 0 ) c2 = 0;
        for (index = 0; index < nright; index++) {
            int alpha = (index < nright - 2) ? 256 : 128;
            if (index == nright - 1)
                alpha = (step > 0) ? fade / 2 : 0;
            if (index == nright - 2)
                alpha = (step > 0) ? 128 + fade / 2 : fade / 2;
            if (index == nright - 3)
                alpha = (step > 0) ? 256 : 128 + fade / 2;
            render_slide(&right_slides[index], &r, alpha, c2 + 1,
                         LCD_WIDTH);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                MYLCD(drawrect)(r.left, r.top, r.right - r.left,
                                r.bottom - r.top);
#endif
                c2 = r.right;
            }
        }
    }
}


/**
  Updates the animation effect. Call this periodically from a timer.
*/
void update_scroll_animation(void)
{
    if (step == 0)
        return;

    int speed = 16384;
    int i;

    /* deaccelerate when approaching the target */
    if (true) {
        const int max = 2 * 65536;

        int fi = slide_frame;
        fi -= (target << 16);
        if (fi < 0)
            fi = -fi;
        fi = fmin(fi, max);

        int ia = IANGLE_MAX * (fi - max / 2) / (max * 2);
        speed = 512 + 16384 * (PFREAL_ONE + fsin(ia)) / PFREAL_ONE;
    }

    slide_frame += speed * step;

    int index = slide_frame >> 16;
    int pos = slide_frame & 0xffff;
    int neg = 65536 - pos;
    int tick = (step < 0) ? neg : pos;
    PFreal ftick = (tick * PFREAL_ONE) >> 16;

    /* the leftmost and rightmost slide must fade away */
    fade = pos / 256;

    if (step < 0)
        index++;
    if (center_index != index) {
        center_index = index;
        slide_frame = index << 16;
        center_slide.slide_index = center_index;
        for (i = 0; i < config.show_slides; i++)
            left_slides[i].slide_index = center_index - 1 - i;
        for (i = 0; i < config.show_slides; i++)
            right_slides[i].slide_index = center_index + 1 + i;
    }

    center_slide.angle = (step * tick * itilt) >> 16;
    center_slide.cx = -step * fmul(offsetX, ftick);
    center_slide.cy = fmul(offsetY, ftick);

    if (center_index == target) {
        reset_slides();
        pf_state = pf_idle;
        step = 0;
        fade = 256;
        return;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx =
            -(offsetX + config.spacing_between_slides * i * PFREAL_ONE + step
                        * config.spacing_between_slides * ftick);
        si->cy = offsetY;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx =
            offsetX + config.spacing_between_slides * i * PFREAL_ONE - step
                      * config.spacing_between_slides * ftick;
        si->cy = offsetY;
    }

    if (step > 0) {
        PFreal ftick = (neg * PFREAL_ONE) >> 16;
        right_slides[0].angle = -(neg * itilt) >> 16;
        right_slides[0].cx = fmul(offsetX, ftick);
        right_slides[0].cy = fmul(offsetY, ftick);
    } else {
        PFreal ftick = (pos * PFREAL_ONE) >> 16;
        left_slides[0].angle = (pos * itilt) >> 16;
        left_slides[0].cx = -fmul(offsetX, ftick);
        left_slides[0].cy = fmul(offsetY, ftick);
    }

    /* must change direction ? */
    if (target < index)
        if (step > 0)
            step = -1;
    if (target > index)
        if (step < 0)
            step = 1;
}


/**
  Cleanup the plugin
*/
void cleanup(void *parameter)
{
    (void) parameter;
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    int i;
    for (i = 0; i < slide_cache_in_use; i++) {
        rb->bufclose(cache[i].hid);
    }
    if ( empty_slide_hid != - 1)
        rb->bufclose(empty_slide_hid);
#ifdef USEGSLIB
    grey_release();
#endif
    rb->lcd_set_drawmode(old_drawmode);
}

/**
  Create the "?" slide, that is shown while loading
  or when no cover was found.
 */
int create_empty_slide(bool force)
{
    if ( force || ! rb->file_exists( EMPTY_SLIDE ) )  {
        struct bitmap input_bmp;
        int ret;
        input_bmp.width = DISPLAY_SIZE;
        input_bmp.height = DISPLAY_SIZE;
        input_bmp.format = FORMAT_NATIVE;
        input_bmp.data = (char*)plugin_buf;
        ret = rb->read_bmp_file(EMPTY_SLIDE_BMP, &input_bmp,
                                plugin_buf_size,
                                FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                                FPLUGIN);
        if ( ! create_bmp(&input_bmp, plugin_buf + WRNDUP(ret, 4),
                          plugin_buf_size - WRNDUP(ret, 4), EMPTY_SLIDE) )
            return false;
    }

    empty_slide_hid = read_pfraw( EMPTY_SLIDE );
    if (empty_slide_hid == -1 ) return false;

    return true;
}

/**
    Shows the album name setting menu
*/
int album_name_menu(void)
{
    int selection = config.show_album_name;

    MENUITEM_STRINGLIST(album_name_menu,"Show album title",NULL,
            "Hide album title", "Show at the bottom", "Show at the top");
    rb->do_menu(&album_name_menu, &selection, NULL, false);

    config.show_album_name = selection;
    return GO_TO_PREVIOUS;
}

/**
  Shows the settings menu
 */
int settings_menu(void)
{
    int selection = 0;
    bool old_val;

    MENUITEM_STRINGLIST(settings_menu, "PictureFlow Settings", NULL, "Show FPS",
                        "Spacing", "Center margin", "Number of slides", "Zoom",
                        "Show album title", "Resize Covers", "Rebuild cache");

    do {
        selection=rb->do_menu(&settings_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                rb->set_bool("Show FPS", &(config.show_fps));
                reset_track_list();
                break;

            case 1:
                rb->set_int("Spacing between slides", "", 1,
                            &(config.spacing_between_slides),
                            NULL, 1, 0, 100, NULL );
                recalc_table();
                reset_slides();
                break;

            case 2:
                rb->set_int("Center margin", "", 1,
                            &(config.extra_spacing_for_center_slide),
                            NULL, 1, -70, 70, NULL );
                recalc_table();
                reset_slides();
                break;

            case 3:
                rb->set_int("Number of slides", "", 1, &(config.show_slides),
                            NULL, 1, 1, MAX_SLIDES_COUNT, NULL );
                recalc_table();
                reset_slides();
                break;

            case 4:
                rb->set_int("Zoom", "", 1, &(config.zoom),
                            NULL, 1, 10, 300, NULL );
                recalc_table();
                reset_slides();
                break;
            case 5:
                album_name_menu();
                reset_track_list();
                recalc_table();
                reset_slides();
                break;
            case 6:
                old_val = config.resize;
                rb->set_bool("Resize Covers", &(config.resize));
                if (old_val == config.resize) /* changed? */
                    break;
                /* fallthrough if changed, since cache needs to be rebuilt */
            case 7:
                rb->remove(CACHE_PREFIX "/ready");
                rb->remove(EMPTY_SLIDE);
                rb->splash(HZ, "Cache will be rebuilt on next restart");
                break;

            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
        }
    } while ( selection >= 0 );
    return 0;
}

/**
  Show the main menu
 */
int main_menu(void)
{
    int selection = 0;
    int result;

    rb->lcd_set_foreground(N_BRIGHT(255));

    MENUITEM_STRINGLIST(main_menu,"PictureFlow Main Menu",NULL,
                        "Settings", "Return", "Quit");
    while (1)  {
        switch (rb->do_menu(&main_menu,&selection, NULL, false)) {
            case 0:
                result = settings_menu();
                if ( result != 0 ) return result;
                break;

            case 1:
                return 0;

            case 2:
                return -1;

            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;

            default:
                return 0;
        }
    }
}

/**
  Fill the config struct with some defaults
 */
void set_default_config(void)
{
    config.spacing_between_slides = (LCD_WIDTH - DISPLAY_SIZE) / 8;
    config.extra_spacing_for_center_slide = (LCD_WIDTH - DISPLAY_SIZE) / 16;
    config.show_slides = 4;
    config.avg_album_width = 0;
    config.zoom = 100;
    config.show_fps = false;
    config.resize = true;
    config.show_album_name = album_name_top;
}

/**
  Read the config file.
  For now, the size has to match.
  Later a version number might be appropiate.
 */
bool read_pfconfig(void)
{
    set_default_config();
    /* defaults */
    int fh = rb->open( CONFIG_FILE, O_RDONLY );
    if ( fh < 0 ) { /* no config yet */
        return true;
    }
    int ret = rb->read(fh, &config, sizeof(struct config_data));
    rb->close(fh);
    if ( ret != sizeof(struct config_data) ) {
        set_default_config();
        rb->splash(2*HZ, "Config invalid. Using defaults");
    }
    return true;
}

/**
  Write the config file
 */
bool write_pfconfig(void)
{
    int fh = rb->creat( CONFIG_FILE );
    if( fh < 0 ) return false;
    rb->write( fh, &config, sizeof( struct config_data ) );
    rb->close( fh );
    return true;
}

/**
   Animation step for zooming into the current cover
 */
void update_cover_in_animation(void)
{
    cover_animation_keyframe++;
    if( cover_animation_keyframe < 20 ) {
        center_slide.distance-=5;
        center_slide.angle+=1;
        extra_fade += 13;
    }
    else if( cover_animation_keyframe < 35 ) {
        center_slide.angle+=16;
    }
    else {
        cover_animation_keyframe = 0;
        pf_state = pf_show_tracks;
    }
}

/**
   Animation step for zooming out the current cover
 */
void update_cover_out_animation(void)
{
    cover_animation_keyframe++;
    if( cover_animation_keyframe <= 15 ) {
        center_slide.angle-=16;
    }
    else if( cover_animation_keyframe < 35 ) {
        center_slide.distance+=5;
        center_slide.angle-=1;
        extra_fade -= 13;
    }
    else {
        cover_animation_keyframe = 0;
        pf_state = pf_idle;
    }
}

/**
   Draw a blue gradient at y with height h
 */
static inline void draw_gradient(int y, int h)
{
    static int r, inc, c;
    inc = (100 << 8) / h;
    c = 0;
    selected_track_pulse = (selected_track_pulse+1) % 10;
    int c2 = selected_track_pulse - 5;
    for (r=0; r<h; r++) {
        MYLCD(set_foreground)(G_PIX(c2+80-(c >> 9), c2+100-(c >> 9),
                                           c2+250-(c >> 8)));
        MYLCD(hline)(0, LCD_WIDTH, r+y);
        if ( r > h/2 )
            c-=inc;
        else
            c+=inc;
    }
}


static void track_list_yh(int char_height)
{
    switch (config.show_album_name)
    {
        case album_name_hide:
            track_list_y = (config.show_fps ? char_height : 0);
            track_list_h = LCD_HEIGHT - track_list_y;
            break;
        case album_name_bottom:
            track_list_y = (config.show_fps ? char_height : 0);
            track_list_h = LCD_HEIGHT - track_list_y - char_height * 2;
            break;
        default: /* case album_name_top */
            track_list_y = char_height * 2;
            track_list_h = LCD_HEIGHT - track_list_y -
                           (config.show_fps ? char_height : 0);
            break;
    }
}

/**
    Reset the track list after a album change
 */
void reset_track_list(void)
{
    int albumtxt_h = rb->screens[SCREEN_MAIN]->getcharheight();
    track_list_yh(albumtxt_h);
    track_list_visible_entries = fmin( track_list_h/albumtxt_h , track_count );
    start_index_track_list = 0;
    track_scroll_index = 0;
    track_scroll_dir = 1;
    selected_track = 0;

    /* let the tracklist start more centered
     * if the screen isn't filled with tracks */
    if (track_count*albumtxt_h < track_list_h)
    {
        track_list_h = track_count * albumtxt_h;
        track_list_y = LCD_HEIGHT / 2  - (track_list_h / 2);
    }
}

/**
  Display the list of tracks
 */
void show_track_list(void)
{
    MYLCD(clear_display)();
    if ( center_slide.slide_index != track_index ) {
        create_track_index(center_slide.slide_index);
        reset_track_list();
    }
    static int titletxt_w, titletxt_x, color, titletxt_h;
    titletxt_h = rb->screens[SCREEN_MAIN]->getcharheight();

    int titletxt_y = track_list_y;
    int track_i;
    track_i = start_index_track_list;
    for (;track_i < track_list_visible_entries+start_index_track_list;
         track_i++)
    {
        MYLCD(getstringsize)(get_track_name(track_i), &titletxt_w, NULL);
        titletxt_x = (LCD_WIDTH-titletxt_w)/2;
        if ( track_i == selected_track ) {
            draw_gradient(titletxt_y, titletxt_h);
            MYLCD(set_foreground)(G_BRIGHT(255));
            if (titletxt_w > LCD_WIDTH ) {
                if ( titletxt_w + track_scroll_index <= LCD_WIDTH )
                    track_scroll_dir = 1;
                else if ( track_scroll_index >= 0 ) track_scroll_dir = -1;
                track_scroll_index += track_scroll_dir*2;
                titletxt_x = track_scroll_index;
            }
            MYLCD(putsxy)(titletxt_x,titletxt_y,get_track_name(track_i));
        }
        else {
            color = 250 - (abs(selected_track - track_i) * 200 / track_count);
            MYLCD(set_foreground)(G_BRIGHT(color));
            MYLCD(putsxy)(titletxt_x,titletxt_y,get_track_name(track_i));
        }
        titletxt_y += titletxt_h;
    }
}

void select_next_track(void)
{
    if (  selected_track < track_count - 1 ) {
        selected_track++;
        track_scroll_index = 0;
        track_scroll_dir = 1;
        if (selected_track==(track_list_visible_entries+start_index_track_list))
            start_index_track_list++;
    }
}

void select_prev_track(void)
{
    if (selected_track > 0 ) {
        if (selected_track==start_index_track_list) start_index_track_list--;
        track_scroll_index = 0;
        track_scroll_dir = 1;
        selected_track--;
    }
}

/**
   Draw the current album name
 */
void draw_album_text(void)
{
    if (0 == config.show_album_name)
        return;

    int albumtxt_w, albumtxt_h;
    int albumtxt_y = 0;

    char *albumtxt;
    int c;
    /* Draw album text */
    if ( pf_state == pf_scrolling ) {
        c = ((slide_frame & 0xffff )/ 255);
        if (step < 0) c = 255-c;
        if (c > 128 ) { /* half way to next slide .. still not perfect! */
            albumtxt = get_album_name(center_index+step);
            c = (c-128)*2;
        }
        else {
            albumtxt = get_album_name(center_index);
            c = (128-c)*2;
        }
    }
    else {
        c= 255;
        albumtxt = get_album_name(center_index);
    }

    MYLCD(set_foreground)(G_BRIGHT(c));
    MYLCD(getstringsize)(albumtxt, &albumtxt_w, &albumtxt_h);
    if (center_index != prev_center_index) {
        albumtxt_x = 0;
        albumtxt_dir = -1;
        prev_center_index = center_index;
    }

    if (config.show_album_name == album_name_top)
        albumtxt_y = albumtxt_h / 2;
    else
        albumtxt_y = LCD_HEIGHT - albumtxt_h - albumtxt_h/2;

    if (albumtxt_w > LCD_WIDTH ) {
        MYLCD(putsxy)(albumtxt_x, albumtxt_y , albumtxt);
        if ( pf_state == pf_idle || pf_state == pf_show_tracks ) {
            if ( albumtxt_w + albumtxt_x <= LCD_WIDTH ) albumtxt_dir = 1;
            else if ( albumtxt_x >= 0 ) albumtxt_dir = -1;
            albumtxt_x += albumtxt_dir;
        }
    }
    else {
        MYLCD(putsxy)((LCD_WIDTH - albumtxt_w) /2, albumtxt_y , albumtxt);
    }


}


/**
  Main function that also contain the main plasma
  algorithm.
 */
int main(void)
{
    int ret;
    draw_splashscreen();

    if ( ! rb->dir_exists( CACHE_PREFIX ) ) {
        if ( rb->mkdir( CACHE_PREFIX ) < 0 ) {
            rb->splash(HZ, "Could not create directory " CACHE_PREFIX );
            return PLUGIN_ERROR;
        }
    }

    if (!read_pfconfig()) {
        rb->splash(HZ, "Error in config. Please delete " CONFIG_FILE);
        return PLUGIN_ERROR;
    }

    ret = create_album_index();
    if (ret == ERROR_BUFFER_FULL) {
        rb->splash(HZ, "Not enough memory for album names");
        return PLUGIN_ERROR;
    } else if (ret == ERROR_NO_ALBUMS) {
        rb->splash(HZ, "No albums found. Please enable database");
        return PLUGIN_ERROR;
    }

    if (!create_albumart_cache(config.avg_album_width == 0)) {
        rb->splash(HZ, "Could not create album art cache");
        return PLUGIN_ERROR;
    }

    if (!create_empty_slide(false)) {
        rb->splash(HZ, "Could not load the empty slide");
        return PLUGIN_ERROR;
    }

    if (!create_pf_thread()) {
        rb->splash(HZ, "Cannot create thread!");
        return PLUGIN_ERROR;
    }

    int i;

    /* initialize */
    int min_slide_cache = fmin(number_of_slides, SLIDE_CACHE_SIZE);
    for (i = 0; i < min_slide_cache; i++) {
        cache[i].hid = -1;
        cache[i].touched = 0;
        slide_cache_stack[i] = SLIDE_CACHE_SIZE-i-1;
    }
    slide_cache_stack_index = min_slide_cache-1;
    slide_cache_in_use = 0;
#ifdef USEGSLIB
    if (!grey_init(rb, plugin_buf, plugin_buf_size, GREY_BUFFERED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
        rb->splash(HZ, "Greylib init failed!");
#endif
    buffer = LCD_BUF;

    pf_state = pf_idle;

    track_index = -1;
    extra_fade = 0;
    center_index = 0;
    slide_frame = 0;
    step = 0;
    target = 0;
    fade = 256;

    recalc_table();
    reset_slides();

    char fpstxt[10];
    int button;

    int frames = 0;
    long last_update = *rb->current_tick;
    long current_update;
    long update_interval = 100;
    int fps = 0;
    int fpstxt_y;

    bool instant_update;
    old_drawmode = rb->lcd_get_drawmode();
#ifdef USEGSLIB
    grey_show(true);
#endif
    MYLCD(set_drawmode)(DRMODE_FG);
    while (true) {
        current_update = *rb->current_tick;
        frames++;

        /* Initial rendering */
        instant_update = false;

        /* Handle states */
        switch ( pf_state ) {
            case pf_scrolling:
                update_scroll_animation();
                render_all_slides();
                instant_update = true;
                break;
            case pf_cover_in:
                update_cover_in_animation();
                render_all_slides();
                instant_update = true;
                break;
            case pf_cover_out:
                update_cover_out_animation();
                render_all_slides();
                instant_update = true;
                break;
            case pf_show_tracks:
                show_track_list();
                break;
            case pf_idle:
                render_all_slides();
                break;
        }

        /* Calculate FPS */
        if (current_update - last_update > update_interval) {
            fps = frames * HZ / (current_update - last_update);
            last_update = current_update;
            frames = 0;
        }
        /* Draw FPS */
        if (config.show_fps)
        {
#ifdef USEGSLIB
            MYLCD(set_foreground)(G_BRIGHT(255));
#else
            MYLCD(set_foreground)(G_PIX(255,0,0));
#endif
            rb->snprintf(fpstxt, sizeof(fpstxt), "FPS: %d", fps);
            if (config.show_album_name == album_name_top)
                fpstxt_y = LCD_HEIGHT -
                           rb->screens[SCREEN_MAIN]->getcharheight();
            else
                fpstxt_y = 0;
            MYLCD(putsxy)(0, fpstxt_y, fpstxt);
        }
        draw_album_text();


        /* Copy offscreen buffer to LCD and give time to other threads */
        MYLCD(update)();
        rb->yield();

        /*/ Handle buttons */
        button = pluginlib_getaction(rb, instant_update ? 0 : HZ/16,
                                     plugin_contexts, NB_ACTION_CONTEXTS);

        switch (button) {
        case PICTUREFLOW_QUIT:
            return PLUGIN_OK;

        case PICTUREFLOW_MENU:
            if ( pf_state == pf_idle || pf_state == pf_scrolling ) {
#ifdef USEGSLIB
    grey_show(false);
#endif
                ret = main_menu();
                if ( ret == -1 ) return PLUGIN_OK;
                if ( ret != 0 ) return i;
#ifdef USEGSLIB
    grey_show(true);
#endif
                MYLCD(set_drawmode)(DRMODE_FG);
            }
            else {
                pf_state = pf_cover_out;
            }
            break;

        case PICTUREFLOW_NEXT_ALBUM:
        case PICTUREFLOW_NEXT_ALBUM_REPEAT:
#ifdef HAVE_SCROLLWHEEL
            if ( pf_state == pf_show_tracks )
                select_next_track();
#endif
            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_next_slide();
            break;

        case PICTUREFLOW_PREV_ALBUM:
        case PICTUREFLOW_PREV_ALBUM_REPEAT:
#ifdef HAVE_SCROLLWHEEL
            if ( pf_state == pf_show_tracks )
                select_prev_track();
#endif
            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_previous_slide();
            break;

#ifndef HAVE_SCROLLWHEEL
        case PICTUREFLOW_NEXT_TRACK:
        case PICTUREFLOW_NEXT_TRACK_REPEAT:
            if ( pf_state == pf_show_tracks )
                select_next_track();
            break;

        case PICTUREFLOW_PREV_TRACK:
        case PICTUREFLOW_PREV_TRACK_REPEAT:
            if ( pf_state == pf_show_tracks )
                select_prev_track();
            break;
#endif

        case PICTUREFLOW_SELECT_ALBUM:
            if ( pf_state == pf_idle ) {
                pf_state = pf_cover_in;
            }
            if ( pf_state == pf_show_tracks )
                pf_state = pf_cover_out;
            break;

        default:
            if (rb->default_event_handler_ex(button, cleanup, NULL)
                == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;
        }
    }


}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const struct plugin_api *api,
                                const void *parameter)
{
    int ret;

    rb = api;                   /* copy to global api pointer */
    (void) parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(rb);     /* backlight control in lib/helper.c */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    plugin_buf = rb->plugin_get_buffer(&plugin_buf_size);
    plugin_buf_size = rb->align_buffer(PUN_PTR(void**,&plugin_buf),
                                       plugin_buf_size, 4);
    ret = main();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    if ( ret == PLUGIN_OK ) {
        if (!write_pfconfig()) {
            rb->splash(HZ, "Error writing config.");
            ret = PLUGIN_ERROR;
        }
    }

    end_pf_thread();
    cleanup(NULL);
    return ret;
}
