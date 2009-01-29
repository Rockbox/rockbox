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
#include <albumart.h>
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"
#include "lib/configfile.h"
#include "lib/picture.h"
#include "pluginbitmaps/pictureflow_logo.h"
#include "lib/grey.h"
#include "lib/feature_wrappers.h"

PLUGIN_HEADER

/******************************* Globals ***********************************/

const struct button_mapping *plugin_contexts[]
= {generic_actions, generic_directions};

#define NB_ACTION_CONTEXTS sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

#if LCD_DEPTH < 8
#if LCD_DEPTH > 1
#define N_BRIGHT(y) LCD_BRIGHTNESS(y)
#else
#define N_BRIGHT(y) ((y > 127) ? 0 : 1)
#endif
#define USEGSLIB
GREY_INFO_STRUCT
#define LCD_BUF _grey_info.buffer
#define MYLCD(fn) grey_ ## fn
#define G_PIX(r,g,b) \
    (77 * (unsigned)(r) + 150 * (unsigned)(g) + 29 * (unsigned)(b)) / 256
#define N_PIX(r,g,b) N_BRIGHT(G_PIX(r,g,b))
#define G_BRIGHT(y) (y)
#define BUFFER_WIDTH _grey_info.width
#define BUFFER_HEIGHT _grey_info.height
typedef unsigned char pix_t;
#else
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

#define REFLECT_TOP (LCD_HEIGHT * 2 / 3)
#define REFLECT_HEIGHT (LCD_HEIGHT - REFLECT_TOP)
#define DISPLAY_HEIGHT REFLECT_TOP
#define DISPLAY_WIDTH MAX((LCD_HEIGHT * LCD_PIXEL_ASPECT_HEIGHT / \
    LCD_PIXEL_ASPECT_WIDTH / 2), (LCD_WIDTH * 2 / 5))
#define REFLECT_SC ((0x10000U * 3 + (REFLECT_HEIGHT * 5 - 1)) / \
    (REFLECT_HEIGHT * 5))
#define DISPLAY_OFFS ((LCD_HEIGHT / 2) - REFLECT_HEIGHT)

#define SLIDE_CACHE_SIZE 100

#define MAX_SLIDES_COUNT 10

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

/* Error return values */
#define ERROR_NO_ALBUMS     -1
#define ERROR_BUFFER_FULL   -2

/* current version for cover cache */
#define CACHE_VERSION 2
#define CONFIG_VERSION 1
#define CONFIG_FILE "pictureflow.cfg"

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
static char* show_album_name_conf[] =
{
    "hide",
    "bottom",
    "top"
};

#define MAX_SPACING 40
#define MAX_MARGIN 80

/* config values and their defaults */
static int slide_spacing = DISPLAY_WIDTH / 4;
static int center_margin = (LCD_WIDTH - DISPLAY_WIDTH) / 12;
static int num_slides = 4;
static int zoom = 100;
static bool show_fps = false;
static bool resize = true;
static int cache_version = 0;
static int show_album_name = (LCD_HEIGHT > 100) ? album_name_top : album_name_bottom;

static struct configdata config[] =
{
    { TYPE_INT, 0, MAX_SPACING, { .int_p = &slide_spacing }, "slide spacing",
      NULL },
    { TYPE_INT, 0, MAX_MARGIN, { .int_p = &center_margin }, "center margin",
      NULL },
    { TYPE_INT, 0, MAX_SLIDES_COUNT, { .int_p = &num_slides }, "slides count",
      NULL },
    { TYPE_INT, 0, 300, { .int_p = &zoom }, "zoom", NULL },
    { TYPE_BOOL, 0, 1, { .bool_p = &show_fps }, "show fps", NULL },
    { TYPE_BOOL, 0, 1, { .bool_p = &resize }, "resize", NULL },
    { TYPE_INT, 0, 100, { .int_p = &cache_version }, "cache version", NULL },
    { TYPE_ENUM, 0, 2, { .int_p = &show_album_name }, "show album name",
      show_album_name_conf }
};

#define CONFIG_NUM_ITEMS (sizeof(config) / sizeof(struct configdata))

/** below we allocate the memory we want to use **/

static pix_t *buffer; /* for now it always points to the lcd framebuffer */
static PFreal rays[LCD_WIDTH];
static uint16_t reflect_table[REFLECT_HEIGHT];
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

