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
#include "lib/read_image.h"
#include "lib/pluginlib_actions.h"
#include "lib/helper.h"
#include "lib/configfile.h"
#include "lib/grey.h"
#include "lib/feature_wrappers.h"
#include "lib/buflib.h"

PLUGIN_HEADER

/******************************* Globals ***********************************/

/*
 *  Targets which use plugin_get_audio_buffer() can't have playback from
 * within pictureflow itself, as the whole core audio buffer is occupied */
#define PF_PLAYBACK_CAPABLE (PLUGIN_BUFFER_SIZE > 0x10000)

#if PF_PLAYBACK_CAPABLE
#include "lib/playback_control.h"
#endif

#define PF_PREV ACTION_STD_PREV
#define PF_PREV_REPEAT ACTION_STD_PREVREPEAT
#define PF_NEXT ACTION_STD_NEXT
#define PF_NEXT_REPEAT ACTION_STD_NEXTREPEAT
#define PF_SELECT ACTION_STD_OK
#define PF_CONTEXT ACTION_STD_CONTEXT
#define PF_BACK ACTION_STD_CANCEL
#define PF_MENU ACTION_STD_MENU
#define PF_WPS ACTION_TREE_WPS

#define PF_QUIT (LAST_ACTION_PLACEHOLDER + 1)

#if defined(HAVE_SCROLLWHEEL) || CONFIG_KEYPAD == IRIVER_H10_PAD || \
    CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define USE_CORE_PREVNEXT
#endif

#ifndef USE_CORE_PREVNEXT
    /* scrollwheel targets use the wheel, just as they do in lists,
     * so there's no need for a special context,
     * others use left/right here too (as oppsed to up/down in lists) */
const struct button_mapping pf_context_album_scroll[] =
{
#ifdef HAVE_TOUCHSCREEN
    {PF_PREV,         BUTTON_MIDLEFT,                 BUTTON_NONE},
    {PF_PREV_REPEAT,  BUTTON_MIDLEFT|BUTTON_REPEAT,   BUTTON_NONE},
    {PF_NEXT,         BUTTON_MIDRIGHT,                BUTTON_NONE},
    {PF_NEXT_REPEAT,  BUTTON_MIDRIGHT|BUTTON_REPEAT,  BUTTON_NONE},
#endif
#if (CONFIG_KEYPAD == IAUDIO_M3_PAD || CONFIG_KEYPAD == MROBE500_PAD)
    {PF_PREV,         BUTTON_RC_REW,              BUTTON_NONE},
    {PF_PREV_REPEAT,  BUTTON_RC_REW|BUTTON_REPEAT,BUTTON_NONE},
    {PF_NEXT,         BUTTON_RC_FF,               BUTTON_NONE},
    {PF_NEXT_REPEAT,  BUTTON_RC_FF|BUTTON_REPEAT, BUTTON_NONE},
#else
    {PF_PREV,         BUTTON_LEFT,                BUTTON_NONE},
    {PF_PREV_REPEAT,  BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    {PF_NEXT,         BUTTON_RIGHT,               BUTTON_NONE},
    {PF_NEXT_REPEAT,  BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT},
#endif
#if CONFIG_KEYPAD == ONDIO_PAD
    {PF_SELECT,       BUTTON_UP|BUTTON_REL,       BUTTON_UP},
    {PF_CONTEXT,      BUTTON_UP|BUTTON_REPEAT,    BUTTON_UP},
    {ACTION_NONE,     BUTTON_UP,                  BUTTON_NONE},
    {ACTION_NONE,     BUTTON_DOWN,                BUTTON_NONE},
    {ACTION_NONE,     BUTTON_DOWN|BUTTON_REPEAT,  BUTTON_NONE},
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_PLUGIN|1)
};
#endif /* !USE_CORE_PREVNEXT */

const struct button_mapping pf_context_buttons[] =
{
#ifdef HAVE_TOUCHSCREEN
    {PF_SELECT,       BUTTON_CENTER,              BUTTON_NONE},
    {PF_BACK,         BUTTON_BOTTOMRIGHT,         BUTTON_NONE},
#endif
#if CONFIG_KEYPAD == ARCHOS_AV300_PAD
    {PF_QUIT,         BUTTON_OFF,                 BUTTON_NONE},
#elif CONFIG_KEYPAD == SANSA_C100_PAD
    {PF_QUIT,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
#elif CONFIG_KEYPAD == CREATIVEZV_PAD || CONFIG_KEYPAD == CREATIVEZVM_PAD || \
    CONFIG_KEYPAD == PHILIPS_HDD1630_PAD || CONFIG_KEYPAD == IAUDIO67_PAD || \
    CONFIG_KEYPAD == GIGABEAT_PAD || CONFIG_KEYPAD == GIGABEAT_S_PAD || \
    CONFIG_KEYPAD == MROBE100_PAD || CONFIG_KEYPAD == MROBE500_PAD || \
    CONFIG_KEYPAD == PHILIPS_SA9200_PAD || CONFIG_KEYPAD == SANSA_CLIP_PAD
    {PF_QUIT,         BUTTON_POWER,               BUTTON_NONE},
#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
    {PF_QUIT,         BUTTON_HOME|BUTTON_REPEAT,  BUTTON_NONE},
/* These all use short press of BUTTON_POWER for menu, map long POWER to quit
*/
#elif CONFIG_KEYPAD == SANSA_C200_PAD || CONFIG_KEYPAD == SANSA_M200_PAD || \
    CONFIG_KEYPAD == IRIVER_H10_PAD || CONFIG_KEYPAD == COWON_D2_PAD
    {PF_QUIT,         BUTTON_POWER|BUTTON_REPEAT, BUTTON_POWER},
#if CONFIG_KEYPAD == COWON_D2_PAD
    {PF_BACK,         BUTTON_POWER|BUTTON_REL,    BUTTON_POWER},
    {ACTION_NONE,     BUTTON_POWER,               BUTTON_NONE},
#endif
#elif CONFIG_KEYPAD == SANSA_E200_PAD
    {PF_QUIT,         BUTTON_POWER,               BUTTON_NONE},
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
    {PF_QUIT,         BUTTON_EQ,                  BUTTON_NONE},
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PF_QUIT,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
#elif CONFIG_KEYPAD == LOGIK_DAX_PAD
    {PF_QUIT,         BUTTON_POWERPLAY|BUTTON_REPEAT, BUTTON_POWERPLAY},
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    {PF_QUIT,         BUTTON_RC_REC,              BUTTON_NONE},
#elif CONFIG_KEYPAD == MEIZU_M6SL_PAD
    {PF_QUIT,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
#elif CONFIG_KEYPAD == IRIVER_H100_PAD || CONFIG_KEYPAD == IRIVER_H300_PAD || \
    CONFIG_KEYPAD == RECORDER_PAD || CONFIG_KEYPAD == ONDIO_PAD
    {PF_QUIT,         BUTTON_OFF,                 BUTTON_NONE},
#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
    {PF_QUIT,         BUTTON_REC,                 BUTTON_NONE},
#endif
#if CONFIG_KEYPAD == IAUDIO_M3_PAD
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_STD|CONTEXT_REMOTE)
#else
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_TREE)
#endif
};
const struct button_mapping *pf_contexts[] =
{
#ifndef USE_CORE_PREVNEXT
    pf_context_album_scroll,
#endif
    pf_context_buttons
};

#if LCD_DEPTH < 8
#if LCD_DEPTH > 1
#define N_BRIGHT(y) LCD_BRIGHTNESS(y)
#else   /* LCD_DEPTH <= 1 */
#define N_BRIGHT(y) ((y > 127) ? 0 : 1)
#ifdef HAVE_NEGATIVE_LCD  /* m:robe 100, Clip */
#define PICTUREFLOW_DRMODE DRMODE_SOLID
#else
#define PICTUREFLOW_DRMODE (DRMODE_SOLID|DRMODE_INVERSEVID)
#endif
#endif  /* LCD_DEPTH <= 1 */
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
#else   /* LCD_DEPTH >= 8 */
#define LCD_BUF rb->lcd_framebuffer
#define MYLCD(fn) rb->lcd_ ## fn
#define G_PIX LCD_RGBPACK
#define N_PIX LCD_RGBPACK
#define G_BRIGHT(y) LCD_RGBPACK(y,y,y)
#define N_BRIGHT(y) LCD_RGBPACK(y,y,y)
#define BUFFER_WIDTH LCD_WIDTH
#define BUFFER_HEIGHT LCD_HEIGHT
typedef fb_data pix_t;
#endif  /* LCD_DEPTH >= 8 */

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
#define CAM_DIST MAX(MIN(LCD_HEIGHT,LCD_WIDTH),120)
#define CAM_DIST_R (CAM_DIST << PFREAL_SHIFT)
#define DISPLAY_LEFT_R (PFREAL_HALF - LCD_WIDTH * PFREAL_HALF)
#define MAXSLIDE_LEFT_R (PFREAL_HALF - DISPLAY_WIDTH * PFREAL_HALF)

#define SLIDE_CACHE_SIZE 64 /* probably more than can be loaded */

#define MAX_SLIDES_COUNT 10

#define THREAD_STACK_SIZE DEFAULT_STACK_SIZE + 0x200
#define CACHE_PREFIX PLUGIN_DEMOS_DIR "/pictureflow"

#define EV_EXIT 9999
#define EV_WAKEUP 1337

#define EMPTY_SLIDE CACHE_PREFIX "/emptyslide.pfraw"
#define EMPTY_SLIDE_BMP PLUGIN_DEMOS_DIR "/pictureflow_emptyslide.bmp"
#define SPLASH_BMP PLUGIN_DEMOS_DIR "/pictureflow_splash.bmp"

/* Error return values */
#define ERROR_NO_ALBUMS     -1
#define ERROR_BUFFER_FULL   -2

/* current version for cover cache */
#define CACHE_VERSION 3
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
    short next; /* "next" slide, with LRU last */
    short prev; /* "previous" slide */
};

struct album_data {
    int name_idx;
    long seek;
};