void * plugin_buf;
size_t plugin_buf_size;

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
static inline pix_t fade_color(pix_t c, unsigned int a);
bool save_pfraw(char* filename, struct bitmap *bm);
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
        0,   100,   200,   297,   392,   483,   569,   650,
      724,   792,   851,   903,   946,   980,  1004,  1019,
     1024,  1019,  1004,   980,   946,   903,   851,   792,
      724,   650,   569,   483,   392,   297,   200,   100,
        0,  -100,  -200,  -297,  -392,  -483,  -569,  -650,
     -724,  -792,  -851,  -903,  -946,  -980, -1004, -1019,
    -1024, -1019, -1004,  -980,  -946,  -903,  -851,  -792,
     -724,  -650,  -569,  -483,  -392,  -297,  -200,  -100,
        0
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

static inline uint32_t div255(uint32_t val)
{
    return ((((val >> 8) + val) >> 8) + val) >> 8;
}

#define SCALE_VAL(val,out) div255((val) * (out) + 127)

static void output_row_transposed(uint32_t row, void * row_in,
                                       struct scaler_context *ctx)
{
    pix_t *dest = (pix_t*)ctx->bm->data + row;
    pix_t *end = dest + ctx->bm->height * ctx->bm->width;
#ifdef USEGSLIB
    uint32_t *qp = (uint32_t*)row_in;
    for (; dest < end; dest += ctx->bm->height)
        *dest = SC_MUL((*qp++) + ctx->round, ctx->divisor);
#else
    struct uint32_rgb *qp = (struct uint32_rgb*)row_in;
    uint32_t rb_mul = SCALE_VAL(ctx->divisor, 31),
             rb_rnd = SCALE_VAL(ctx->round, 31),
             g_mul = SCALE_VAL(ctx->divisor, 63),
             g_rnd = SCALE_VAL(ctx->round, 63);
    int r, g, b;
    for (; dest < end; dest += ctx->bm->height)
    {
        r = SC_MUL(qp->r + rb_rnd, rb_mul);
        g = SC_MUL(qp->g + g_rnd, g_mul);
        b = SC_MUL(qp->b + rb_rnd, rb_mul);
        qp++;
        *dest = LCD_RGBPACK_LCD(r,g,b);
    }
#endif
}

static unsigned int get_size(struct bitmap *bm)
{
    return bm->width * bm->height * sizeof(pix_t);
}

const struct custom_format format_transposed = {
    .output_row = output_row_transposed,
    .get_size = get_size
};

/* Create the lookup table with the scaling values for the reflections */
void init_reflect_table(void)
{
    int i;
    for (i = 0; i < REFLECT_HEIGHT; i++)
        reflect_table[i] =
            (768 * (REFLECT_HEIGHT - i) + (5 * REFLECT_HEIGHT / 2)) /
            (5 * REFLECT_HEIGHT);
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
        if ( search_albumart_files(&id3, "", buf, buflen) )
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

#if LCD_DEPTH > 1
    rb->lcd_set_background(N_BRIGHT(0));
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif
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

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(100));
#endif
    rb->lcd_drawrect(x, y, w+2, bar_height);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_PIX(165, 231, 82));
#endif

    rb->lcd_fillrect(x+1, y+1, step * w / album_count, bar_height-2);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif
    rb->lcd_update();
    rb->yield();
}

/**
 Precomupte the album art images and store them in CACHE_PREFIX.
 */
bool create_albumart_cache(void)
{
    int ret;

    int i, slides = 0;
    struct bitmap input_bmp;

    char pfraw_file[MAX_PATH];
    char albumart_file[MAX_PATH];
    unsigned int format = FORMAT_NATIVE;
    cache_version = 0;
    configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);
    if (resize)
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
        input_bmp.width = DISPLAY_WIDTH;
        input_bmp.height = DISPLAY_HEIGHT;
        ret = scaled_read_bmp_file(albumart_file, &input_bmp,
                                plugin_buf_size, format, &format_transposed);
        if (ret <= 0) {
            rb->splash(HZ, "Could not read bmp");
            continue; /* skip missing/broken files */
        }
        if (!save_pfraw(pfraw_file, &input_bmp))
        {
            rb->splash(HZ, "Could not write bmp");
        }
        slides++;
        if ( rb->button_get(false) == PICTUREFLOW_MENU ) return false;
    }
    if ( slides == 0 ) {
        /* Warn the user that we couldn't find any albumart */
        rb->splash(2*HZ, "No album art found");
        return false;
    }
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
#if LCD_DEPTH > 1
    bm->format = FORMAT_NATIVE;