struct track_data {
    uint32_t sort;
    int name_idx;       /* offset to the track name */
    long seek;
#if PF_PLAYBACK_CAPABLE
    /* offset to the filename in the string, needed for playlist generation */
    int filename_idx;
#endif
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

enum show_album_name_values { album_name_hide = 0, album_name_bottom,
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
static int show_album_name = (LCD_HEIGHT > 100)
    ? album_name_top : album_name_bottom;

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
static uint8_t reflect_table[REFLECT_HEIGHT];
static struct slide_data center_slide;
static struct slide_data left_slides[MAX_SLIDES_COUNT];
static struct slide_data right_slides[MAX_SLIDES_COUNT];
static int slide_frame;
static int step;
static int target;
static int fade;
static int center_index = 0; /* index of the slide that is in the center */
static int itilt;
static PFreal offsetX;
static PFreal offsetY;
static int number_of_slides;

static struct slide_cache cache[SLIDE_CACHE_SIZE];
static int cache_free;
static int cache_used = -1;
static int cache_left_index = -1;
static int cache_right_index = -1;
static int cache_center_index = -1;

/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE / sizeof(long)];
/* queue (as array) for scheduling load_surface */

static int empty_slide_hid;

unsigned int thread_id;
struct event_queue thread_q;

static struct tagcache_search tcs;

static struct buflib_context buf_ctx;

static struct album_data *album;
static char *album_names;
static int album_count;

static struct track_data *tracks;
static char *track_names;
static size_t borrowed = 0;
static int track_count;
static int track_index;
static int selected_track;
static int selected_track_pulse;
void reset_track_list(void);

void * buf;
size_t buf_size;

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
static bool free_slide_prio(int prio);
static inline unsigned fade_color(pix_t c, unsigned a);
bool save_pfraw(char* filename, struct bitmap *bm);
bool load_new_slide(void);
int load_surface(int);

static inline PFreal fmul(PFreal a, PFreal b)
{
    return (a*b) >> PFREAL_SHIFT;
}

/**
 * This version preshifts each operand, which is useful when we know how many
 * of the least significant bits will be empty, or are worried about overflow
 * in a particular calculation
 */
static inline PFreal fmuln(PFreal a, PFreal b, int ps1, int ps2)
{
    return ((a >> ps1) * (b >> ps2)) >> (PFREAL_SHIFT - ps1 - ps2);
}

/* ARMv5+ has a clz instruction equivalent to our function.
 */
#if (defined(CPU_ARM) && (ARM_ARCH > 4))
static inline int clz(uint32_t v)
{
    return __builtin_clz(v);
}

/* Otherwise, use our clz, which can be inlined */
#elif defined(CPU_COLDFIRE)
/* This clz is based on the log2(n) implementation at
 * http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
 * A clz benchmark plugin showed this to be about 14% faster on coldfire
 * than the LUT-based version.
 */
static inline int clz(uint32_t v)
{
    int r = 32;
    if (v >= 0x10000)
    {
        v >>= 16;
        r -= 16;
    }
    if (v & 0xff00)
    {
        v >>= 8;
        r -= 8;
    }
    if (v & 0xf0)
    {
        v >>= 4;
        r -= 4;
    }
    if (v & 0xc)
    {
        v >>= 2;
        r -= 2;
    }
    if (v & 2)
    {
        v >>= 1;
        r -= 1;
    }
    r -= v;
    return r;
}
#else
static const char clz_lut[16] = { 4, 3, 2, 2, 1, 1, 1, 1,
                                  0, 0, 0, 0, 0, 0, 0, 0 };
/* This clz is based on the log2(n) implementation at
 * http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
 * It is not any faster than the one above, but trades 16B in the lookup table
 * for a savings of 12B per each inlined call.
 */
static inline int clz(uint32_t v)
{
    int r = 28;
    if (v >= 0x10000)
    {
        v >>= 16;
        r -= 16;
    }
    if (v & 0xff00)
    {
        v >>= 8;
        r -= 8;
    }
    if (v & 0xf0)
    {
        v >>= 4;
        r -= 4;
    }
    return r + clz_lut[v];
}
#endif

/* Return the maximum possible left shift for a signed int32, without
 * overflow
 */
static inline int allowed_shift(int32_t val)
{
    uint32_t uval = val ^ (val >> 31);
    return clz(uval) - 1;
}

/* Calculate num/den, with the result shifted left by PFREAL_SHIFT, by shifting
 * num and den before dividing.
 */
static inline PFreal fdiv(PFreal num, PFreal den)
{
    int shift = allowed_shift(num);
    shift = MIN(PFREAL_SHIFT, shift);
    num <<= shift;
    den >>= PFREAL_SHIFT - shift;
    return num / den;
}

#define fmin(a,b) (((a) < (b)) ? (a) : (b))
#define fmax(a,b) (((a) > (b)) ? (a) : (b))
#define fabs(a) (a < 0 ? -a : a)
#define fbound(min,val,max) (fmax((min),fmin((max),(val))))

#if CONFIG_CPU == SH7034
/* 16*16->32 bit multiplication is a single instrcution on the SH1 */
#define MULUQ(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#else
#define MULUQ(a, b) ((a) * (b))
#endif


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
static const short sin_tab[] = {
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

static inline unsigned scale_val(unsigned val, unsigned bits)
{
    val = val * ((1 << bits) - 1);
    return ((val >> 8) + val + 128) >> 8;
}

static void output_row_8_transposed(uint32_t row, void * row_in,
                                       struct scaler_context *ctx)
{
    pix_t *dest = (pix_t*)ctx->bm->data + row;
    pix_t *end = dest + ctx->bm->height * ctx->bm->width;
#ifdef USEGSLIB
    uint8_t *qp = (uint8_t*)row_in;
    for (; dest < end; dest += ctx->bm->height)
        *dest = *qp++;
#else
    struct uint8_rgb *qp = (struct uint8_rgb*)row_in;
    unsigned r, g, b;
    for (; dest < end; dest += ctx->bm->height)
    {
        r = scale_val(qp->red, 5);
        g = scale_val(qp->green, 6);
        b = scale_val((qp++)->blue, 5);
        *dest = LCD_RGBPACK_LCD(r,g,b);
    }
#endif
}

static void output_row_32_transposed(uint32_t row, void * row_in,
                                       struct scaler_context *ctx)
{
    pix_t *dest = (pix_t*)ctx->bm->data + row;
    pix_t *end = dest + ctx->bm->height * ctx->bm->width;
#ifdef USEGSLIB
    uint32_t *qp = (uint32_t*)row_in;
    for (; dest < end; dest += ctx->bm->height)
        *dest = SC_OUT(*qp++, ctx);
#else
    struct uint32_rgb *qp = (struct uint32_rgb*)row_in;
    int r, g, b;
    for (; dest < end; dest += ctx->bm->height)
    {
        r = scale_val(SC_OUT(qp->r, ctx), 5);
        g = scale_val(SC_OUT(qp->g, ctx), 6);
        b = scale_val(SC_OUT(qp->b, ctx), 5);
        qp++;
        *dest = LCD_RGBPACK_LCD(r,g,b);
    }
#endif
}

#ifdef HAVE_LCD_COLOR
static void output_row_32_transposed_fromyuv(uint32_t row, void * row_in,
                                       struct scaler_context *ctx)
{
    pix_t *dest = (pix_t*)ctx->bm->data + row;
    pix_t *end = dest + ctx->bm->height * ctx->bm->width;
    struct uint32_rgb *qp = (struct uint32_rgb*)row_in;
    for (; dest < end; dest += ctx->bm->height)
    {
        unsigned r, g, b, y, u, v;
        y = SC_OUT(qp->b, ctx);
        u = SC_OUT(qp->g, ctx);
        v = SC_OUT(qp->r, ctx);
        qp++;
        yuv_to_rgb(y, u, v, &r, &g, &b);
        r = scale_val(r, 5);
        g = scale_val(g, 6);
        b = scale_val(b, 5);
        *dest = LCD_RGBPACK_LCD(r, g, b);
    }
}
#endif

static unsigned int get_size(struct bitmap *bm)
{
    return bm->width * bm->height * sizeof(pix_t);
}

const struct custom_format format_transposed = {
    .output_row_8 = output_row_8_transposed,
#ifdef HAVE_LCD_COLOR
    .output_row_32 = {
        output_row_32_transposed,
        output_row_32_transposed_fromyuv
    },
#else
    .output_row_32 = output_row_32_transposed,
#endif
    .get_size = get_size
};

static const struct button_mapping* get_context_map(int context)
{
    return pf_contexts[context & ~CONTEXT_PLUGIN];
}

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
    album = ((struct album_data *)(buf_size + (char *) buf)) - 1;
    rb->memset(&tcs, 0, sizeof(struct tagcache_search) );
    album_count = 0;
    rb->tagcache_search(&tcs, tag_album);
    unsigned int l, old_l = 0;
    album_names = buf;
    album[0].name_idx = 0;
    while (rb->tagcache_get_next(&tcs))
    {
        buf_size -= sizeof(struct album_data);
        l = tcs.result_len;
        if ( album_count > 0 )
            album[-album_count].name_idx = album[1-album_count].name_idx + old_l;

        if ( l > buf_size )
            /* not enough memory */
            return ERROR_BUFFER_FULL;

        rb->strcpy(buf, tcs.result);
        buf_size -= l;
        buf = l + (char *)buf;
        album[-album_count].seek = tcs.result_seek;
        old_l = l;
            album_count++;
    }
    rb->tagcache_search_finish(&tcs);
    ALIGN_BUFFER(buf, buf_size, 4);
    int i;
    struct album_data* tmp_album = (struct album_data*)buf;
    for (i = album_count - 1; i >= 0; i--)
        tmp_album[i] = album[-i];
    album = tmp_album;
    buf = album + album_count;
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
#if PF_PLAYBACK_CAPABLE
char* get_track_filename(const int track_index)
{
    if ( track_index < track_count )
        return track_names + tracks[track_index].filename_idx;
    return 0;
}
#endif
/**
  Compare two unsigned ints passed via pointers.
 */
int compare_tracks (const void *a_v, const void *b_v)
{
    uint32_t a = ((struct track_data *)a_v)->sort;
    uint32_t b = ((struct track_data *)b_v)->sort;
    return (int)(a - b);
}

/**
  Create the track index of the given slide_index.
 */
void create_track_index(const int slide_index)
{
    if ( slide_index == track_index )
        return;
    track_index = slide_index;

    if (!rb->tagcache_search(&tcs, tag_title))
        goto fail;

    rb->tagcache_search_add_filter(&tcs, tag_album, album[slide_index].seek);
    track_count=0;
    int string_index = 0, track_num;
    int disc_num;
    size_t out = 0;
    track_names = (char *)buflib_buffer_out(&buf_ctx, &out);
    borrowed += out;
    int avail = borrowed;
    tracks = (struct track_data*)(track_names + borrowed);
    while (rb->tagcache_get_next(&tcs))
    {
        int len = 0, fn_idx = 0;

        avail -= sizeof(struct track_data);
        track_num = rb->tagcache_get_numeric(&tcs, tag_tracknumber) - 1;
        disc_num = rb->tagcache_get_numeric(&tcs, tag_discnumber);

        if (disc_num < 0)
            disc_num = 0;
retry:
        if (track_num >= 0)
        {
            if (disc_num)
                fn_idx = 1 + rb->snprintf(track_names + string_index , avail,
                    "%d.%02d: %s", disc_num, track_num + 1, tcs.result);
            else
                fn_idx = 1 + rb->snprintf(track_names + string_index , avail,
                    "%d: %s", track_num + 1, tcs.result);
        }
        else
        {
            track_num = 0;
            fn_idx = 1 + rb->snprintf(track_names + string_index, avail,
                "%s", tcs.result);
        }
        if (fn_idx <= 0)
            goto fail;
#if PF_PLAYBACK_CAPABLE
        int remain = avail - fn_idx;
        if (remain >= MAX_PATH)
        {   /* retrieve filename for building the playlist */
            rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_filename,
                    track_names + string_index + fn_idx, remain);
            len = fn_idx + rb->strlen(track_names + string_index + fn_idx) + 1;
            /* make sure track name and file name are really split by a \0, else
             * get_track_name might fail */
            *(track_names + string_index + fn_idx -1) = '\0';

        }
        else /* request more buffer so that track and filename fit */
            len = (avail - remain) + MAX_PATH;
#else
            len = fn_idx;
#endif
        if (len > avail)
        {
            while (len > avail)
            {
                if (!free_slide_prio(0))
                    goto fail;
                out = 0;
                buflib_buffer_out(&buf_ctx, &out);
                avail += out;
                borrowed += out;
                if (track_count)
                {
                    struct track_data *new_tracks = (struct track_data *)(out + (uintptr_t)tracks);
                    unsigned int bytes = track_count * sizeof(struct track_data);
                    rb->memmove(new_tracks, tracks, bytes);
                    tracks = new_tracks;
                }
            }
            goto retry;
        }

        avail -= len;
        tracks--;
        tracks->sort = ((disc_num - 1) << 24) + (track_num << 14) + track_count;
        tracks->name_idx = string_index;
        tracks->seek = tcs.result_seek;
#if PF_PLAYBACK_CAPABLE
        tracks->filename_idx = fn_idx + string_index;
#endif
        track_count++;
        string_index += len;
    }

    rb->tagcache_search_finish(&tcs);

    /* now fix the track list order */
    rb->qsort(tracks, track_count, sizeof(struct track_data), compare_tracks);
    return;
fail:
    track_count = 0;
    return;
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
        rb->strlcpy( buf, EMPTY_SLIDE, buflen );
    }

    if (!rb->tagcache_search(&tcs, tag_filename))
        return false;

    bool result;
    /* find the first track of the album */
    rb->tagcache_search_add_filter(&tcs, tag_album, album[slide_index].seek);

    if ( rb->tagcache_get_next(&tcs) ) {
        struct mp3entry id3;
        int fd;

#ifdef HAVE_TC_RAMCACHE
        if (rb->tagcache_fill_tags(&id3, tcs.result))
        {
            rb->strlcpy(id3.path, tcs.result, sizeof(id3.path));
        }
        else
#endif
        {
            fd = rb->open(tcs.result, O_RDONLY);
            rb->get_metadata(&id3, fd, tcs.result);
            rb->close(fd);
        }
        if ( search_albumart_files(&id3, ":", buf, buflen) )
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
    unsigned char * buf_tmp = buf;
    size_t buf_tmp_size = buf_size;
    struct screen* display = rb->screens[0];
#if FB_DATA_SZ > 1
    ALIGN_BUFFER(buf_tmp, buf_tmp_size, sizeof(fb_data));
#endif
    struct bitmap logo = {
#if LCD_WIDTH < 200
        .width = 100,
        .height = 18,
#else
        .width = 193,
        .height = 34,
#endif
        .data = buf_tmp
    };
    int ret = rb->read_bmp_file(SPLASH_BMP, &logo, buf_tmp_size, FORMAT_NATIVE,
        NULL);
#if LCD_DEPTH > 1
    rb->lcd_set_background(N_BRIGHT(0));
    rb->lcd_set_foreground(N_BRIGHT(255));
#else
    rb->lcd_set_drawmode(PICTUREFLOW_DRMODE);
#endif
    rb->lcd_clear_display();

    if (ret > 0)
    {
#if LCD_DEPTH == 1  /* Mono LCDs need the logo inverted */
        rb->lcd_set_drawmode(PICTUREFLOW_DRMODE ^ DRMODE_INVERSEVID);
#endif
        display->bitmap(logo.data, (LCD_WIDTH - logo.width) / 2, 10,
            logo.width, logo.height);
#if LCD_DEPTH == 1  /* Mono LCDs need the logo inverted */
        rb->lcd_set_drawmode(PICTUREFLOW_DRMODE);
#endif
    }

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

        input_bmp.data = buf;
        input_bmp.width = DISPLAY_WIDTH;
        input_bmp.height = DISPLAY_HEIGHT;
        ret = read_image_file(albumart_file, &input_bmp,
                              buf_size, format, &format_transposed);
        if (ret <= 0) {
            rb->splash(HZ, "Could not read bmp");
            continue; /* skip missing/broken files */
        }
        if (!save_pfraw(pfraw_file, &input_bmp))
        {
            rb->splash(HZ, "Could not write bmp");
        }
        slides++;
        if ( rb->button_get(false) == PF_MENU ) return false;
    }
    if ( slides == 0 ) {
        /* Warn the user that we couldn't find any albumart */
        rb->splash(2*HZ, "No album art found");
        return false;
    }
    return true;
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
        while ( load_new_slide() ) {
            rb->yield();
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
                               IF_PRIO(, MAX(PRIORITY_USER_INTERFACE / 2,
                                       PRIORITY_REALTIME + 1))
                               IF_COP(, CPU)
                                      )
        ) == 0) {
        return false;
    }
    thread_is_running = true;
    rb->queue_post(&thread_q, EV_WAKEUP, 0);
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
    int fh = rb->creat( filename , 0666);
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


/*
 * The following functions implement the linked-list-in-array used to manage
 * the LRU cache of slides, and the list of free cache slots.
 */

#define seek_right_while(start, cond) \
({ \
    int ind_, next_ = (start); \
    do { \
        ind_ = next_; \
        next_ = cache[ind_].next; \
    } while (next_ != cache_used && (cond)); \
    ind_; \
})

#define seek_left_while(start, cond) \
({ \
    int ind_, next_ = (start); \
    do { \
        ind_ = next_; \
        next_ = cache[ind_].prev; \
    } while (ind_ != cache_used && (cond)); \
    ind_; \
})