#endif
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
    for (i = 0; i < num_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx = -(offsetX + slide_spacing * i * PFREAL_ONE);
        si->cy = offsetY;
        si->slide_index = center_index - 1 - i;
        si->distance = 0;
    }

    for (i = 0; i < num_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx = offsetX + slide_spacing * i * PFREAL_ONE;
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

    offsetX = DISPLAY_WIDTH / 2 * (fsin(itilt) + PFREAL_ONE);
    offsetY = DISPLAY_WIDTH / 2 * (fsin(itilt) + PFREAL_ONE / 2);
    offsetX += center_margin << PFREAL_SHIFT;
}


/**
   Fade the given color by spreading the fb_data (ushort)
   to an uint, multiply and compress the result back to a ushort.
 */
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
static inline pix_t fade_color(pix_t c, unsigned int a)
{
    unsigned int result;
    c = swap16(c);
    a = (a + 2) & 0x1fc;
    result = ((c & 0xf81f) * a) & 0xf81f00;
    result |= ((c & 0x7e0) * a) & 0x7e000;
    result >>= 8;
    return swap16(result);
}
#elif LCD_PIXELFORMAT == RGB565
static inline pix_t fade_color(pix_t c, unsigned int a)
{
    unsigned int result;
    a = (a + 2) & 0x1fc;
    result = ((c & 0xf81f) * a) & 0xf81f00;
    result |= ((c & 0x7e0) * a) & 0x7e000;
    result >>= 8;
    return result;
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
void render_slide(struct slide_data *slide, const int alpha)
{
    struct bitmap *bmp = surface(slide->slide_index);
    if (!bmp) {
        return;
    }
    pix_t *src = (pix_t *)bmp->data;

    const int sw = bmp->width;
    const int sh = bmp->height;

    const int h = LCD_HEIGHT;
    const int w = LCD_WIDTH;


    int distance = (h + slide->distance) * 100 / zoom;
    if (distance < 100 ) distance = 100; /* clamp distances */
    PFreal dist = distance * PFREAL_ONE;
    PFreal sdx = fcos(slide->angle);
    PFreal sdy = fsin(slide->angle);
    PFreal xs = slide->cx - sw * fdiv(sdx, dist) / 2;

    int xi = fmax(0, xs) >> PFREAL_SHIFT;
    if (xi >= w) {
        return;
    }

    int x;
    for (x = fmax(xi, 0); x < w; x++) {
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

        int y1 = (LCD_HEIGHT / 2) - 1;
        int y2 = y1 + 1;
        pix_t *pixel1 = &buffer[y1 * BUFFER_WIDTH + x];
        pix_t *pixel2 = &buffer[y2 * BUFFER_WIDTH + x];
        const int pixelstep = pixel2 - pixel1;

        int dy = dist / h;
        int p1 = (bmp->height - 1 - (DISPLAY_OFFS)) * PFREAL_ONE;
        int p2 = p1 + dy;
        const pix_t *ptr = &src[column * bmp->height];

        if (alpha == 256)
        {
            while ((y1 >= 0) && (p1 >= 0))
            {
                *pixel1 = ptr[p1 >> PFREAL_SHIFT];
                p1 -= dy;
                y1--;
                pixel1 -= pixelstep;
            }
            while ((p2 < sh * PFREAL_ONE) && (y2 < h))
            {
                *pixel2 = ptr[p2 >> PFREAL_SHIFT];
                p2 += dy;
                y2++;
                pixel2 += pixelstep;
            }
            while ((p2 < MIN(sh + REFLECT_HEIGHT, sh * 2) * PFREAL_ONE) &&
                   (y2 < h))
            {
                int ty = (p2 >> PFREAL_SHIFT) - sh;
                int lalpha = reflect_table[ty];
                *pixel2 = fade_color(ptr[sh - 1 - ty],lalpha);
                p2 += dy;
                y2++;
                pixel2 += pixelstep;
            }
        }
        else
            while ((y1 >= 0) && (p1 >= 0))
            {
                *pixel1 = fade_color(ptr[p1 >> PFREAL_SHIFT],alpha);
                p1 -= dy;
                y1--;
                pixel1 -= pixelstep;
            }
            while ((p2 < sh * PFREAL_ONE) && (y2 < h))
            {
                *pixel2 = fade_color(ptr[p2 >> PFREAL_SHIFT],alpha);
                p2 += dy;
                y2++;
                pixel2 += pixelstep;
            }
            while ((p2 < MIN(sh + REFLECT_HEIGHT, sh * 2) * PFREAL_ONE) &&
                   (y2 < h))
            {
                int ty = (p2 >> PFREAL_SHIFT) - sh;
                int lalpha = (reflect_table[ty] * alpha + 128) >> 8;
                *pixel2 = fade_color(ptr[sh - 1 - ty],lalpha);
                p2 += dy;
                y2++;
                pixel2 += pixelstep;
            }
    }
    /* let the music play... */
    rb->yield();

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

    int nleft = num_slides;
    int nright = num_slides;

    int index;
    if (step == 0) {
        /* no animation, boring plain rendering */
        for (index = nleft - 2; index >= 0; index--) {
            int alpha = (index < nleft - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha > 0 )
                render_slide(&left_slides[index], alpha);
        }
        for (index = nright - 2; index >= 0; index--) {
            int alpha = (index < nright - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha > 0 )
                render_slide(&right_slides[index], alpha);
        }
    } else {
        /* the first and last slide must fade in/fade out */
        for (index = nleft - 1; index >= 0; index--) {
            int alpha = 256;
            if (index == nleft - 1)
                alpha = (step > 0) ? 0 : 128 - fade / 2;
            if (index == nleft - 2)
                alpha = (step > 0) ? 128 - fade / 2 : 256 - fade / 2;
            if (index == nleft - 3)
                alpha = (step > 0) ? 256 - fade / 2 : 256;
            render_slide(&left_slides[index], alpha);
        }
        for (index = nright - 1; index >= 0; index--) {
            int alpha = (index < nright - 2) ? 256 : 128;
            if (index == nright - 1)
                alpha = (step > 0) ? fade / 2 : 0;
            if (index == nright - 2)
                alpha = (step > 0) ? 128 + fade / 2 : fade / 2;
            if (index == nright - 3)
                alpha = (step > 0) ? 256 : 128 + fade / 2;
            render_slide(&right_slides[index], alpha);
        }
    }
    render_slide(&center_slide, 256);
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
        for (i = 0; i < num_slides; i++)
            left_slides[i].slide_index = center_index - 1 - i;
        for (i = 0; i < num_slides; i++)
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

    for (i = 0; i < num_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx =
            -(offsetX + slide_spacing * i * PFREAL_ONE + step
                        * slide_spacing * ftick);
        si->cy = offsetY;
    }

    for (i = 0; i < num_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx =
            offsetX + slide_spacing * i * PFREAL_ONE - step
                      * slide_spacing * ftick;
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
    backlight_use_settings(); /* backlight control in lib/helper.c */

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
        input_bmp.width = DISPLAY_WIDTH;
        input_bmp.height = DISPLAY_HEIGHT;
#if LCD_DEPTH > 1
        input_bmp.format = FORMAT_NATIVE;
#endif
        input_bmp.data = (char*)plugin_buf;
        ret = scaled_read_bmp_file(EMPTY_SLIDE_BMP, &input_bmp,
                                plugin_buf_size,
                                FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                                &format_transposed);
        if (!save_pfraw(EMPTY_SLIDE, &input_bmp))
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
    int selection = show_album_name;

    MENUITEM_STRINGLIST(album_name_menu,"Show album title",NULL,
            "Hide album title", "Show at the bottom", "Show at the top");
    rb->do_menu(&album_name_menu, &selection, NULL, false);

    show_album_name = selection;
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
                rb->set_bool("Show FPS", &show_fps);
                reset_track_list();
                break;

            case 1:
                rb->set_int("Spacing between slides", "", 1,
                            &slide_spacing,
                            NULL, 1, 0, 100, NULL );
                recalc_table();
                reset_slides();
                break;

            case 2:
                rb->set_int("Center margin", "", 1,
                            &center_margin,
                            NULL, 1, 0, 80, NULL );
                recalc_table();
                reset_slides();
                break;

            case 3:
                rb->set_int("Number of slides", "", 1, &num_slides,
                            NULL, 1, 1, MAX_SLIDES_COUNT, NULL );
                recalc_table();
                reset_slides();
                break;

            case 4:
                rb->set_int("Zoom", "", 1, &zoom,
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
                old_val = resize;
                rb->set_bool("Resize Covers", &resize);
                if (old_val == resize) /* changed? */
                    break;
                /* fallthrough if changed, since cache needs to be rebuilt */
            case 7:
                cache_version = 0;
                rb->remove(EMPTY_SLIDE);
                rb->splash(HZ, "Cache will be rebuilt on next restart");
                break;

            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
        }
    } while ( selection >= 0 );
    configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);
    return 0;
}

/**
  Show the main menu
 */
int main_menu(void)
{
    int selection = 0;
    int result;

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif

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
    switch (show_album_name)
    {
        case album_name_hide:
            track_list_y = (show_fps ? char_height : 0);
            track_list_h = LCD_HEIGHT - track_list_y;
            break;
        case album_name_bottom:
            track_list_y = (show_fps ? char_height : 0);
            track_list_h = LCD_HEIGHT - track_list_y - char_height * 2;
            break;
        default: /* case album_name_top */
            track_list_y = char_height * 2;
            track_list_h = LCD_HEIGHT - track_list_y -
                           (show_fps ? char_height : 0);
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
    if (0 == show_album_name)
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

    if (show_album_name == album_name_top)
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
    
    rb->lcd_setfont(FONT_UI);
    draw_splashscreen();

    if ( ! rb->dir_exists( CACHE_PREFIX ) ) {
        if ( rb->mkdir( CACHE_PREFIX ) < 0 ) {
            rb->splash(HZ, "Could not create directory " CACHE_PREFIX );
            return PLUGIN_ERROR;
        }
    }

    configfile_load(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);

    init_reflect_table();

    ret = create_album_index();
    if (ret == ERROR_BUFFER_FULL) {
        rb->splash(HZ, "Not enough memory for album names");
        return PLUGIN_ERROR;
    } else if (ret == ERROR_NO_ALBUMS) {
        rb->splash(HZ, "No albums found. Please enable database");
        return PLUGIN_ERROR;
    }
    
    number_of_slides  = album_count;
    if ((cache_version != CACHE_VERSION) && !create_albumart_cache()) {
        rb->splash(HZ, "Could not create album art cache");
        return PLUGIN_ERROR;
    }

    if (!create_empty_slide(cache_version != CACHE_VERSION)) {
        rb->splash(HZ, "Could not load the empty slide");
        return PLUGIN_ERROR;
    }
    cache_version = CACHE_VERSION;
    configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);

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
    if (!grey_init(plugin_buf, plugin_buf_size, GREY_BUFFERED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
        rb->splash(HZ, "Greylib init failed!");
    grey_setfont(FONT_UI);
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
    grey_set_drawmode(DRMODE_FG);
#endif
    rb->lcd_set_drawmode(DRMODE_FG);
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
        if (show_fps)
        {
#ifdef USEGSLIB
            MYLCD(set_foreground)(G_BRIGHT(255));
#else
            MYLCD(set_foreground)(G_PIX(255,0,0));
#endif
            rb->snprintf(fpstxt, sizeof(fpstxt), "FPS: %d", fps);
            if (show_album_name == album_name_top)
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
        button = pluginlib_getaction(instant_update ? 0 : HZ/16,
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

enum plugin_status plugin_start(const void *parameter)
{
    int ret;

    (void) parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on();     /* backlight control in lib/helper.c */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    plugin_buf = rb->plugin_get_buffer(&plugin_buf_size);
    ALIGN_BUFFER(plugin_buf, plugin_buf_size, 4);
    ret = main();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    if ( ret == PLUGIN_OK ) {
        if (configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS,
                            CONFIG_VERSION))
        {
            rb->splash(HZ, "Error writing config.");
            ret = PLUGIN_ERROR;
        }
    }

    end_pf_thread();
    cleanup(NULL);
    return ret;
}