/**
 Pop the given item from the linked list starting at *head, returning the next
 item, or -1 if the list is now empty.
*/
static inline int lla_pop_item (int *head, int i)
{
    int prev = cache[i].prev;
    int next = cache[i].next;
    if (i == next)
    {
        *head = -1;
        return -1;
    }
    else if (i == *head)
        *head = next;
    cache[next].prev = prev;
    cache[prev].next = next;
    return next;
}


/**
 Pop the head item from the list starting at *head, returning the index of the
 item, or -1 if the list is already empty.
*/
static inline int lla_pop_head (int *head)
{
    int i = *head;
    if (i != -1)
        lla_pop_item(head, i);
    return i;
}

/**
 Insert the item at index i before the one at index p.
*/
static inline void lla_insert (int i, int p)
{
    int next = p;
    int prev = cache[next].prev;
    cache[next].prev = i;
    cache[prev].next = i;
    cache[i].next = next;
    cache[i].prev = prev;
}


/**
 Insert the item at index i at the end of the list starting at *head.
*/
static inline void lla_insert_tail (int *head, int i)
{
    if (*head == -1)
    {
        *head = i;
        cache[i].next = i;
        cache[i].prev = i;
    } else
        lla_insert(i, *head);
}

/**
 Insert the item at index i before the one at index p.
*/
static inline void lla_insert_after(int i, int p)
{
    p = cache[p].next;
    lla_insert(i, p);
}


/**
 Insert the item at index i before the one at index p in the list starting at
 *head
*/
static inline void lla_insert_before(int *head, int i, int p)
{
    lla_insert(i, p);
    if (*head == p)
        *head = i;
}


/**
 Free the used slide at index i, and its buffer, and move it to the free
 slides list.
*/
static inline void free_slide(int i)
{
    if (cache[i].hid != empty_slide_hid)
        buflib_free(&buf_ctx, cache[i].hid);
    cache[i].index = -1;
    lla_pop_item(&cache_used, i);
    lla_insert_tail(&cache_free, i);
    if (cache_used == -1)
    {
        cache_right_index = -1;
        cache_left_index = -1;
        cache_center_index = -1;
    }
}


/**
 Free one slide ranked above the given priority. If no such slide can be found,
 return false.
*/
static bool free_slide_prio(int prio)
{
    if (cache_used == -1)
        return false;
    int i, l = cache_used, r = cache[cache_used].prev, prio_max;
    int prio_l = cache[l].index < center_index ?
           center_index - cache[l].index : 0;
    int prio_r = cache[r].index > center_index ?
           cache[r].index - center_index : 0;
    if (prio_l > prio_r)
    {
        i = l;
        prio_max = prio_l;
    } else {
        i = r;
        prio_max = prio_r;
    }
    if (prio_max > prio)
    {
        if (i == cache_left_index)
            cache_left_index = cache[i].next;
        if (i == cache_right_index)
            cache_right_index = cache[i].prev;
        free_slide(i);
        return true;
    } else
        return false;
}

/**
 Read the pfraw image given as filename and return the hid of the buffer
 */
int read_pfraw(char* filename, int prio)
{
    struct pfraw_header bmph;
    int fh = rb->open(filename, O_RDONLY);
    if( fh < 0 )
        return empty_slide_hid;
    else
        rb->read(fh, &bmph, sizeof(struct pfraw_header));

    int size =  sizeof(struct bitmap) + sizeof( pix_t ) *
                bmph.width * bmph.height;

    int hid;
    while (!(hid = buflib_alloc(&buf_ctx, size)) && free_slide_prio(prio));

    if (!hid) {
        rb->close( fh );
        return 0;
    }

    struct dim *bm = buflib_get_data(&buf_ctx, hid);

    bm->width = bmph.width;
    bm->height = bmph.height;
    pix_t *data = (pix_t*)(sizeof(struct dim) + (char *)bm);

    int y;
    for( y = 0; y < bm->height; y++ )
    {
        rb->read( fh, data , sizeof( pix_t ) * bm->width );
        data += bm->width;
    }
    rb->close( fh );
    return hid;
}


/**
  Load the surface for the given slide_index into the cache at cache_index.
 */
static inline bool load_and_prepare_surface(const int slide_index,
                                            const int cache_index,
                                            const int prio)
{
    char tmp_path_name[MAX_PATH+1];
    rb->snprintf(tmp_path_name, sizeof(tmp_path_name), CACHE_PREFIX "/%d.pfraw",
                 slide_index);

    int hid = read_pfraw(tmp_path_name, prio);
    if (!hid)
        return false;

    cache[cache_index].hid = hid;

    if ( cache_index < SLIDE_CACHE_SIZE ) {
        cache[cache_index].index = slide_index;
    }

    return true;
}


/**
 Load the "next" slide that we can load, freeing old slides if needed, provided
 that they are further from center_index than the current slide
*/
bool load_new_slide(void)
{
    int i = -1;
    if (cache_center_index != -1)
    {
        int next, prev;
        if (cache[cache_center_index].index != center_index)
        {
            if (cache[cache_center_index].index < center_index)
            {
                cache_center_index = seek_right_while(cache_center_index,
                                       cache[next_].index <= center_index);
                prev = cache_center_index;
                next = cache[cache_center_index].next;
            }
            else
            {
                cache_center_index = seek_left_while(cache_center_index,
                                      cache[next_].index >= center_index);
                next = cache_center_index;
                prev = cache[cache_center_index].prev;
            }
            if (cache[cache_center_index].index != center_index)
            {
                if (cache_free == -1)
                    free_slide_prio(0);
                i = lla_pop_head(&cache_free);
                if (!load_and_prepare_surface(center_index, i, 0))
                    goto fail_and_refree;
                if (cache[next].index == -1)
                {
                    if (cache[prev].index == -1)
                        goto insert_first_slide;
                    else
                        next = cache[prev].next;
                }
                lla_insert(i, next);
                if (cache[i].index < cache[cache_used].index)
                    cache_used = i;
                cache_center_index = i;
                cache_left_index = i;
                cache_right_index = i;
                return true;
            }
        }
        if (cache[cache_left_index].index >
                cache[cache_center_index].index)
            cache_left_index = cache_center_index;
        if (cache[cache_right_index].index <
                cache[cache_center_index].index)
            cache_right_index = cache_center_index;
        cache_left_index = seek_left_while(cache_left_index,
                   cache[ind_].index - 1 == cache[next_].index);
        cache_right_index = seek_right_while(cache_right_index,
                   cache[ind_].index - 1 == cache[next_].index);
        int prio_l = cache[cache_center_index].index -
                     cache[cache_left_index].index + 1;
        int prio_r = cache[cache_right_index].index -
                     cache[cache_center_index].index + 1;
        if ((prio_l < prio_r ||
             cache[cache_right_index].index >= number_of_slides) &&
             cache[cache_left_index].index > 0)
        {
            if (cache_free == -1 && !free_slide_prio(prio_l))
                return false;
            i = lla_pop_head(&cache_free);
            if (load_and_prepare_surface(cache[cache_left_index].index
                                         - 1, i, prio_l))
            {
                lla_insert_before(&cache_used, i, cache_left_index);
                cache_left_index = i;
                return true;
            }
        } else if(cache[cache_right_index].index < number_of_slides - 1)
        {
            if (cache_free == -1 && !free_slide_prio(prio_r))
                return false;
            i = lla_pop_head(&cache_free);
            if (load_and_prepare_surface(cache[cache_right_index].index
                                         + 1, i, prio_r))
            {
                lla_insert_after(i, cache_right_index);
                cache_right_index = i;
                return true;
            }
        }
    } else {
        i = lla_pop_head(&cache_free);
        if (load_and_prepare_surface(center_index, i, 0))
        {
insert_first_slide:
            cache[i].next = i;
            cache[i].prev = i;
            cache_center_index = i;
            cache_left_index = i;
            cache_right_index = i;
            cache_used = i;
            return true;
        }
    }
fail_and_refree:
    if (i != -1)
    {
        lla_insert_tail(&cache_free, i);
    }
    return false;
}


/**
  Get a slide from the buffer
 */
static inline struct dim *get_slide(const int hid)
{
    if (!hid)
        return NULL;

    struct dim *bmp;

    bmp = buflib_get_data(&buf_ctx, hid);

    return bmp;
}


/**
 Return the requested surface
*/
static inline struct dim *surface(const int slide_index)
{
    if (slide_index < 0)
        return 0;
    if (slide_index >= number_of_slides)
        return 0;
    int i;
    if ((i = cache_used ) != -1)
    {
        do {
            if (cache[i].index == slide_index)
                return get_slide(cache[i].hid);
            i = cache[i].next;
        } while (i != cache_used);
    }
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
 *
 * To calculate the offset that will provide the proper margin, we use the same
 * projection used to render the slides. The solution for xc, the slide center,
 * is:
 *                         xp * (zo + xs * sin(r))
 * xc = xp - xs * cos(r) + ───────────────────────
 *                                    z
 * TODO: support moving the side slides toward or away from the camera
 */
void recalc_offsets(void)
{
    PFreal xs = PFREAL_HALF - DISPLAY_WIDTH * PFREAL_HALF;
    PFreal zo;
    PFreal xp = (DISPLAY_WIDTH * PFREAL_HALF - PFREAL_HALF + center_margin *
        PFREAL_ONE) * zoom / 100;
    PFreal cosr, sinr;

    itilt = 70 * IANGLE_MAX / 360;      /* approx. 70 degrees tilted */
    cosr = fcos(-itilt);
    sinr = fsin(-itilt);
    zo = CAM_DIST_R * 100 / zoom - CAM_DIST_R +
        fmuln(MAXSLIDE_LEFT_R, sinr, PFREAL_SHIFT - 2, 0);
    offsetX = xp - fmul(xs, cosr) + fmuln(xp,
        zo + fmuln(xs, sinr, PFREAL_SHIFT - 2, 0), PFREAL_SHIFT - 2, 0)
        / CAM_DIST;
    offsetY = DISPLAY_WIDTH / 2 * (fsin(itilt) + PFREAL_ONE / 2);
}


/**
   Fade the given color by spreading the fb_data (ushort)
   to an uint, multiply and compress the result back to a ushort.
 */
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
static inline unsigned fade_color(pix_t c, unsigned a)
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
static inline unsigned fade_color(pix_t c, unsigned a)
{
    unsigned int result;
    a = (a + 2) & 0x1fc;
    result = ((c & 0xf81f) * a) & 0xf81f00;
    result |= ((c & 0x7e0) * a) & 0x7e000;
    result >>= 8;
    return result;
}
#else
static inline unsigned fade_color(pix_t c, unsigned a)
{
    unsigned val = c;
    return MULUQ(val, a) >> 8;
}
#endif

/**
 * Render a single slide
 * Where xc is the slide's horizontal offset from center, xs is the horizontal
 * on the slide from its center, zo is the slide's depth offset from the plane
 * of the display, r is the angle at which the slide is tilted, and xp is the
 * point on the display corresponding to xs on the slide, the projection
 * formulas are:
 *
 *      z * (xc + xs * cos(r))
 * xp = ──────────────────────
 *       z + zo + xs * sin(r)
 *
 *      z * (xc - xp) - xp * zo
 * xs = ────────────────────────
 *      xp * sin(r) - z * cos(r)
 *
 * We use the xp projection once, to find the left edge of the slide on the
 * display. From there, we use the xs reverse projection to find the horizontal
 * offset from the slide center of each column on the screen, until we reach
 * the right edge of the slide, or the screen. The reverse projection can be
 * optimized by saving the numerator and denominator of the fraction, which can
 * then be incremented by (z + zo) and sin(r) respectively.
 */
void render_slide(struct slide_data *slide, const int alpha)
{
    struct dim *bmp = surface(slide->slide_index);
    if (!bmp) {
        return;
    }
    if (slide->angle > 255 || slide->angle < -255)
        return;
    pix_t *src = (pix_t*)(sizeof(struct dim) + (char *)bmp);

    const int sw = bmp->width;
    const int sh = bmp->height;
    const PFreal slide_left = -sw * PFREAL_HALF + PFREAL_HALF;
    const int w = LCD_WIDTH;

    uint8_t reftab[REFLECT_HEIGHT]; /* on stack, which is in IRAM on several targets */

    if (alpha == 256) { /* opaque -> copy table */
        rb->memcpy(reftab, reflect_table, sizeof(reftab));
    } else {            /* precalculate faded table */
        int i, lalpha;
        for (i = 0; i < REFLECT_HEIGHT; i++) {
            lalpha = reflect_table[i];
            reftab[i] = (MULUQ(lalpha, alpha) + 129) >> 8;
        }
    }

    PFreal cosr = fcos(slide->angle);
    PFreal sinr = fsin(slide->angle);
    PFreal zo = PFREAL_ONE * slide->distance + CAM_DIST_R * 100 / zoom
        - CAM_DIST_R - fmuln(MAXSLIDE_LEFT_R, fabs(sinr), PFREAL_SHIFT - 2, 0);
    PFreal xs = slide_left, xsnum, xsnumi, xsden, xsdeni;
    PFreal xp = fdiv(CAM_DIST * (slide->cx + fmul(xs, cosr)),
        (CAM_DIST_R + zo + fmul(xs,sinr)));

    /* Since we're finding the screen position of the left edge of the slide,
     * we round up.
     */
    int xi = (fmax(DISPLAY_LEFT_R, xp) - DISPLAY_LEFT_R + PFREAL_ONE - 1)
        >> PFREAL_SHIFT;
    xp = DISPLAY_LEFT_R + xi * PFREAL_ONE;
    if (xi >= w) {
        return;
    }
    xsnum = CAM_DIST * (slide->cx - xp) - fmuln(xp, zo, PFREAL_SHIFT - 2, 0);
    xsden = fmuln(xp, sinr, PFREAL_SHIFT - 2, 0) - CAM_DIST * cosr;
    xs = fdiv(xsnum, xsden);

    xsnumi = -CAM_DIST_R - zo;
    xsdeni = sinr;
    int x;
    int dy = PFREAL_ONE;
    for (x = xi; x < w; x++) {
        int column = (xs - slide_left) / PFREAL_ONE;
        if (column >= sw)
            break;
        if (zo || slide->angle)
            dy = (CAM_DIST_R + zo + fmul(xs, sinr)) / CAM_DIST;

        const pix_t *ptr = &src[column * bmp->height];
        
#if   defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
#define PIXELSTEP_Y   1
#define LCDADDR(x, y) (&buffer[BUFFER_HEIGHT*(x) + (y)])
#else
#define PIXELSTEP_Y   BUFFER_WIDTH
#define LCDADDR(x, y) (&buffer[(y)*BUFFER_WIDTH + (x)])
#endif

        int p = (bmp->height-1-DISPLAY_OFFS) * PFREAL_ONE;
        int plim = MAX(0, p - (LCD_HEIGHT/2-1) * dy);
        pix_t *pixel = LCDADDR(x, (LCD_HEIGHT/2)-1 );

        if (alpha == 256) {
            while (p >= plim) {
                *pixel = ptr[((unsigned)p) >> PFREAL_SHIFT];
                p -= dy;
                pixel -= PIXELSTEP_Y;
            }
        } else {
            while (p >= plim) {
                *pixel = fade_color(ptr[((unsigned)p) >> PFREAL_SHIFT], alpha);
                p -= dy;
                pixel -= PIXELSTEP_Y;
            }
        }
        p = (bmp->height-DISPLAY_OFFS) * PFREAL_ONE;
        plim = MIN(sh * PFREAL_ONE, p + (LCD_HEIGHT/2) * dy);
        int plim2 = MIN(MIN(sh + REFLECT_HEIGHT, sh * 2) * PFREAL_ONE,
                        p + (LCD_HEIGHT/2) * dy);
        pixel = LCDADDR(x, (LCD_HEIGHT/2) );

        if (alpha == 256) {
            while (p < plim) {
                *pixel = ptr[((unsigned)p) >> PFREAL_SHIFT];
                p += dy;
                pixel += PIXELSTEP_Y;
            }
        } else {
            while (p < plim) {
                *pixel = fade_color(ptr[((unsigned)p) >> PFREAL_SHIFT], alpha);
                p += dy;
                pixel += PIXELSTEP_Y;
            }
        }
        while (p < plim2) {
            int ty = (((unsigned)p) >> PFREAL_SHIFT) - sh;
            int lalpha = reftab[ty];
            *pixel = fade_color(ptr[sh - 1 - ty], lalpha);
            p += dy;
            pixel += PIXELSTEP_Y;
        }

        if (zo || slide->angle)
        {
            xsnum += xsnumi;
            xsden += xsdeni;
            xs = fdiv(xsnum, xsden);
        } else
            xs += PFREAL_ONE;

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
    int old_center_index = center_index;
    step = 0;
    center_index = fbound(slide_index, 0, number_of_slides - 1);
    if (old_center_index != center_index)
        rb->queue_post(&thread_q, EV_WAKEUP, 0);
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
        rb->queue_post(&thread_q, EV_WAKEUP, 0);
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
    int i;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    end_pf_thread();
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

#ifdef USEGSLIB
    grey_release();
#endif
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);
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
        input_bmp.data = (char*)buf;
        ret = scaled_read_bmp_file(EMPTY_SLIDE_BMP, &input_bmp,
                                buf_size,
                                FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT,
                                &format_transposed);
        if (!save_pfraw(EMPTY_SLIDE, &input_bmp))
            return false;
    }

    return true;
}

/**
  Shows the settings menu
 */
int settings_menu(void)
{
    int selection = 0;
    bool old_val;

    MENUITEM_STRINGLIST(settings_menu, "PictureFlow Settings", NULL, "Show FPS",
                        "Spacing", "Centre margin", "Number of slides", "Zoom",
                        "Show album title", "Resize Covers", "Rebuild cache");

    static const struct opt_items album_name_options[] = {
        { "Hide album title", -1 },
        { "Show at the bottom", -1 },
        { "Show at the top", -1 }
    };

    do {
        selection=rb->do_menu(&settings_menu,&selection, NULL, true);
        switch(selection) {
            case 0:
                rb->set_bool("Show FPS", &show_fps);
                reset_track_list();
                break;

            case 1:
                rb->set_int("Spacing between slides", "", 1,
                            &slide_spacing,
                            NULL, 1, 0, 100, NULL );
                recalc_offsets();
                reset_slides();
                break;

            case 2:
                rb->set_int("Centre margin", "", 1,
                            &center_margin,
                            NULL, 1, 0, 80, NULL );
                recalc_offsets();
                reset_slides();
                break;

            case 3:
                rb->set_int("Number of slides", "", 1, &num_slides,
                            NULL, 1, 1, MAX_SLIDES_COUNT, NULL );
                recalc_offsets();
                reset_slides();
                break;

            case 4:
                rb->set_int("Zoom", "", 1, &zoom,
                            NULL, 1, 10, 300, NULL );
                recalc_offsets();
                reset_slides();
                break;
            case 5:
                rb->set_option("Show album title", &show_album_name,
                               INT, album_name_options, 3, NULL);
                reset_track_list();
                recalc_offsets();
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
enum {
    PF_GOTO_WPS,
#if PF_PLAYBACK_CAPABLE
    PF_MENU_PLAYBACK_CONTROL,
#endif
    PF_MENU_SETTINGS,
    PF_MENU_RETURN,
    PF_MENU_QUIT,
};

int main_menu(void)
{
    int selection = 0;
    int result;

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif

    MENUITEM_STRINGLIST(main_menu,"PictureFlow Main Menu",NULL,
                        "Go to WPS",
#if PF_PLAYBACK_CAPABLE
                        "Playback Control",
#endif
                                            "Settings", "Return", "Quit");
    while (1)  {
        switch (rb->do_menu(&main_menu,&selection, NULL, true)) {
            case PF_GOTO_WPS: /* WPS */
                return -2;
#if PF_PLAYBACK_CAPABLE
            case PF_MENU_PLAYBACK_CONTROL: /* Playback Control */
                playback_control(NULL);
                break;
#endif
            case PF_MENU_SETTINGS:
                result = settings_menu();
                if ( result != 0 ) return result;
                break;
            case PF_MENU_RETURN:
                return 0;
            case PF_MENU_QUIT:
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
#ifdef HAVE_LCD_COLOR
        MYLCD(set_foreground)(G_PIX(c2+80-(c >> 9), c2+100-(c >> 9),
                                           c2+250-(c >> 8)));
#else
        MYLCD(set_foreground)(G_BRIGHT(c2+160-(c >> 8)));
#endif
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

#if PF_PLAYBACK_CAPABLE
/*
 * Puts the current tracklist into a newly created playlist and starts playling
 */
void start_playback(void)
{
    static int old_playlist = -1, old_shuffle = 0;
    int count = 0;
    int position = selected_track;
    int shuffle = rb->global_settings->playlist_shuffle;
    /* reuse existing playlist if possible
     * regenerate if shuffle is on or changed, since playlist index and
     * selected track are "out of sync" */
    if (!shuffle && center_slide.slide_index == old_playlist
            && (old_shuffle == shuffle))
    {
        goto play;
    }
    /* First, replace the current playlist with a new one */
    else if (rb->playlist_remove_all_tracks(NULL) == 0
            && rb->playlist_create(NULL, NULL) == 0)
    {
        do {
            rb->yield();
            if (rb->playlist_insert_track(NULL, get_track_filename(count),
                    PLAYLIST_INSERT_LAST, false, true) < 0)
                break;
        } while(++count < track_count);
        rb->playlist_sync(NULL);
    }
    else
        return;

    if (rb->global_settings->playlist_shuffle)
        position = rb->playlist_shuffle(*rb->current_tick, selected_track);
play:
    /* TODO: can we adjust selected_track if !play_selected ?
     * if shuffle, we can't predict the playing track easily, and for either
     * case the track list doesn't get auto scrolled*/
    rb->playlist_start(position, 0);
    old_playlist = center_slide.slide_index;
    old_shuffle = shuffle;
}
#endif

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
  Display an error message and wait for input.
*/
void error_wait(const char *message)
{
    rb->splashf(0, "%s. Press any button to continue.", message);
    while (rb->get_action(CONTEXT_STD, 1) == ACTION_NONE)
        rb->yield();
    rb->sleep(2 * HZ);
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
            error_wait("Could not create directory " CACHE_PREFIX);
            return PLUGIN_ERROR;
        }
    }

    configfile_load(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);

    init_reflect_table();

    ALIGN_BUFFER(buf, buf_size, 4);
    ret = create_album_index();
    if (ret == ERROR_BUFFER_FULL) {
        error_wait("Not enough memory for album names");
        return PLUGIN_ERROR;
    } else if (ret == ERROR_NO_ALBUMS) {
        error_wait("No albums found. Please enable database");
        return PLUGIN_ERROR;
    }

    ALIGN_BUFFER(buf, buf_size, 4);
    number_of_slides  = album_count;
    if ((cache_version != CACHE_VERSION) && !create_albumart_cache()) {
        error_wait("Could not create album art cache");
        return PLUGIN_ERROR;
    }

    if (!create_empty_slide(cache_version != CACHE_VERSION)) {
        error_wait("Could not load the empty slide");
        return PLUGIN_ERROR;
    }
    cache_version = CACHE_VERSION;
    configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);


#ifdef USEGSLIB
    long grey_buf_used;
    if (!grey_init(buf, buf_size, GREY_BUFFERED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, &grey_buf_used))
    {
        error_wait("Greylib init failed!");
        return PLUGIN_ERROR;
    }
    grey_setfont(FONT_UI);
    buf_size -= grey_buf_used;
    buf = (void*)(grey_buf_used + (char*)buf);
#endif
    buflib_init(&buf_ctx, (void *)buf, buf_size);

    if (!(empty_slide_hid = read_pfraw(EMPTY_SLIDE, 0)))
    {
        error_wait("Unable to load empty slide image");
        return PLUGIN_ERROR;
    }

    if (!create_pf_thread()) {
        error_wait("Cannot create thread!");
        return PLUGIN_ERROR;
    }

    int i;

    /* initialize */
    for (i = 0; i < SLIDE_CACHE_SIZE; i++) {
        cache[i].hid = 0;
        cache[i].index = 0;
        cache[i].next = i + 1;
        cache[i].prev = i - 1;
    }
    cache[0].prev = i - 1;
    cache[i - 1].next = 0;
    cache_free = 0;
    buffer = LCD_BUF;

    pf_state = pf_idle;

    track_index = -1;
    extra_fade = 0;
    slide_frame = 0;
    step = 0;
    target = 0;
    fade = 256;

    recalc_offsets();
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
        button = rb->get_custom_action(CONTEXT_PLUGIN
#ifndef USE_CORE_PREVNEXT
            |(pf_state == pf_show_tracks ? 1 : 0)
#endif
            ,instant_update ? 0 : HZ/16,
            get_context_map);

        switch (button) {
        case PF_QUIT:
            return PLUGIN_OK;
        case PF_WPS:
            return PLUGIN_GOTO_WPS;
        case PF_BACK:
            if ( pf_state == pf_show_tracks )
            {
                buflib_buffer_in(&buf_ctx, borrowed);
                borrowed = 0;
                track_index = -1;
                pf_state = pf_cover_out;
            }
            if (pf_state == pf_idle || pf_state == pf_scrolling)
                return PLUGIN_OK;
            break;
        case PF_MENU:
#ifdef USEGSLIB
            grey_show(false);
#endif
            ret = main_menu();
            if ( ret == -2 ) return PLUGIN_GOTO_WPS;
            if ( ret == -1 ) return PLUGIN_OK;
            if ( ret != 0 ) return ret;
#ifdef USEGSLIB
            grey_show(true);
#endif
            MYLCD(set_drawmode)(DRMODE_FG);
            break;

        case PF_NEXT:
        case PF_NEXT_REPEAT:
            if ( pf_state == pf_show_tracks )
                select_next_track();
            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_next_slide();
            break;

        case PF_PREV:
        case PF_PREV_REPEAT:
            if ( pf_state == pf_show_tracks )
                select_prev_track();
            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_previous_slide();
            break;

        case PF_SELECT:
            if ( pf_state == pf_idle ) {
                pf_state = pf_cover_in;
            }
            else if ( pf_state == pf_show_tracks ) {
#if PF_PLAYBACK_CAPABLE
                start_playback();
#endif
            }
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
    int ret, i;
    (void) parameter;

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, false, NULL);
    /* Turn off backlight timeout */
    backlight_force_on();     /* backlight control in lib/helper.c */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
#if PF_PLAYBACK_CAPABLE
    buf = rb->plugin_get_buffer(&buf_size);
#else
    buf = rb->plugin_get_audio_buffer(&buf_size);
#ifndef SIMULATOR
    if ((uintptr_t)buf < (uintptr_t)plugin_start_addr)
    {
        uint32_t tmp_size = (uintptr_t)plugin_start_addr - (uintptr_t)buf;
        buf_size = MIN(buf_size, tmp_size);
    }
#endif
#endif
    ret = main();
    if ( ret == PLUGIN_OK ) {
        if (configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS,
                            CONFIG_VERSION))
        {
            rb->splash(HZ, "Error writing config.");
            ret = PLUGIN_ERROR;
        }
    }

    cleanup(NULL);
    return ret;
}
