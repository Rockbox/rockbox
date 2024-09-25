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
#include "albumart.h"
#include "lib/read_image.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/helper.h"
#include "lib/configfile.h"
#include "lib/grey.h"
#include "lib/mylcd.h"
#include "lib/feature_wrappers.h"
#include "lib/id3.h"

/******************************* Globals ***********************************/
static fb_data *lcd_fb;

/*
 *  Targets which use plugin_get_audio_buffer() can't have playback from
 * within pictureflow itself, as the whole core audio buffer is occupied */
#define PF_PLAYBACK_CAPABLE (PLUGIN_BUFFER_SIZE > 0x10000)

#if PF_PLAYBACK_CAPABLE
#include "lib/playback_control.h"
#include "lib/mul_id3.h"
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
#define PF_JMP ACTION_LISTTREE_PGDOWN
#define PF_JMP_PREV ACTION_LISTTREE_PGUP

#define PF_QUIT (LAST_ACTION_PLACEHOLDER + 1)
#define PF_TRACKLIST (LAST_ACTION_PLACEHOLDER + 2)
#define PF_SORTING_NEXT (LAST_ACTION_PLACEHOLDER + 3)
#define PF_SORTING_PREV (LAST_ACTION_PLACEHOLDER + 4)

#if defined(HAVE_SCROLLWHEEL) || CONFIG_KEYPAD == IRIVER_H10_PAD || \
    CONFIG_KEYPAD == MPIO_HD300_PAD
#if (CONFIG_KEYPAD != IPOD_1G2G_PAD) \
    && (CONFIG_KEYPAD != IPOD_3G_PAD) \
    && (CONFIG_KEYPAD != IPOD_4G_PAD) \
    && (CONFIG_KEYPAD != FIIO_M3K_PAD)
#define USE_CORE_PREVNEXT
#endif
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
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD) \
    || (CONFIG_KEYPAD == FIIO_M3K_PAD)
    {PF_JMP_PREV,     BUTTON_LEFT,                BUTTON_NONE},
    {PF_JMP_PREV,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    {PF_JMP,          BUTTON_RIGHT,               BUTTON_NONE},
    {PF_JMP,          BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT},
#elif defined(BUTTON_LEFT) && defined(BUTTON_RIGHT)
    {PF_PREV,         BUTTON_LEFT,                BUTTON_NONE},
    {PF_PREV_REPEAT,  BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    {PF_NEXT,         BUTTON_RIGHT,               BUTTON_NONE},
    {PF_NEXT_REPEAT,  BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REL,     BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REL,    BUTTON_RIGHT},
    {ACTION_NONE,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_LEFT},
    {ACTION_NONE,     BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_RIGHT},
#else
#warning "LEFT/RIGHT not defined!"
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
#if CONFIG_KEYPAD == CREATIVEZV_PAD || CONFIG_KEYPAD == CREATIVEZVM_PAD || \
    CONFIG_KEYPAD == PHILIPS_HDD1630_PAD || \
    CONFIG_KEYPAD == GIGABEAT_PAD || CONFIG_KEYPAD == GIGABEAT_S_PAD || \
    CONFIG_KEYPAD == MROBE100_PAD || CONFIG_KEYPAD == MROBE500_PAD || \
    CONFIG_KEYPAD == PHILIPS_SA9200_PAD || CONFIG_KEYPAD == SANSA_CLIP_PAD || \
    CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD || CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD || \
    CONFIG_KEYPAD == XDUOO_X3_PAD
    {PF_QUIT,         BUTTON_POWER,               BUTTON_NONE},
#if CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
    {PF_MENU,         BUTTON_SELECT|BUTTON_REPEAT,  BUTTON_SELECT},
#endif
#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
    {PF_QUIT,         BUTTON_HOME|BUTTON_REPEAT,  BUTTON_NONE},
    {PF_TRACKLIST,    BUTTON_RIGHT,  BUTTON_NONE},
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
#elif (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
    || (CONFIG_KEYPAD == IPOD_3G_PAD) \
    || (CONFIG_KEYPAD == IPOD_4G_PAD)
    {PF_MENU,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
    {PF_QUIT,         BUTTON_MENU|BUTTON_REL,     BUTTON_MENU},
    {PF_SORTING_NEXT, BUTTON_SELECT|BUTTON_MENU,  BUTTON_NONE},
    {PF_SORTING_PREV, BUTTON_SELECT|BUTTON_PLAY,  BUTTON_NONE},
#elif CONFIG_KEYPAD == MPIO_HD300_PAD
    {PF_QUIT,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
    {PF_QUIT,         BUTTON_RC_REC,              BUTTON_NONE},
#elif CONFIG_KEYPAD == MEIZU_M6SL_PAD
    {PF_QUIT,         BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
#elif CONFIG_KEYPAD == IRIVER_H100_PAD || CONFIG_KEYPAD == IRIVER_H300_PAD
    {PF_QUIT,         BUTTON_OFF,                 BUTTON_NONE},
#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
    {PF_QUIT,         BUTTON_REC,                 BUTTON_NONE},
#elif CONFIG_KEYPAD == SAMSUNG_YH820_PAD || CONFIG_KEYPAD == SAMSUNG_YH92X_PAD
    {PF_QUIT,         BUTTON_REW|BUTTON_REPEAT,   BUTTON_REW},
    {PF_MENU,         BUTTON_REW|BUTTON_REL,      BUTTON_REW},
    {PF_SELECT,       BUTTON_PLAY|BUTTON_REL,     BUTTON_PLAY},
    {PF_CONTEXT,      BUTTON_FFWD|BUTTON_REPEAT,  BUTTON_FFWD},
    {PF_TRACKLIST,    BUTTON_FFWD|BUTTON_REL,     BUTTON_FFWD},
    {PF_WPS,          BUTTON_PLAY|BUTTON_REPEAT,  BUTTON_PLAY},
#elif CONFIG_KEYPAD == FIIO_M3K_PAD
    {PF_JMP_PREV,     BUTTON_LEFT,                BUTTON_NONE},
    {PF_JMP_PREV,     BUTTON_LEFT|BUTTON_REPEAT,  BUTTON_NONE},
    {PF_JMP,          BUTTON_RIGHT,               BUTTON_NONE},
    {PF_JMP,          BUTTON_RIGHT|BUTTON_REPEAT, BUTTON_NONE},
    {PF_MENU,         BUTTON_POWER|BUTTON_REL,    BUTTON_POWER},
    {PF_SORTING_NEXT, BUTTON_VOL_UP,              BUTTON_NONE},
    {PF_SORTING_PREV, BUTTON_VOL_DOWN,            BUTTON_NONE},
    {PF_QUIT,         BUTTON_POWER|BUTTON_REPEAT, BUTTON_POWER},
    {PF_CONTEXT,      BUTTON_MENU|BUTTON_REL,     BUTTON_MENU},
    {PF_TRACKLIST,    BUTTON_MENU|BUTTON_REPEAT,  BUTTON_MENU},
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
#define G_PIX(r,g,b) \
    (77 * (unsigned)(r) + 150 * (unsigned)(g) + 29 * (unsigned)(b)) / 256
#define N_PIX(r,g,b) N_BRIGHT(G_PIX(r,g,b))
#define G_BRIGHT(y) (y)
#define BUFFER_WIDTH _grey_info.width
#define BUFFER_HEIGHT _grey_info.height
typedef unsigned char pix_t;
#else   /* LCD_DEPTH >= 8 */
#define LCD_BUF lcd_fb
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
#define CACHE_PREFIX PLUGIN_DEMOS_DATA_DIR "/pictureflow"
#define ALBUM_INDEX CACHE_PREFIX "/pictureflow_album.idx"

#define EV_EXIT 9999
#define EV_WAKEUP 1337

#define EMPTY_SLIDE CACHE_PREFIX "/emptyslide.pfraw"
#define EMPTY_SLIDE_BMP PLUGIN_DEMOS_DIR "/pictureflow_emptyslide.bmp"
#define SPLASH_BMP PLUGIN_DEMOS_DIR "/pictureflow_splash.bmp"

/* some magic numbers for cache_version. */
#define CACHE_REBUILD   0

/* Error return values */
#define SUCCESS              0
#define ERROR_NO_ALBUMS     -1
#define ERROR_BUFFER_FULL   -2
#define ERROR_NO_ARTISTS    -3
#define ERROR_USER_ABORT    -4

/* current version for cover cache */
#define CACHE_VERSION 4
#define CONFIG_VERSION 1
#define CONFIG_FILE "pictureflow.cfg"
#define INDEX_HDR "PFID"

/** structs we use */
struct pf_config_t
{
     /* config values */
     int slide_spacing;
     int center_margin;

     int num_slides;
     int zoom;

     int auto_wps;
     int last_album;

     int backlight_mode;
     int cache_version;

     int show_album_name;
     int sort_albums_by;
     int year_sort_order;
     bool show_year;

     bool resize;
     bool show_fps;

     bool update_albumart;
};

struct pf_index_t {
    uint32_t            header; /*INDEX_HDR*/
    uint16_t            artist_ct;
    uint16_t            album_ct;

    char               *artist_names;
    struct artist_data *artist_index;
    size_t              artist_len;

    unsigned int        album_untagged_idx;
    char               *album_names;
    struct album_data  *album_index;
    size_t              album_len;
    long                album_untagged_seek;

    void * buf;
    size_t buf_sz;
};

struct pf_track_t {
    int    count;
    int    cur_idx;
    int    sel;
    int    sel_pulse;
    int    last_sel;
    int    list_start;
    int    list_visible;
    int    list_y;
    int    list_h;
    size_t borrowed;
    size_t used;
    struct track_data *index;
    char  *names;
};

struct albumart_t {
    struct bitmap input_bmp;
    char pfraw_file[MAX_PATH];
    char file[MAX_PATH];
    int idx;
    int slides;
    int inspected;
    void * buf;
    size_t buf_sz;
};

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
    int name_idx;    /* offset to the album name */
    int artist_idx;  /* offset to the artist name */
    int year;        /* album year */
    long artist_seek; /* artist taglist position */
    long seek;        /* album taglist position */
};

struct artist_data {
    int name_idx; /* offset to the artist name */
    long seek;    /* artist taglist position */
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

struct pf_slide_cache
{
    struct slide_cache cache[SLIDE_CACHE_SIZE];
    int free;
    int used;
    int left_idx;
    int right_idx;
    int center_idx;
};

enum pf_scroll_line_type {
    PF_SCROLL_TRACK = 0,
    PF_SCROLL_ALBUM,
    PF_SCROLL_ARTIST,
    PF_MAX_SCROLL_LINES
};

struct pf_scroll_line_info {
    long ticks;         /* number of ticks between each move */
    long delay;         /* number of ticks to delay starting scrolling */
    int step;           /* pixels to move */
    long next_scroll;   /* tick of the next move */
};

struct pf_scroll_line {
    int width;          /* width of the string */
    int offset;         /* x coordinate of the string */
    int step;           /* 0 if scroll is disabled. otherwise, pixels to move */
    long start_tick;    /* tick when to start scrolling */
};

struct pfraw_header {
    int32_t width;          /* bmap width in pixels */
    int32_t height;         /* bmap height in pixels */
};

enum show_album_name_values {
    ALBUM_NAME_HIDE = 0,
    ALBUM_NAME_BOTTOM,
    ALBUM_NAME_TOP,
    ALBUM_AND_ARTIST_TOP,
    ALBUM_AND_ARTIST_BOTTOM
};
static char* show_album_name_conf[] =
{
    "hide",
    "bottom",
    "top",
    "both top",
    "both bottom",
};

enum sort_albums_by_values {
    SORT_BY_ARTIST_AND_NAME = 0,
    SORT_BY_ARTIST_AND_YEAR,
    SORT_BY_YEAR,
    SORT_BY_NAME,

    SORT_VALUES_SIZE
};
static char* sort_albums_by_conf[] =
{
    "artist + name",
    "artist + year",
    "year",
    "name"
};
enum year_sort_order_values {
    ASCENDING = 0,
    DESCENDING
};
static char* year_sort_order_conf[] =
{
    "ascending",
    "descending"
};

#define MAX_SPACING 40
#define MAX_MARGIN 80

static struct albumart_t aa_cache;
static struct pf_config_t pf_cfg;

static struct configdata config[] =
{
    { TYPE_INT, 0, MAX_SPACING, { .int_p = &pf_cfg.slide_spacing }, "slide spacing",
      NULL },
    { TYPE_INT, 0, MAX_MARGIN, { .int_p = &pf_cfg.center_margin }, "center margin",
      NULL },
    { TYPE_INT, 0, MAX_SLIDES_COUNT, { .int_p = &pf_cfg.num_slides }, "slides count",
      NULL },
    { TYPE_INT, 0, 300, { .int_p = &pf_cfg.zoom }, "zoom", NULL },
    { TYPE_BOOL, 0, 1, { .bool_p = &pf_cfg.show_fps }, "show fps", NULL },
    { TYPE_BOOL, 0, 1, { .bool_p = &pf_cfg.resize }, "resize", NULL },
    { TYPE_INT, 0, 100, { .int_p = &pf_cfg.cache_version }, "cache version", NULL },
    { TYPE_ENUM, 0, 5, { .int_p = &pf_cfg.show_album_name }, "show album name",
      show_album_name_conf },
    { TYPE_INT, 0, 2, { .int_p = &pf_cfg.auto_wps }, "auto wps", NULL },
    { TYPE_INT, 0, 999999, { .int_p = &pf_cfg.last_album }, "last album", NULL },
    { TYPE_INT, 0, 1, { .int_p = &pf_cfg.backlight_mode }, "backlight", NULL },
    { TYPE_INT, 0, 999999, { .int_p = &aa_cache.idx }, "art cache pos", NULL },
    { TYPE_INT, 0, 999999, { .int_p = &aa_cache.inspected }, "art cache inspected", NULL },
    { TYPE_ENUM, 0, 4, { .int_p = &pf_cfg.sort_albums_by }, "sort albums by",
      sort_albums_by_conf },
    { TYPE_ENUM, 0, 2, { .int_p = &pf_cfg.year_sort_order }, "year order",
      year_sort_order_conf },
    { TYPE_BOOL, 0, 1, { .bool_p = &pf_cfg.show_year }, "show year", NULL },
    { TYPE_BOOL, 0, 1, { .bool_p = &pf_cfg.update_albumart }, "update albumart", NULL }
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
static bool is_initial_slide = true;
static bool show_tracks_while_browsing = false;

static struct pf_slide_cache pf_sldcache;

/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE / sizeof(long)];
/* queue (as array) for scheduling load_surface */

static int empty_slide_hid;

unsigned int thread_id;
struct event_queue thread_q;

static struct tagcache_search tcs;

static struct buflib_context buf_ctx;

static struct pf_index_t pf_idx;

static struct pf_track_t pf_tracks;

static struct mp3entry id3;

void reset_track_list(void);

static bool thread_is_running;
static bool wants_to_quit = false;

/*
    Prevent picture loading thread from allocating
    buflib memory while the main thread may be
    performing buffer-shifting operations.
*/
static struct mutex buf_ctx_mutex;
static bool buf_ctx_locked = false;

static int cover_animation_keyframe;
static int extra_fade;

static struct pf_scroll_line_info scroll_line_info;
static struct pf_scroll_line scroll_lines[PF_MAX_SCROLL_LINES];

enum ePFS{ePFS_ARTIST = 0, ePFS_ALBUM};
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

#if PF_PLAYBACK_CAPABLE
static bool insert_whole_album;
static bool old_shuffle = false;
static int old_playlist = -1;
#endif

/** code */
static bool free_slide_prio(int prio);
bool load_new_slide(void);
int load_surface(int);
static void draw_progressbar(int step, int count, char *msg);
static void draw_splashscreen(unsigned char * buf_tmp, size_t buf_tmp_size);
static void free_all_slide_prio(int prio);

static inline void buf_ctx_lock(void)
{
    rb->mutex_lock(&buf_ctx_mutex);
    buf_ctx_locked = true;
}

static inline void buf_ctx_unlock(void)
{
    rb->mutex_unlock(&buf_ctx_mutex);
    buf_ctx_locked = false;
}

static bool check_database(void)
{
    bool needwarn = true;
    int spin = 5;

    struct tagcache_stat *stat = rb->tagcache_get_stat();

    while ( !(stat->initialized && stat->ready) )
    {
        if (--spin > 0)
        {
            rb->sleep(HZ/5);
        }
        else if (needwarn)
        {
            needwarn = false;
            rb->splash(0, ID2P(LANG_TAGCACHE_BUSY));
        }
        else
            return false;

        rb->yield();
        stat = rb->tagcache_get_stat();
    }
    return true;
}

static bool confirm_quit(void)
{
    const struct text_message prompt =
             { (const char*[]) {"Quit?", "Progress will be lost"}, 2};
    enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
    while (rb->button_get(false) == BUTTON_NONE)
    {;;}

    if(response == YESNO_NO)
        return false;
    else
        return true;
}

static void config_save(int cache_version, bool update_albumart)
{
    pf_cfg.cache_version = cache_version;
    pf_cfg.update_albumart = update_albumart;
    configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);
}

static void config_set_defaults(struct pf_config_t *cfg)
{
     cfg->slide_spacing = DISPLAY_WIDTH / 4;
     cfg->center_margin = (LCD_WIDTH - DISPLAY_WIDTH) / 12;
     cfg->num_slides = 4;
     cfg->zoom = 100;
     cfg->show_fps = false;
     cfg->auto_wps = 0;
     cfg->last_album = 0;
     cfg->backlight_mode = 0;
     cfg->resize = true;
     cfg->cache_version = CACHE_REBUILD;
     cfg->show_album_name = (LCD_HEIGHT > 100)
        ? ALBUM_AND_ARTIST_BOTTOM : ALBUM_NAME_BOTTOM;
     cfg->sort_albums_by = SORT_BY_ARTIST_AND_NAME;
     cfg->year_sort_order = ASCENDING;
     cfg->show_year = false;
     cfg->update_albumart = false;
}

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

#define MULUQ(a, b) ((a) * (b))

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

/* scales the 8bit subpixel value to native lcd format, indicated by bits */
static inline unsigned scale_subpixel_lcd(unsigned val, unsigned bits)
{
    (void) bits;
#if LCD_PIXELFORMAT != RGB888
    val = val * ((1 << bits) - 1);
    val = ((val >> 8) + val + 128) >> 8;
#endif
    return val;
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
        r = scale_subpixel_lcd(qp->red, 5);
        g = scale_subpixel_lcd(qp->green, 6);
        b = scale_subpixel_lcd((qp++)->blue, 5);
        *dest = FB_RGBPACK_LCD(r,g,b);
    }
#endif
}

/* read_image_file() is called without FORMAT_TRANSPARENT so
 * it's safe to ignore alpha channel in the next two functions */
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
    struct uint32_argb *qp = (struct uint32_argb*)row_in;
    int r, g, b;
    for (; dest < end; dest += ctx->bm->height)
    {
        r = scale_subpixel_lcd(SC_OUT(qp->r, ctx), 5);
        g = scale_subpixel_lcd(SC_OUT(qp->g, ctx), 6);
        b = scale_subpixel_lcd(SC_OUT(qp->b, ctx), 5);
        qp++;
        *dest = FB_RGBPACK_LCD(r,g,b);
    }
#endif
}

#ifdef HAVE_LCD_COLOR
static void output_row_32_transposed_fromyuv(uint32_t row, void * row_in,
                                       struct scaler_context *ctx)
{
    pix_t *dest = (pix_t*)ctx->bm->data + row;
    pix_t *end = dest + ctx->bm->height * ctx->bm->width;
    struct uint32_argb *qp = (struct uint32_argb*)row_in;
    for (; dest < end; dest += ctx->bm->height)
    {
        unsigned r, g, b, y, u, v;
        y = SC_OUT(qp->b, ctx);
        u = SC_OUT(qp->g, ctx);
        v = SC_OUT(qp->r, ctx);
        qp++;
        yuv_to_rgb(y, u, v, &r, &g, &b);
        r = scale_subpixel_lcd(r, 5);
        g = scale_subpixel_lcd(g, 6);
        b = scale_subpixel_lcd(b, 5);
        *dest = FB_RGBPACK_LCD(r, g, b);
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
    context &= ~CONTEXT_LOCKED;
    return pf_contexts[context & ~CONTEXT_PLUGIN];
}

/* scrolling */
static void init_scroll_lines(void)
{
    int i;
    static const char scroll_tick_table[16] = {
     /* Hz values:
        1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
        100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
    };

    scroll_line_info.ticks = scroll_tick_table[rb->global_settings->scroll_speed];
    scroll_line_info.step = rb->global_settings->scroll_step;
    scroll_line_info.delay = rb->global_settings->scroll_delay / (HZ / 10);
    scroll_line_info.next_scroll = *rb->current_tick;
    for (i = 0; i < PF_MAX_SCROLL_LINES; i++)
        scroll_lines[i].step = 0;
}

static void set_scroll_line(const char *str, enum pf_scroll_line_type type)
{
    struct pf_scroll_line *s = &scroll_lines[type];
    s->width = mylcd_getstringsize(str, NULL, NULL);
    s->step = 0;
    s->offset = 0;
    s->start_tick = *rb->current_tick + scroll_line_info.delay;
    if (LCD_WIDTH - s->width < 0)
        s->step = scroll_line_info.step;
    else
        s->offset = (LCD_WIDTH - s->width) / 2;
}

static int get_scroll_line_offset(enum pf_scroll_line_type type)
{
    return scroll_lines[type].offset;
}

static void update_scroll_lines(void)
{
    int i;

    if (TIME_BEFORE(*rb->current_tick, scroll_line_info.next_scroll))
        return;

    scroll_line_info.next_scroll = *rb->current_tick + scroll_line_info.ticks;

    for (i = 0; i < PF_MAX_SCROLL_LINES; i++)
    {
        struct pf_scroll_line *s = &scroll_lines[i];
        if (s->step && TIME_BEFORE(s->start_tick, *rb->current_tick))
        {
            s->offset -= s->step;

            if (s->offset >= 0) {
                /* at beginning of line */
                s->offset = 0;
                s->step = scroll_line_info.step;
                s->start_tick = *rb->current_tick + scroll_line_info.delay * 2;
            }
            if (s->offset <= LCD_WIDTH - s->width) {
                /* at end of line */
                s->offset = LCD_WIDTH - s->width;
                s->step = -scroll_line_info.step;
                s->start_tick = *rb->current_tick + scroll_line_info.delay * 2;
            }
        }
    }
}

/* Create the lookup table with the scaling values for the reflections */
static void init_reflect_table(void)
{
    int i;
    for (i = 0; i < REFLECT_HEIGHT; i++)
        reflect_table[i] =
            (768 * (REFLECT_HEIGHT - i) + (5 * REFLECT_HEIGHT / 2)) /
            (5 * REFLECT_HEIGHT);
}


static int compare_albums (const void *a_v, const void *b_v)
{
    uint32_t artist_a = ((struct album_data *)a_v)->artist_idx;
    uint32_t artist_b = ((struct album_data *)b_v)->artist_idx;

    uint32_t album_a = ((struct album_data *)a_v)->name_idx;
    uint32_t album_b = ((struct album_data *)b_v)->name_idx;

    int year_a = ((struct album_data *)a_v)->year;
    int year_b = ((struct album_data *)b_v)->year;

    switch (pf_cfg.sort_albums_by)
    {
        case SORT_BY_ARTIST_AND_NAME:
            if (artist_a - artist_b == 0)
                return (int)(album_a - album_b);
            break;
        case SORT_BY_ARTIST_AND_YEAR:
            if (artist_a - artist_b == 0)
            {
                if (pf_cfg.year_sort_order == ASCENDING)
                    return year_a - year_b;
                else
                    return year_b - year_a;
            }
            break;
        case SORT_BY_YEAR:
            if (year_a - year_b != 0)
            {
                if (pf_cfg.year_sort_order == ASCENDING)
                    return year_a - year_b;
                else
                    return year_b - year_a;
            }
            break;
        case SORT_BY_NAME:
            if (album_a - album_b != 0)
                return (int)(album_a - album_b);
            break;
    }

    return (int)(artist_a - artist_b);
}

static int compare_album_artists (const void *a_v, const void *b_v)
{
    uint32_t a = ((struct album_data *)a_v)->artist_idx;
    uint32_t b = ((struct album_data *)b_v)->artist_idx;
    return (int)(a - b);
}

static void write_album_index(int idx, int name_idx,
                              long album_seek, int artist_idx, long artist_seek)
{
    pf_idx.album_index[idx].name_idx = name_idx;
    pf_idx.album_index[idx].seek = album_seek;
    pf_idx.album_index[idx].artist_idx = artist_idx;
    pf_idx.album_index[idx].artist_seek = artist_seek;
    pf_idx.album_index[idx].year = 0;
}

static inline void write_album_entry(struct tagcache_search *tcs,
                                     int name_idx, unsigned int len)
{
    write_album_index(-pf_idx.album_ct, name_idx, tcs->result_seek, 0, -1);
    pf_idx.album_len += len;
    pf_idx.album_ct++;

    if (pf_idx.album_untagged_seek == -1 && rb->strcmp(UNTAGGED, tcs->result) == 0)
    {
        pf_idx.album_untagged_idx = name_idx;
        pf_idx.album_untagged_seek = tcs->result_seek;
    }
}

static void write_artist_entry(struct tagcache_search *tcs,
                               int name_idx, unsigned int len)
{
    pf_idx.artist_index[-pf_idx.artist_ct].name_idx = name_idx;
    pf_idx.artist_index[-pf_idx.artist_ct].seek = tcs->result_seek;
    pf_idx.artist_len += len;
    pf_idx.artist_ct++;
}

/* adds tagcache_search results into artist/album index */
static int get_tcs_search_res(int type, struct tagcache_search *tcs,
                              void **buf, size_t *bufsz)
{
    char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    int ret = SUCCESS;
    unsigned int l, name_idx = 0;
    void (*writefn)(struct tagcache_search *, int, unsigned int);
    int data_size;
    if (type == ePFS_ARTIST)
    {
        writefn = &write_artist_entry;
        data_size = sizeof(struct artist_data);
    }
    else
    {
        writefn = &write_album_entry;
        data_size = sizeof(struct album_data);
    }

    while (rb->tagcache_get_next(tcs, tcs_buf, tcs_bufsz))
    {
        if (rb->button_get(false) > BUTTON_NONE)
        {
            if (confirm_quit())
            {
                ret = ERROR_USER_ABORT;
                break;
            } else
                rb->lcd_clear_display();
        }

        *bufsz -= data_size;

        l = tcs->result_len;

        if ( l > *bufsz )
        {
            /* not enough memory */
            ret = ERROR_BUFFER_FULL;
            break;
        }

        rb->strcpy(*buf, tcs->result);

        *bufsz -= l;
        *buf = l + (char *)*buf;

        writefn(tcs, name_idx, l);

        name_idx += l;
    }
    rb->tagcache_search_finish(tcs);
    return ret;
}

/*adds <untagged> albums/artist to existing album index */
static int create_album_untagged(struct tagcache_search *tcs,
                                 void **buf, size_t *bufsz)
{
    static char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    int ret = SUCCESS;
    int album_count = pf_idx.album_ct; /* store existing count */
    int total_count = pf_idx.album_ct + pf_idx.artist_ct * 2;
    long seek;
    int last, final, retry;
    int i, j;
    draw_splashscreen(*buf, *bufsz);
    draw_progressbar(0, total_count, "Searching " UNTAGGED);

    /* search tagcache for all <untagged> albums & save the albumartist seek pos */
    if (rb->tagcache_search(tcs, tag_albumartist))
    {
        rb->tagcache_search_add_filter(tcs, tag_album, pf_idx.album_untagged_seek);

        while (rb->tagcache_get_next(tcs, tcs_buf, tcs_bufsz))
        {
            if (rb->button_get(false) > BUTTON_NONE) {
                if (confirm_quit())
                {
                    rb->tagcache_search_finish(tcs);
                    return ERROR_USER_ABORT;
                }
                else
                {
                    rb->lcd_clear_display();
                    draw_progressbar(pf_idx.album_ct, total_count,
                                     "Searching " UNTAGGED);
                }
            }

            if (tcs->result_seek ==
                pf_idx.album_index[-(pf_idx.album_ct - 1)].artist_seek)
                continue;

            if (sizeof(struct album_data) > *bufsz)
            {
                /* not enough memory */
                ret = ERROR_BUFFER_FULL;
                break;
            }

            *bufsz -= sizeof(struct album_data);
            write_album_index(-pf_idx.album_ct, pf_idx.album_untagged_idx,
                               pf_idx.album_untagged_seek, -1, tcs->result_seek);

            pf_idx.album_ct++;
            draw_progressbar(pf_idx.album_ct, total_count, NULL);
        }
        rb->tagcache_search_finish(tcs);

        if (ret == SUCCESS) {
            draw_splashscreen(*buf, *bufsz);
            draw_progressbar(0, pf_idx.album_ct, "Finalizing " UNTAGGED);

            last = 0;
            final = pf_idx.artist_ct;
            retry = 0;

            /* map the artist_seek position to the artist name index */
            for (j = album_count; j < pf_idx.album_ct; j++)
            {
                if (rb->button_get(false) > BUTTON_NONE) {
                    if (confirm_quit())
                        return ERROR_USER_ABORT;
                    else
                    {
                        rb->lcd_clear_display();
                        draw_progressbar(j, pf_idx.album_ct, "Finalizing " UNTAGGED);
                    }
                }

                draw_progressbar(j, pf_idx.album_ct, NULL);
                seek = pf_idx.album_index[-j].artist_seek;

    retry_artist_lookup:
                retry++;
                for (i = last; i < final; i++)
                {
                    if (seek == pf_idx.artist_index[i].seek)
                    {
                        int idx = pf_idx.artist_index[i].name_idx;
                        pf_idx.album_index[-j].artist_idx = idx;
                        last = i; /* last match, start here next loop */
                        final = pf_idx.artist_ct;
                        retry = 0;
                        break;
                    }
                }
                if (retry > 0 && retry < 2)
                {
                    /* no match start back at beginning */
                    final = last;
                    last = 0;
                    goto retry_artist_lookup;
                }
            }
        }
    }

    return ret;
}

/* Create an index of all artists from the database */
static int build_artist_index(struct tagcache_search *tcs,
                                 void **buf, size_t *bufsz)
{
    int i, res = SUCCESS;
    struct artist_data* tmp_artist;

    /* artist index starts at end of buf it will be rearranged when finalized */
    pf_idx.artist_index = ((struct artist_data *)(*bufsz + (char *) *buf)) - 1;
    pf_idx.artist_ct = 0;
    pf_idx.artist_len = 0;
    /* artist names starts at beginning of buf */
    pf_idx.artist_names = *buf;

    rb->tagcache_search(tcs, tag_albumartist);
    res = get_tcs_search_res(ePFS_ARTIST, tcs, &(*buf), bufsz);
    rb->tagcache_search_finish(tcs);
    if (res < SUCCESS)
        return res;

    /* finalize the artist index */
    ALIGN_BUFFER(*buf, *bufsz, alignof(struct artist_data));
    tmp_artist = (struct artist_data*)*buf;
    for (i = pf_idx.artist_ct - 1; i >= 0; i--)
        tmp_artist[i] = pf_idx.artist_index[-i];

    pf_idx.artist_index = tmp_artist;
    /* move buf ptr to end of artist_index */
    *buf = pf_idx.artist_index + pf_idx.artist_ct;

    if (res == SUCCESS)
    {
        if (pf_idx.artist_ct > 0)
            res = pf_idx.artist_ct;
        else
            res = ERROR_NO_ALBUMS;
    }

    return res;
}


static int assign_album_year(void)
{
    char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    draw_progressbar(0, pf_idx.album_ct, "Assigning Album Year");
    for (int album_idx = 0; album_idx < pf_idx.album_ct; album_idx++)
    {
        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        if (rb->button_get(false) > BUTTON_NONE)
        {
            if (confirm_quit())
                return ERROR_USER_ABORT;
            else
            {
                rb->lcd_clear_display();
                draw_progressbar(album_idx, pf_idx.album_ct, "Assigning Album Year");
            }
        }
        draw_progressbar(album_idx, pf_idx.album_ct, NULL);
        int album_year = 0;

        if (rb->tagcache_search(&tcs, tag_year))
        {
            rb->tagcache_search_add_filter(&tcs, tag_album,
                                       pf_idx.album_index[album_idx].seek);

            if (pf_idx.album_index[album_idx].artist_idx >= 0)
                rb->tagcache_search_add_filter(&tcs, tag_albumartist,
                    pf_idx.album_index[album_idx].artist_seek);

            while (rb->tagcache_get_next(&tcs, tcs_buf, tcs_bufsz)) {
                int track_year = rb->tagcache_get_numeric(&tcs, tag_year);
                if (track_year > album_year)
                    album_year = track_year;
            }
        }
        rb->tagcache_search_finish(&tcs);

        pf_idx.album_index[album_idx].year = album_year;
    }
    return SUCCESS;
}

/**
  Create an index of all artists and albums from the database.
  Also store the artists and album names so we can access them later.
 */
static int create_album_index(void)
{
    static char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    void *buf = pf_idx.buf;
    size_t buf_size = pf_idx.buf_sz;

    struct album_data* tmp_album;

    int i, j, last, final, retry, res;

    draw_splashscreen(buf, buf_size);
    ALIGN_BUFFER(buf, buf_size, sizeof(long));

    /* Artists */
    res = build_artist_index(&tcs, &buf, &buf_size);
    if (res < SUCCESS)
        return res;

    /* Albums */
    pf_idx.album_ct = 0;
    pf_idx.album_len =0;
    pf_idx.album_untagged_idx = 0;
    pf_idx.album_untagged_seek = -1;

    /* album_index starts at end of buf it will be rearranged when finalized */
    pf_idx.album_index = ((struct album_data *)(buf_size + (char *)buf)) - 1;
    /* album_names starts at the beginning of buf */
    pf_idx.album_names = buf;

    rb->tagcache_search(&tcs, tag_album);
    res = get_tcs_search_res(ePFS_ALBUM, &tcs, &buf, &buf_size);
    rb->tagcache_search_finish(&tcs);
    if (res < SUCCESS)
        return res;

    /* Build artist list for untagged albums */
    res = create_album_untagged(&tcs, &buf, &buf_size);

    if (res < SUCCESS)
        return res;

    /* finalize the album index */
    ALIGN_BUFFER(buf, buf_size, alignof(struct album_data));
    tmp_album = (struct album_data*)buf;
    for (i = pf_idx.album_ct - 1; i >= 0; i--)
        tmp_album[i] = pf_idx.album_index[-i];

    pf_idx.album_index = tmp_album;
    /* move buf ptr to end of album_index */
    buf = pf_idx.album_index + pf_idx.album_ct;

    /* Assign indices */
    draw_splashscreen(buf, buf_size);
    draw_progressbar(0, pf_idx.album_ct, "Assigning Albums");
    for (j = 0; j < pf_idx.album_ct; j++)
    {
        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        if (rb->button_get(false) > BUTTON_NONE)
        {
            if (confirm_quit())
                return ERROR_USER_ABORT;
            else
            {
                rb->lcd_clear_display();
                draw_progressbar(j, pf_idx.album_ct, "Assigning Albums");
            }

        }

        draw_progressbar(j, pf_idx.album_ct, NULL);
        if (pf_idx.album_index[j].artist_seek >= 0) { continue; }

        rb->tagcache_search(&tcs, tag_albumartist);
        rb->tagcache_search_add_filter(&tcs, tag_album, pf_idx.album_index[j].seek);

        last = 0;
        final = pf_idx.artist_ct;
        retry = 0;
        if (rb->tagcache_get_next(&tcs, tcs_buf, tcs_bufsz))
        {

retry_artist_lookup:
            retry++;
            for (i = last; i < final; i++)
            {
                if (tcs.result_seek == pf_idx.artist_index[i].seek)
                {
                    int idx = pf_idx.artist_index[i].name_idx;
                    pf_idx.album_index[j].artist_idx = idx;
                    pf_idx.album_index[j].artist_seek = tcs.result_seek;
                    last = i; /* last match, start here next loop */
                    final = pf_idx.artist_ct;
                    retry = 0;
                    break;
                }
            }
            if (retry > 0 && retry < 2)
            {
                /* no match start back at beginning */
                final = last;
                last = 0;
                goto retry_artist_lookup;
            }
        }
        rb->tagcache_search_finish(&tcs);
    }

    draw_splashscreen(buf, buf_size);

    res = assign_album_year();

    if (res < SUCCESS)
        return res;

    /* sort list order to find duplicates */
    rb->qsort(pf_idx.album_index, pf_idx.album_ct,
              sizeof(struct album_data), compare_album_artists);

    draw_splashscreen(buf, buf_size);
    draw_progressbar(0, pf_idx.album_ct, "Removing duplicates");
    /* mark duplicate albums for deletion */
    for (i = 0; i < pf_idx.album_ct - 1; i++) /* -1 don't check last entry */
    {
        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        int idxi = pf_idx.album_index[i].artist_idx;
        int seeki = pf_idx.album_index[i].seek;

        draw_progressbar(i, pf_idx.album_ct, NULL);
        for (j = i + 1; j < pf_idx.album_ct; j++)
        {
            if (idxi > 0 &&
            idxi == pf_idx.album_index[j].artist_idx &&
            seeki == pf_idx.album_index[j].seek)
            {
                pf_idx.album_index[j].artist_idx = -1;
            }
            else
            {
                i = j - 1;
                break;
            }
        }
    }

    /* now fix the album list order */
    rb->qsort(pf_idx.album_index, pf_idx.album_ct,
              sizeof(struct album_data), compare_album_artists);

    /* remove any extra untagged albums
     * extra space is orphaned till restart */
    for (i = 0; i < pf_idx.album_ct; i++)
    {
        if (pf_idx.album_index[i].artist_idx > 0)
        {
            if (i > 0) { i--; }
            pf_idx.album_index += i;
            pf_idx.album_ct -= i;
            break;
        }
    }

    pf_idx.buf = buf;
    pf_idx.buf_sz = buf_size;
    pf_idx.artist_index = 0;

    rb->qsort(pf_idx.album_index, pf_idx.album_ct,
                          sizeof(struct album_data), compare_albums);

    return (pf_idx.album_ct > 0) ? 0 : ERROR_NO_ALBUMS;
}

/*Saves the album index into a binary file to be recovered the
 next time PictureFlow is launched*/

static int save_album_index(void){
    int fd = rb->creat(ALBUM_INDEX,0666);

    struct pf_index_t data;
    memcpy(&data, &pf_idx, sizeof(struct pf_index_t));

    if(fd >= 0)
    {
        rb->memcpy(&data.header, INDEX_HDR, sizeof(pf_idx.header));

        rb->write(fd, &data, sizeof(struct pf_index_t));

        rb->write(fd, data.artist_names, data.artist_len);
        rb->write(fd, data.album_names, data.album_len);

        rb->write(fd, data.album_index, data.album_ct * sizeof(struct album_data));

        rb->close(fd);
        return 0;
    }
    return -1;
}

/* reads data from save file to buffer */
static inline int read2buf(int fildes, void *buf, size_t nbyte){
    int read;
    read = rb->read(fildes, buf, nbyte);
    if (read < (int)nbyte)
        return 0;

    return read;
}

/*Loads the album_index information stored in the hard drive*/
static int load_album_index(void){

    int i, fr = rb->open(ALBUM_INDEX, O_RDONLY);
    struct pf_index_t data;

    void *bufstart = pf_idx.buf;
    unsigned int bufstart_sz = pf_idx.buf_sz;

    void* buf = pf_idx.buf;
    size_t buf_size = pf_idx.buf_sz;

    unsigned int name_sz, album_idx_sz;
    int album_idx, artist_idx;

    if (fr >= 0){
        const unsigned long filesize = rb->filesize(fr);
        if (filesize > sizeof(data))
        {
            if (rb->read(fr, &data, sizeof(data)) == sizeof(data) &&
                rb->memcmp(&(data.header), INDEX_HDR, sizeof(data.header)) == 0)
            {
                name_sz = data.artist_len + data.album_len;
                album_idx_sz = data.album_ct * sizeof(struct album_data);

                if (name_sz + album_idx_sz > bufstart_sz)
                    goto failure;

                //rb->lseek(fr, sizeof(data) + 1, SEEK_SET);
                /* artist names */
                if (read2buf(fr, buf, data.artist_len) == 0)
                    goto failure;

                data.artist_names = buf;
                buf = (char *)buf + data.artist_len;
                buf_size -= data.artist_len;

                /* album names */
                if (read2buf(fr, buf, data.album_len) == 0)
                    goto failure;

                data.album_names = buf;
                buf = (char *)buf + data.album_len;
                buf_size -= data.album_len;

                /* index of album names */
                ALIGN_BUFFER(buf, buf_size, alignof(struct album_data));
                if (read2buf(fr, buf, album_idx_sz) == 0)
                    goto failure;

                data.album_index = buf;
                buf = (char *)buf + album_idx_sz;
                buf_size -= album_idx_sz;

                rb->close(fr);

                /* sanity check loaded data */
                for (i = 0; i < data.album_ct; i++)
                {
                    album_idx = data.album_index[i].name_idx;
                    artist_idx = data.album_index[i].artist_idx;
                    if (album_idx >= (int) data.album_len ||
                        artist_idx >= (int) data.artist_len)
                    {
                        goto failure;
                    }
                }

                memcpy(&pf_idx, &data, sizeof(struct pf_index_t));
                pf_idx.buf = buf;
                pf_idx.buf_sz = buf_size;

                rb->qsort(pf_idx.album_index, pf_idx.album_ct,
                          sizeof(struct album_data), compare_albums);

                return 0;
            }
        }
    }

failure:
    rb->splash(HZ/2, "Failed to load index");
    if (fr >= 0)
        rb->close(fr);

    pf_idx.buf = bufstart;
    pf_idx.buf_sz = bufstart_sz;
    pf_idx.artist_ct = 0;
    pf_idx.album_ct = 0;
    return -1;

}

/**
 Return a pointer to the album name of the given slide_index
 */
static char* get_album_name(const int slide_index)
{
    char *name = pf_idx.album_names + pf_idx.album_index[slide_index].name_idx;
    return name;
}

/**
 Return a pointer to the album name of the given slide_index
 */
static char* get_album_name_idx(const int slide_index, int *idx)
{
    *idx = pf_idx.album_index[slide_index].name_idx;
    char *name = pf_idx.album_names + pf_idx.album_index[slide_index].name_idx;
    return name;
}

/**
 Return a pointer to the album artist of the given slide_index
 */
static char* get_album_artist(const int slide_index)
{
    if (slide_index < pf_idx.album_ct && slide_index >= 0){
        int idx = pf_idx.album_index[slide_index].artist_idx;
        if (idx >= 0 && idx < (int) pf_idx.artist_len) {
            char *name = pf_idx.artist_names + idx;
            return name;
        }
    }
    return "?";
}


static char* get_slide_name(const int slide_index, bool artist)
{
    if (artist)
        return get_album_artist(slide_index);

    return get_album_name(slide_index);
}

/**
 Return a pointer to the track name of the active album
 create_track_index has to be called first.
 */
static char* get_track_name(const int track_index)
{
    if (track_index >= 0 && track_index < pf_tracks.count )
        return pf_tracks.names + pf_tracks.index[track_index].name_idx;
    return 0;
}
#if PF_PLAYBACK_CAPABLE
static char* get_track_filename(const int track_index)
{
    if ( track_index < pf_tracks.count )
        return pf_tracks.names + pf_tracks.index[track_index].filename_idx;
    return 0;
}
#endif



static int jmp_idx_prev(void)
{
    if (aa_cache.inspected < pf_idx.album_ct)
    {
#ifdef USEGSLIB
        grey_show(false);
        rb->lcd_clear_display();
        rb->lcd_update();
#endif
        rb->splash(HZ*2, rb->str(LANG_WAIT_FOR_CACHE));
#ifdef USEGSLIB
        grey_show(true);
#endif
        return center_index;
    }

    if (pf_cfg.sort_albums_by == SORT_BY_YEAR)
    {
        int current_year = pf_idx.album_index[center_index].year;

        for (int i = center_index - 1; i > 0; i-- )
        {
            if(pf_idx.album_index[i].year != current_year)
                current_year = pf_idx.album_index[i].year;
            while (i > 0)
            {
                if (pf_idx.album_index[i-1].year != current_year)
                    break;
                i--;
            }
            return i;
        }
    }
    else
    {
        bool by_artist = pf_cfg.sort_albums_by != SORT_BY_NAME;
        char *current_selection = get_slide_name(center_index, by_artist);

        for (int i = center_index - 1; i > 0; i-- )
        {
            if(rb->strncmp(get_slide_name(i, by_artist), current_selection, 1))
                current_selection = get_slide_name(i, by_artist);
            while (i > 0)
            {
                if (rb->strncmp(get_slide_name(i-1, by_artist), current_selection, 1))
                    break;
                i--;
            }
            return i;
        }
    }

    return 0;
}

static int jmp_idx_next(void)
{
    if (aa_cache.inspected < pf_idx.album_ct)
    {
#ifdef USEGSLIB
        grey_show(false);
        rb->lcd_clear_display();
        rb->lcd_update();
#endif
        rb->splash(HZ*2, rb->str(LANG_WAIT_FOR_CACHE));
#ifdef USEGSLIB
        grey_show(true);
#endif
        return center_index;
    }

    if (pf_cfg.sort_albums_by == SORT_BY_YEAR)
    {
        int current_year = pf_idx.album_index[center_index].year;
        for (int i = center_index + 1; i < pf_idx.album_ct; i++ )
            if(pf_idx.album_index[i].year != current_year)
                return i;
    }
    else
    {
        bool by_artist = pf_cfg.sort_albums_by != SORT_BY_NAME;
        char *current_selection = get_slide_name(center_index, by_artist);
        for (int i = center_index + 1; i < pf_idx.album_ct; i++ )
            if(rb->strncmp(get_slide_name(i, by_artist), current_selection, 1))
                return i;
    }
    return pf_idx.album_ct - 1;
}

static int id3_get_index(struct mp3entry *id3)
{
    char* current_artist = UNTAGGED;
    char* current_album  = UNTAGGED;

    if(id3)
    {
        /* we could be looking for the artist in either field */
        if(id3->albumartist)
            current_artist = id3->albumartist;
        else if(id3->artist)
            current_artist = id3->artist;

        if (id3->album && rb->strlen(id3->album) > 0)
            current_album = id3->album;

        //rb->splashf(1000, "%s, %s", current_album, current_artist);

        int i;
        int album_idx, artist_idx;

        for (i = 0; i < pf_idx.album_ct; i++ )
        {
            album_idx = pf_idx.album_index[i].name_idx;
            artist_idx = pf_idx.album_index[i].artist_idx;

            if(!rb->strcmp(pf_idx.album_names + album_idx, current_album) &&
                !rb->strcmp(pf_idx.artist_names + artist_idx, current_artist))
                return i;
        }

    }
    rb->splash(HZ/2, "Album Not Found!");
    return pf_cfg.last_album;
}

/**
  Compare two unsigned ints passed via pointers.
 */
static int compare_tracks (const void *a_v, const void *b_v)
{
    uint32_t a = ((struct track_data *)a_v)->sort;
    uint32_t b = ((struct track_data *)b_v)->sort;
    return (int)(a - b);
}



static bool track_buffer_avail(size_t needed)
{
    size_t total_out = 0;
    size_t out = 0;
    if (pf_tracks.borrowed == 0 && pf_tracks.used == 0)
    {
        pf_tracks.names = rb->buflib_buffer_out(&buf_ctx, &out);
        pf_tracks.borrowed = out;
    }

    if (needed <= pf_tracks.borrowed - pf_tracks.used)
        return true;

    while (needed > (pf_tracks.borrowed + total_out) - pf_tracks.used)
    {
        if (!free_slide_prio(0))
            break;
        out = 0;
        rb->buflib_buffer_out(&buf_ctx, &out);
        total_out += out;
    }
    pf_tracks.borrowed += total_out;

    // have to move already stored track_data structs
    if (pf_tracks.count)
    {
        struct track_data *new_tracks = (struct track_data *)(total_out + (uintptr_t)pf_tracks.index);
        unsigned int bytes = pf_tracks.count * sizeof(struct track_data);
        rb->memmove(new_tracks, pf_tracks.index, bytes);
    }

    if (needed > pf_tracks.borrowed - pf_tracks.used)
        return false;

    return true;
}


static int pf_tcs_retrieve_track_title(int string_index, int disc_num, int track_num)
{
    char file_name[MAX_PATH];
    char *track_title = NULL;
    int str_len;

    if (rb->strcmp(UNTAGGED, tcs.result) == 0)
    {
        /* show filename instead of <untaggged> */
        if (!rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_virt_basename,
                                file_name, MAX_PATH))
            return 0;
        track_title = file_name;
    }

    if (!track_title)
        track_title = tcs.result;

    int max_len = rb->strlen(track_title) + 10;
    if (!track_buffer_avail(max_len))
        return 0;

    if (track_num > 0)
    {
        if (disc_num > 0)
            str_len = rb->snprintf(pf_tracks.names + string_index, max_len,
                "%d.%02d: %s", disc_num, track_num, track_title);
        else
            str_len = rb->snprintf(pf_tracks.names + string_index, max_len,
                "%d: %s", track_num, track_title);
    }
    else
        str_len = rb->snprintf(pf_tracks.names + string_index, max_len,
            "%s", track_title);
    return str_len;
}

#if PF_PLAYBACK_CAPABLE
static int pf_tcs_retrieve_file_name(int fn_idx)
{
    if (!track_buffer_avail(MAX_PATH))
        return 0;

    rb->tagcache_retrieve(&tcs, tcs.idx_id, tag_filename,
            pf_tracks.names + fn_idx, MAX_PATH);

    return rb->strlen(pf_tracks.names + fn_idx);
}
#endif

/**
  Create the track index of the given slide_index.
 */
static void create_track_index(const int slide_index)
{
    char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    buf_ctx_lock();
    if ( slide_index == pf_tracks.cur_idx )
        return;

    if (!rb->tagcache_search(&tcs, tag_title))
        goto fail;

    rb->tagcache_search_add_filter(&tcs, tag_album,
                                   pf_idx.album_index[slide_index].seek);

    if (pf_idx.album_index[slide_index].artist_idx >= 0)
        rb->tagcache_search_add_filter(&tcs, tag_albumartist,
            pf_idx.album_index[slide_index].artist_seek);

    int string_index = 0;
    pf_tracks.count = 0;

    while (rb->tagcache_get_next(&tcs, tcs_buf, tcs_bufsz))
    {
        int disc_num = rb->tagcache_get_numeric(&tcs, tag_discnumber);
        int track_num = rb->tagcache_get_numeric(&tcs, tag_tracknumber);
        disc_num = disc_num > 0 ? disc_num : 0;
        track_num = track_num > 0 ? track_num : 0;
        int fn_idx = 1 + pf_tcs_retrieve_track_title(string_index, disc_num, track_num);
        if (fn_idx <= 1)
            goto fail;
        pf_tracks.used += fn_idx;

#if PF_PLAYBACK_CAPABLE
        int fn_len = 1 + pf_tcs_retrieve_file_name(string_index + fn_idx);
        if (fn_len <= 1)
            goto fail;
        pf_tracks.used += fn_len;
#endif
        if (!track_buffer_avail(sizeof(struct track_data)))
            goto fail;

        pf_tracks.used += sizeof(struct track_data);
        unsigned int arr_sz = (pf_tracks.count + 1) * sizeof(struct track_data);
        // Arrray descends from upper end of buflib-borrowed buffer.
        pf_tracks.index = (struct track_data*)(pf_tracks.names + pf_tracks.borrowed
                                                               - arr_sz );
        pf_tracks.index->sort = (disc_num << 24) + (track_num << 14);
        pf_tracks.index->sort += pf_tracks.count;
        pf_tracks.index->name_idx = string_index;
        pf_tracks.index->seek = tcs.result_seek;
#if PF_PLAYBACK_CAPABLE
        pf_tracks.index->filename_idx = fn_idx + string_index;
        string_index += (fn_idx + fn_len);
#else
        string_index += fn_idx;
#endif
        pf_tracks.count++;
    }

    rb->tagcache_search_finish(&tcs);

    /* now fix the track list order */
    rb->qsort(pf_tracks.index, pf_tracks.count,
              sizeof(struct track_data), compare_tracks);

    pf_tracks.cur_idx = slide_index;
    return;
fail:
    rb->tagcache_search_finish(&tcs);
    pf_tracks.count = 0;
    return;
}

/**
  Re-grow the buflib buffer by returning space borrowed
  for track list
*/
static inline void free_borrowed_tracks(void)
{
    rb->buflib_buffer_in(&buf_ctx, pf_tracks.borrowed);
    pf_tracks.borrowed = 0;
    pf_tracks.used = 0;
    pf_tracks.cur_idx = -1;
    buf_ctx_unlock();
}

/**
  Determine filename of the album art for the given slide_index and
  store the result in buf.
  The algorithm looks for the first track of the given album uses
  find_albumart to find the filename.
 */
static bool get_albumart_for_index_from_db(const int slide_index, char *buf,
                                    int buflen)
{
    bool ret;
    char tcs_buf[TAGCACHE_BUFSZ];
    const long tcs_bufsz = sizeof(tcs_buf);
    if (tcs.valid || !rb->tagcache_search(&tcs, tag_filename))
        return false;

    /* find the first track of the album */
    rb->tagcache_search_add_filter(&tcs, tag_album,
                                   pf_idx.album_index[slide_index].seek);

    rb->tagcache_search_add_filter(&tcs, tag_albumartist,
                                   pf_idx.album_index[slide_index].artist_seek);

    ret = rb->tagcache_get_next(&tcs, tcs_buf, tcs_bufsz) &&
          retrieve_id3(&id3, tcs.result) &&
          search_albumart_files(&id3, ":", buf, buflen);

    rb->tagcache_search_finish(&tcs);
    return ret;
}

/**
  Draw the PictureFlow logo
 */
static void draw_splashscreen(unsigned char * buf_tmp, size_t buf_tmp_size)
{
    struct screen* display = rb->screens[SCREEN_MAIN];
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
    int ret = rb->read_bmp_file(SPLASH_BMP, &logo, buf_tmp_size,
                                FORMAT_NATIVE, NULL);
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
static void draw_progressbar(int step, int count, char *msg)
{
    static int txt_w, txt_h;
    const int bar_height = 22;
    const int w = LCD_WIDTH - 20;
    const int x = 10;
    static int y;
    if (msg != NULL)
    {
#if LCD_DEPTH > 1
        rb->lcd_set_background(N_BRIGHT(0));
        rb->lcd_set_foreground(N_BRIGHT(255));
#else
        rb->lcd_set_drawmode(PICTUREFLOW_DRMODE);
#endif
        rb->lcd_getstringsize(msg, &txt_w, &txt_h);

        y = (LCD_HEIGHT - txt_h)/2;

        rb->lcd_putsxy((LCD_WIDTH - txt_w)/2, y, msg);
        y += (txt_h + 5);
    }
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(100));
#endif
    rb->lcd_drawrect(x, y, w+2, bar_height);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_PIX(165, 231, 82));
#endif

    rb->lcd_fillrect(x+1, y+1, step * w / count, bar_height-2);
#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif
    rb->lcd_update();
    rb->yield();
}

/* Calculate modified FNV hash of string
 * has good avalanche behaviour and uniform distribution
 * see http://home.comcast.net/~bretm/hash/ */
static unsigned int mfnv(char *str)
{
    const unsigned int p = 16777619;
    unsigned int hash = 0x811C9DC5; // 2166136261;

    if (!str)
        return 0;

    while(*str)
        hash = (hash ^ *str++) * p;
    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return hash;
}

/**
 Save the given bitmap as filename in the pfraw format
 */
static bool save_pfraw(char* filename, struct bitmap *bm)
{
    struct pfraw_header bmph;
    bmph.width = bm->width;
    bmph.height = bm->height;
    int fh = rb->open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if( fh < 0 ) return false;
    rb->write( fh, &bmph, sizeof( struct pfraw_header ) );
    rb->write( fh, bm->data , sizeof( pix_t ) * bm->width *  bm->height );
    rb->close( fh );
    return true;
}

static bool incremental_albumart_cache(bool verbose)
{
    if (!aa_cache.buf)
        goto aa_failure;

    if (aa_cache.inspected >= pf_idx.album_ct)
        return false;

    /* Prevent idle poweroff */
    rb->reset_poweroff_timer();

    int idx, ret;
    unsigned int hash_artist, hash_album;
    unsigned int format = FORMAT_NATIVE;

    if (pf_cfg.resize)
        format |= FORMAT_RESIZE|FORMAT_KEEP_ASPECT;

    idx = aa_cache.idx;
    if (idx >= pf_idx.album_ct || idx < 0) { idx = 0; } /* Rollover */


    aa_cache.idx++;
    aa_cache.inspected++;
    if (aa_cache.idx >= pf_idx.album_ct) { aa_cache.idx = 0; } /* Rollover */


    hash_artist = mfnv(get_album_artist(idx));
    hash_album = mfnv(get_album_name(idx));

    rb->snprintf(aa_cache.pfraw_file, sizeof(aa_cache.pfraw_file),
                 CACHE_PREFIX "/%x%x.pfraw", hash_album, hash_artist);

    if(pf_cfg.update_albumart && rb->file_exists(aa_cache.pfraw_file)) {
        aa_cache.slides++;
        goto aa_success;
    }

    if (!get_albumart_for_index_from_db(idx, aa_cache.file, sizeof(aa_cache.file)))
        goto aa_failure; //rb->strcpy(aa_cache.file, EMPTY_SLIDE_BMP);


    aa_cache.input_bmp.data = aa_cache.buf;
    aa_cache.input_bmp.width = DISPLAY_WIDTH;
    aa_cache.input_bmp.height = DISPLAY_HEIGHT;

    ret = read_image_file(aa_cache.file, &aa_cache.input_bmp,
                          aa_cache.buf_sz, format, &format_transposed);
    if (ret <= 0) {
        if (verbose) {
            rb->splashf(HZ, "Album art is bad: %s", get_album_name(idx));
        }

        goto aa_failure;
    }
    rb->remove(aa_cache.pfraw_file);

    if (!save_pfraw(aa_cache.pfraw_file, &aa_cache.input_bmp))
    {
        if (verbose) { rb->splash(HZ, "Could not write bmp"); }
        goto aa_failure;
    }
    aa_cache.slides++;

aa_failure:
    if (verbose)
    {
        if (aa_cache.inspected >= pf_idx.album_ct)
            configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS,
                            CONFIG_VERSION);
        return false;
    }

aa_success:
    if (aa_cache.inspected >= pf_idx.album_ct)
    {
        configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS,
                            CONFIG_VERSION);
        free_all_slide_prio(0);
        if (pf_state == pf_idle)
            rb->queue_post(&thread_q, EV_WAKEUP, 0);
    }

    if(verbose)/* direct interaction with user */
        return true;

    return false;
}

/**
 Precomupte the album art images and store them in CACHE_PREFIX.
 Use the "?" bitmap if image is not found.
 */
static bool create_albumart_cache(void)
{
    draw_splashscreen(pf_idx.buf, pf_idx.buf_sz);
    draw_progressbar(0, pf_idx.album_ct, "Preparing artwork");
    aa_cache.inspected = 0;
    for (int i=0; i < pf_idx.album_ct; i++)
    {
        incremental_albumart_cache(true);
        draw_progressbar(aa_cache.inspected, pf_idx.album_ct, NULL);
        if (rb->button_get(false) > BUTTON_NONE)
            return true;
    }
    if ( aa_cache.slides == 0 ) {
        /* Warn the user that we couldn't find any albumart */
        rb->splash(2*HZ, ID2P(LANG_NO_ALBUMART_FOUND));
        return false;
    }
    return true;
}

/**
  Create the "?" slide, that is shown while loading
  or when no cover was found.
 */
static int create_empty_slide(bool force)
{
    const unsigned int format = FORMAT_NATIVE|FORMAT_RESIZE|FORMAT_KEEP_ASPECT;

    if (!aa_cache.buf)
        return false;

    if ( force || ! rb->file_exists( EMPTY_SLIDE ) )  {
        aa_cache.input_bmp.width = DISPLAY_WIDTH;
        aa_cache.input_bmp.height = DISPLAY_HEIGHT;
#if LCD_DEPTH > 1
        aa_cache.input_bmp.format = FORMAT_NATIVE;
#endif
        aa_cache.input_bmp.data = (char*)aa_cache.buf;

        scaled_read_bmp_file(EMPTY_SLIDE_BMP, &aa_cache.input_bmp,
                             aa_cache.buf_sz, format, &format_transposed);

        if (!save_pfraw(EMPTY_SLIDE, &aa_cache.input_bmp))
            return false;
    }

    return true;
}

/**
 Thread used for loading and preparing bitmaps in the background
 */
static void thread(void)
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

        if(ev.id != SYS_TIMEOUT) {
            while ( rb->queue_empty(&thread_q) ) {
                buf_ctx_lock();
                bool slide_loaded = load_new_slide();
                buf_ctx_unlock();
                if (!slide_loaded)
                    break;
                rb->yield();
            }
        }
    }
}


/**
 End the thread by posting the EV_EXIT event
 */
static void end_pf_thread(void)
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
static bool create_pf_thread(void)
{
    /* put the thread's queue in the bcast list */
    rb->queue_init(&thread_q, true);
    if ((thread_id = rb->create_thread(
                           thread,
                           thread_stack,
                           sizeof(thread_stack),
                            0,
                           "Picture load thread"
                               IF_PRIO(, PRIORITY_BUFFERING)
                               IF_COP(, CPU)
                                      )
        ) == 0) {
        return false;
    }
    thread_is_running = true;
    rb->queue_post(&thread_q, EV_WAKEUP, 0);
    return true;
}


static void initialize_slide_cache(void)
{
    int i= 0;
    for (i = 0; i < SLIDE_CACHE_SIZE; i++) {
        pf_sldcache.cache[i].hid = 0;
        pf_sldcache.cache[i].index = 0;
        pf_sldcache.cache[i].next = i + 1;
        pf_sldcache.cache[i].prev = i - 1;
    }
    pf_sldcache.cache[0].prev = i - 1;
    pf_sldcache.cache[i - 1].next = 0;

    pf_sldcache.free = 0;
    pf_sldcache.used = -1;
    pf_sldcache.left_idx = -1;
    pf_sldcache.right_idx = -1;
    pf_sldcache.center_idx = -1;
}


/*
 * The following functions implement the linked-list-in-array used to manage
 * the LRU cache of slides, and the list of free cache slots.
 */

#define _SEEK_RIGHT_WHILE(start, cond) \
({ \
    int ind_, next_ = (start); \
    int i_ = 0; \
    do { \
        ind_ = next_; \
        next_ = pf_sldcache.cache[ind_].next; \
        i_++; \
    } while (next_ != pf_sldcache.used && (cond) && i_ < SLIDE_CACHE_SIZE); \
    if (i_ >= SLIDE_CACHE_SIZE) \
    /* TODO: Not supposed to happen */ \
        ind_ = -1; \
    ind_; \
})

#define _SEEK_LEFT_WHILE(start, cond) \
({ \
    int ind_, next_ = (start); \
    int i_ = 0; \
    do { \
        ind_ = next_; \
        next_ = pf_sldcache.cache[ind_].prev; \
        i_++; \
    } while (ind_ != pf_sldcache.used && (cond) && i_ < SLIDE_CACHE_SIZE); \
    if (i_ >= SLIDE_CACHE_SIZE) \
    /* TODO: Not supposed to happen */ \
        ind_ = -1; \
    ind_; \
})

/**
 Pop the given item from the linked list starting at *head, returning the next
 item, or -1 if the list is now empty.
*/
static inline int lla_pop_item (int *head, int i)
{
    int prev = pf_sldcache.cache[i].prev;
    int next = pf_sldcache.cache[i].next;
    if (i == next)
    {
        *head = -1;
        return -1;
    }
    else if (i == *head)
        *head = next;
    pf_sldcache.cache[next].prev = prev;
    pf_sldcache.cache[prev].next = next;
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
    int prev = pf_sldcache.cache[next].prev;
    pf_sldcache.cache[next].prev = i;
    pf_sldcache.cache[prev].next = i;
    pf_sldcache.cache[i].next = next;
    pf_sldcache.cache[i].prev = prev;
}


/**
 Insert the item at index i at the end of the list starting at *head.
*/
static inline void lla_insert_tail (int *head, int i)
{
    if (*head == -1)
    {
        *head = i;
        pf_sldcache.cache[i].next = i;
        pf_sldcache.cache[i].prev = i;
    } else
        lla_insert(i, *head);
}

/**
 Insert the item at index i before the one at index p.
*/
static inline void lla_insert_after(int i, int p)
{
    p = pf_sldcache.cache[p].next;
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
    if (pf_sldcache.cache[i].hid != empty_slide_hid)
        rb->buflib_free(&buf_ctx, pf_sldcache.cache[i].hid);
    pf_sldcache.cache[i].index = -1;
    lla_pop_item(&pf_sldcache.used, i);
    lla_insert_tail(&pf_sldcache.free, i);
    if (pf_sldcache.used == -1)
    {
        pf_sldcache.right_idx = -1;
        pf_sldcache.left_idx = -1;
        pf_sldcache.center_idx = -1;
    }
}


/**
 Free one slide ranked above the given priority. If no such slide can be found,
 return false.
*/
static bool free_slide_prio(int prio)
{
    if (pf_sldcache.used == -1)
        return false;

    int i, prio_max;
    int l = pf_sldcache.used;
    int r = pf_sldcache.cache[pf_sldcache.used].prev;

    int prio_l = pf_sldcache.cache[l].index < center_index ?
           center_index - pf_sldcache.cache[l].index : 0;
    int prio_r = pf_sldcache.cache[r].index > center_index ?
           pf_sldcache.cache[r].index - center_index : 0;
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
        if (i == pf_sldcache.left_idx)
            pf_sldcache.left_idx = pf_sldcache.cache[i].next;
        if (i == pf_sldcache.right_idx)
            pf_sldcache.right_idx = pf_sldcache.cache[i].prev;
        free_slide(i);
        return true;
    } else
        return false;
}


/**
 Free all slides ranked above the given priority.
*/
static void free_all_slide_prio(int prio)
{
    while (free_slide_prio(prio))
    {;;}
}


/**
 Read the pfraw image given as filename and return the hid of the buffer
 */
static int read_pfraw(char* filename, int prio)
{
    struct pfraw_header bmph;
    int fh = rb->open(filename, O_RDONLY);
    if( fh < 0 ) {
        /* pf_cfg.cache_version = CACHE_UPDATE; -- don't invalidate on missing pfraw */
        return empty_slide_hid;
    }
    else
        rb->read(fh, &bmph, sizeof(struct pfraw_header));

    int size =  sizeof(struct dim) +
                sizeof( pix_t ) * bmph.width * bmph.height;

    int hid;
    do {
        hid = rb->buflib_alloc(&buf_ctx, size);
    } while (hid < 0 && free_slide_prio(prio));

    if (hid < 0) {
        rb->close( fh );
        return -1;
    }

    rb->yield(); /* allow audio to play when fast scrolling */
    struct dim *bm = rb->buflib_get_data(&buf_ctx, hid);

    bm->width = bmph.width;
    bm->height = bmph.height;
    pix_t *data = (pix_t*)(sizeof(struct dim) + (char *)bm);

    rb->read( fh, data , sizeof( pix_t ) * bm->width * bm->height );
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
    char pfraw_file[MAX_PATH];
    unsigned int hash_artist = mfnv(get_album_artist(slide_index));
    unsigned int hash_album = mfnv(get_album_name(slide_index));

    rb->snprintf(pfraw_file, sizeof(pfraw_file), CACHE_PREFIX "/%x%x.pfraw",
                 hash_album, hash_artist);

    int hid = read_pfraw(pfraw_file, prio);
    if (hid < 0)
        return false;

    pf_sldcache.cache[cache_index].hid = hid;

    if ( cache_index < SLIDE_CACHE_SIZE ) {
        pf_sldcache.cache[cache_index].index = slide_index;
    }

    return true;
}


/**
 Load the "next" slide that we can load, freeing old slides if needed, provided
 that they are further from center_index than the current slide
*/
bool load_new_slide(void)
{
    if (wants_to_quit)
        return false;

    int i = -1;

    if (pf_sldcache.center_idx != -1)
    {
        int next, prev;
        if (pf_sldcache.cache[pf_sldcache.center_idx].index != center_index)
        {
            if (pf_sldcache.cache[pf_sldcache.center_idx].index < center_index)
            {
                pf_sldcache.center_idx = _SEEK_RIGHT_WHILE(pf_sldcache.center_idx,
                                pf_sldcache.cache[next_].index <= center_index);
                if (pf_sldcache.center_idx == -1)
                    goto fatal_fail;

                prev = pf_sldcache.center_idx;
                next = pf_sldcache.cache[pf_sldcache.center_idx].next;
            }
            else
            {
                pf_sldcache.center_idx = _SEEK_LEFT_WHILE(pf_sldcache.center_idx,
                                pf_sldcache.cache[next_].index >= center_index);
                if (pf_sldcache.center_idx == -1)
                    goto fatal_fail;

                next = pf_sldcache.center_idx;
                prev = pf_sldcache.cache[pf_sldcache.center_idx].prev;
            }
            if (pf_sldcache.cache[pf_sldcache.center_idx].index != center_index)
            {
                if (pf_sldcache.free == -1)
                    free_slide_prio(0);

                i = lla_pop_head(&pf_sldcache.free);
                if (!load_and_prepare_surface(center_index, i, 0))
                    goto fail_and_refree;

                if (pf_sldcache.cache[next].index == -1)
                {
                    if (pf_sldcache.cache[prev].index == -1)
                        goto insert_first_slide;
                    else
                        next = pf_sldcache.cache[prev].next;
                }
                lla_insert(i, next);
                if (pf_sldcache.cache[i].index < pf_sldcache.cache[pf_sldcache.used].index)
                    pf_sldcache.used = i;

                pf_sldcache.center_idx = i;
                pf_sldcache.left_idx = i;
                pf_sldcache.right_idx = i;
                return true;
            }
        }
        int left, center, right;
        left = pf_sldcache.cache[pf_sldcache.left_idx].index;
        center = pf_sldcache.cache[pf_sldcache.center_idx].index;
        right = pf_sldcache.cache[pf_sldcache.right_idx].index;

        if (left > center)
            pf_sldcache.left_idx = pf_sldcache.center_idx;
        if (right < center)
            pf_sldcache.right_idx = pf_sldcache.center_idx;

        pf_sldcache.left_idx = _SEEK_LEFT_WHILE(pf_sldcache.left_idx,
            pf_sldcache.cache[ind_].index - 1 == pf_sldcache.cache[next_].index);

        pf_sldcache.right_idx = _SEEK_RIGHT_WHILE(pf_sldcache.right_idx,
            pf_sldcache.cache[ind_].index - 1 == pf_sldcache.cache[next_].index);
        if (pf_sldcache.right_idx == -1 || pf_sldcache.left_idx == -1)
            goto fatal_fail;


        /* update indices */
        left = pf_sldcache.cache[pf_sldcache.left_idx].index;
        center = pf_sldcache.cache[pf_sldcache.center_idx].index;
        right = pf_sldcache.cache[pf_sldcache.right_idx].index;

        int prio_l = center - left + 1;
        int prio_r = right - center + 1;
        if ((prio_l < prio_r || right >= number_of_slides) && left > 0)
        {
            if (pf_sldcache.free == -1 && !free_slide_prio(prio_l))
            {
                return false;
            }

            i = lla_pop_head(&pf_sldcache.free);
            if (load_and_prepare_surface(left - 1, i, prio_l))
            {
                lla_insert_before(&pf_sldcache.used, i, pf_sldcache.left_idx);
                pf_sldcache.left_idx = i;
                return true;
            }
        } else if(right < number_of_slides - 1)
        {
            if (pf_sldcache.free == -1 && !free_slide_prio(prio_r))
            {
                return false;
            }

            i = lla_pop_head(&pf_sldcache.free);
            if (load_and_prepare_surface(right + 1, i, prio_r))
            {
                lla_insert_after(i, pf_sldcache.right_idx);
                pf_sldcache.right_idx = i;
                return true;
            }
        }
    } else {
        i = lla_pop_head(&pf_sldcache.free);
        if (load_and_prepare_surface(center_index, i, 0))
        {
insert_first_slide:
            pf_sldcache.cache[i].next = i;
            pf_sldcache.cache[i].prev = i;
            pf_sldcache.center_idx = i;
            pf_sldcache.left_idx = i;
            pf_sldcache.right_idx = i;
            pf_sldcache.used = i;
            return true;
        }
    }
fail_and_refree:
    if (i != -1)
    {
        lla_insert_tail(&pf_sldcache.free, i);
    }
    return false;
fatal_fail:
    free_all_slide_prio(0);
    initialize_slide_cache();
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

    bmp = rb->buflib_get_data(&buf_ctx, hid);

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
    if ((i = pf_sldcache.used ) != -1)
    {
        int j = 0;
        do {
            if (pf_sldcache.cache[i].index == slide_index) {
                if (is_initial_slide && slide_index == center_index)
                    is_initial_slide = false;
                return get_slide(pf_sldcache.cache[i].hid);
            }
            i = pf_sldcache.cache[i].next;
            j++;
        } while (i != pf_sldcache.used && j < SLIDE_CACHE_SIZE);
    }
    if (is_initial_slide && slide_index == center_index)
        return NULL;
    else
        return get_slide(empty_slide_hid);
}

/**
 adjust slides so that they are in "steady state" position
 */
static void reset_slides(void)
{
    center_slide.angle = 0;
    center_slide.cx = 0;
    center_slide.cy = 0;
    center_slide.distance = 0;
    center_slide.slide_index = center_index;

    int i;
    for (i = 0; i < pf_cfg.num_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx = -(offsetX + pf_cfg.slide_spacing * i * PFREAL_ONE);
        si->cy = offsetY;
        si->slide_index = center_index - 1 - i;
        si->distance = 0;
    }

    for (i = 0; i < pf_cfg.num_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx = offsetX + pf_cfg.slide_spacing * i * PFREAL_ONE;
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
static void recalc_offsets(void)
{
    PFreal xs = PFREAL_HALF - DISPLAY_WIDTH * PFREAL_HALF;
    PFreal zo;
    PFreal xp = (DISPLAY_WIDTH * PFREAL_HALF - PFREAL_HALF +
                pf_cfg.center_margin * PFREAL_ONE) * pf_cfg.zoom / 100;
    PFreal cosr, sinr;

    itilt = 70 * IANGLE_MAX / 360;      /* approx. 70 degrees tilted */
    cosr = fcos(-itilt);
    sinr = fsin(-itilt);
    zo = CAM_DIST_R * 100 / pf_cfg.zoom - CAM_DIST_R +
        fmuln(MAXSLIDE_LEFT_R, sinr, PFREAL_SHIFT - 2, 0);
    offsetX = xp - fmul(xs, cosr) + fmuln(xp,
        zo + fmuln(xs, sinr, PFREAL_SHIFT - 2, 0), PFREAL_SHIFT - 2, 0)
        / CAM_DIST;
    offsetY = DISPLAY_WIDTH / 2 * (fsin(itilt) + PFREAL_ONE / 2);
}

/**
   Fade the given color by spreading the fb_data
   to an uint, multiply and compress the result back to a fb_data.
 */
static inline pix_t fade_color(pix_t c, unsigned a)
{
#if (LCD_PIXELFORMAT == RGB565SWAPPED)
    unsigned int result;
    c = swap16(c);
    a = (a + 2) & 0x1fc;
    result = ((c & 0xf81f) * a) & 0xf81f00;
    result |= ((c & 0x7e0) * a) & 0x7e000;
    result >>= 8;
    return swap16(result);

#elif LCD_PIXELFORMAT == RGB565
    unsigned int result;
    a = (a + 2) & 0x1fc;
    result = ((c & 0xf81f) * a) & 0xf81f00;
    result |= ((c & 0x7e0) * a) & 0x7e000;
    result >>= 8;
    return result;

#elif (LCD_PIXELFORMAT == RGB888 || LCD_PIXELFORMAT == XRGB8888) // FIXME: check this
    unsigned int pixel = FB_UNPACK_SCALAR_LCD(c);
    unsigned int result;
    a = (a + 2) & 0x1fc;
    result  = ((pixel & 0xff00ff) * a) & 0xff00ff00;
    result |= ((pixel & 0x00ff00) * a) & 0x00ff0000;
    result >>= 8;
    return FB_SCALARPACK(result);

#else
    unsigned val = c;
    return MULUQ(val, a) >> 8;
#endif
}

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
static void render_slide(struct slide_data *slide, const int alpha)
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
    PFreal zo = PFREAL_ONE * slide->distance + CAM_DIST_R * 100 / pf_cfg.zoom
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

#if LCD_STRIDEFORMAT == VERTICAL_STRIDE
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
  Jump to the given slide_index
 */
static inline void set_current_slide(const int slide_index)
{
    int old_center_index = center_index;
    step = 0;
    center_index = fbound(0, slide_index, number_of_slides - 1);
    if (old_center_index != center_index)
    {
        rb->queue_remove_from_head(&thread_q, EV_WAKEUP);
        rb->queue_post(&thread_q, EV_WAKEUP, 0);
    }
    target = center_index;
    slide_frame = center_index << 16;
    reset_slides();
}


static void skip_animation_to_idle_state(void);
static bool sort_albums(int new_sorting, bool from_settings)
{
    int i, album_idx, artist_idx;
    char* current_album_name = NULL;
    char* current_album_artist = NULL;
    static const char* sort_options[] = {
        ID2P(LANG_ARTIST_PLUS_NAME),
        ID2P(LANG_ARTIST_PLUS_YEAR),
        ID2P(LANG_ID3_YEAR),
        ID2P(LANG_NAME)
    };

    /* Only change sorting once artwork has been inspected */
    if (aa_cache.inspected < pf_idx.album_ct)
    {
#ifdef USEGSLIB
        if (!from_settings)
            grey_show(false);
#endif
        rb->splash(HZ*2, rb->str(LANG_WAIT_FOR_CACHE));
#ifdef USEGSLIB
        if (!from_settings)
            grey_show(true);
#endif
        return false;
    }

    /* set idle state */
    if (pf_state == pf_show_tracks)
        free_borrowed_tracks();
    if (pf_state == pf_show_tracks ||
        pf_state == pf_cover_in ||
        pf_state == pf_cover_out)
        skip_animation_to_idle_state();
    else if (pf_state == pf_scrolling)
        set_current_slide(target);
    pf_state = pf_idle;

    pf_cfg.sort_albums_by = new_sorting;
    if (!from_settings)
    {
#ifdef USEGSLIB
        grey_show(false);
#if LCD_DEPTH > 1
        rb->lcd_set_background(N_BRIGHT(0));
        rb->lcd_set_foreground(N_BRIGHT(255));
#endif
        rb->lcd_clear_display();
        rb->lcd_update();
#endif
        rb->splash(HZ, sort_options[pf_cfg.sort_albums_by]);
#ifdef USEGSLIB
        grey_show(true);
#endif
    }

    current_album_artist = get_album_artist(center_index);
    current_album_name = get_album_name(center_index);

    end_pf_thread(); /* stop loading of covers  */

    rb->qsort(pf_idx.album_index, pf_idx.album_ct,
                  sizeof(struct album_data), compare_albums);

    /* Empty cache and restart cover loading thread */
    rb->buflib_init(&buf_ctx, (void *)pf_idx.buf, pf_idx.buf_sz);
    empty_slide_hid = read_pfraw(EMPTY_SLIDE, 0);
    initialize_slide_cache();
    is_initial_slide = true;
    create_pf_thread();

    /* Go to previously selected slide */
    for (i = 0; i < pf_idx.album_ct; i++ )
    {
        album_idx = pf_idx.album_index[i].name_idx;
        artist_idx = pf_idx.album_index[i].artist_idx;

        if(!rb->strcmp(pf_idx.album_names + album_idx, current_album_name) &&
            !rb->strcmp(pf_idx.artist_names + artist_idx, current_album_artist))
        {
            set_current_slide(i);
            pf_cfg.last_album = i;
        }
    }
    return true;
}

/**
  Start the animation for changing slides
 */
static void start_animation(void)
{
    step = (target < center_slide.slide_index) ? -1 : 1;
    pf_state = pf_scrolling;
}

static void update_scroll_animation(void);

/**
  Go to the previous slide
 */
static void show_previous_slide(void)
{
    if (step == 0) {
        if (center_index > 0) {
            target = center_index - 1;
            start_animation();
        }
    } else if ( step > 0 ) {
        target = center_index;
        step = (target <= center_slide.slide_index) ? -1 : 1;
        if (step < 0)
            update_scroll_animation();
    } else {
        target = fmax(0, center_index - 2);
    }
}


/**
  Go to the next slide
 */
static void show_next_slide(void)
{
    if (step == 0) {
        if (center_index < number_of_slides - 1) {
            target = center_index + 1;
            start_animation();
        }
    } else if ( step < 0 ) {
        target = center_index;
        step = (target < center_slide.slide_index) ? -1 : 1;
        if (step > 0)
            update_scroll_animation();
    } else {
        target = fmin(center_index + 2, number_of_slides - 1);
    }
}


/**
  Render the slides. Updates only the offscreen buffer.
*/
static void render_all_slides(void)
{
    mylcd_set_background(G_BRIGHT(0));
    /* TODO: Optimizes this by e.g. invalidating rects */
    mylcd_clear_display();

    int nleft = pf_cfg.num_slides;
    int nright = pf_cfg.num_slides;

    int alpha;
    int index;
    if (step == 0) {
        /* no animation, boring plain rendering */
        for (index = nleft - 2; index >= 0; index--) {
            alpha = (index < nleft - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha > 0 )
                render_slide(&left_slides[index], alpha);
        }
        for (index = nright - 2; index >= 0; index--) {
            alpha = (index < nright - 2) ? 256 : 128;
            alpha -= extra_fade;
            if (alpha > 0 )
                render_slide(&right_slides[index], alpha);
        }
    } else {
        /* the first and last slide must fade in/fade out */

        /* if step<0 and nleft==1, left_slides[0] is fading in  */
        alpha = ((step > 0) ? 0 : ((nleft == 1) ? 256 : 128)) - fade / 2;
        for (index = nleft - 1; index >= 0; index--) {
            if (alpha > 0)
                render_slide(&left_slides[index], alpha);
            alpha += 128;
            if (alpha > 256) alpha = 256;
        }
        /* if step>0 and nright==1, right_slides[0] is fading in  */
        alpha = ((step > 0) ? ((nright == 1) ? 128 : 0) : -128) + fade / 2;
        for (index = nright - 1; index >= 0; index--) {
            if (alpha > 0)
                render_slide(&right_slides[index], alpha);
            alpha += 128;
            if (alpha > 256) alpha = 256;
        }
    }
    alpha = 256;
    if (step != 0 && pf_cfg.num_slides <= 2) /* fading out center slide */
        alpha = (step > 0) ? 256 - fade / 2 : 128 + fade / 2;
    render_slide(&center_slide, alpha);
}


/**
  Updates the animation effect. Call this periodically from a timer.
*/
static void update_scroll_animation(void)
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
        for (i = 0; i < pf_cfg.num_slides; i++)
            left_slides[i].slide_index = center_index - 1 - i;
        for (i = 0; i < pf_cfg.num_slides; i++)
            right_slides[i].slide_index = center_index + 1 + i;
    }

    center_slide.angle = (step * tick * itilt) >> 16;
    center_slide.cx = -step * fmul(offsetX, ftick);
    center_slide.cy = fmul(offsetY, ftick);

    if (center_index == target) {
        reset_slides();
        pf_state = pf_idle;
        slide_frame = center_index << 16;
        step = 0;
        fade = 256;
        return;
    }

    for (i = 0; i < pf_cfg.num_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx =
            -(offsetX + pf_cfg.slide_spacing * i * PFREAL_ONE + step
                        * pf_cfg.slide_spacing * ftick);
        si->cy = offsetY;
    }

    for (i = 0; i < pf_cfg.num_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx =
            offsetX + pf_cfg.slide_spacing * i * PFREAL_ONE - step
                      * pf_cfg.slide_spacing * ftick;
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
static void cleanup(void)
{
    wants_to_quit = true;
    if (buf_ctx_locked)
        buf_ctx_unlock();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    end_pf_thread();

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

#ifdef USEGSLIB
    grey_release();
#endif
}

static void skip_animation_to_show_tracks(void);
static void adjust_album_display_for_setting(int old_val, int new_val)
{
    if (old_val == new_val)
        return;

    reset_track_list();
    recalc_offsets();
    reset_slides();

    if (pf_state == pf_show_tracks)
        skip_animation_to_show_tracks();
}

/**
  Shows the settings menu
 */
static int settings_menu(void)
{
    int selection = 0;
    int old_val;

    MENUITEM_STRINGLIST(settings_menu, "PictureFlow Settings", NULL,
                        ID2P(LANG_SHOW_ALBUM_TITLE),
                        ID2P(LANG_SHOW_YEAR_IN_ALBUM_TITLE),
                        ID2P(LANG_SORT_ALBUMS_BY),
                        ID2P(LANG_YEAR_SORT_ORDER),
                        ID2P(LANG_DISPLAY_FPS),
                        ID2P(LANG_SPACING),
                        ID2P(LANG_CENTRE_MARGIN),
                        ID2P(LANG_NUMBER_OF_SLIDES),
                        ID2P(LANG_ZOOM),
                        ID2P(LANG_RESIZE_COVERS),
                        ID2P(LANG_REBUILD_CACHE),
                        ID2P(LANG_UPDATE_CACHE),
                        ID2P(LANG_WPS_INTEGRATION),
                        ID2P(LANG_BACKLIGHT));

    static const struct opt_items album_name_options[] = {
        { STR(LANG_HIDE_ALBUM_TITLE_NEW) },
        { STR(LANG_SHOW_AT_THE_BOTTOM_NEW) },
        { STR(LANG_SHOW_AT_THE_TOP_NEW) },
        { STR(LANG_SHOW_ALL_AT_THE_TOP) },
        { STR(LANG_SHOW_ALL_AT_THE_BOTTOM) },
    };
    static const struct opt_items sort_options[] = {
        { STR(LANG_ARTIST_PLUS_NAME) },
        { STR(LANG_ARTIST_PLUS_YEAR) },
        { STR(LANG_ID3_YEAR) },
        { STR(LANG_NAME) }
    };
    static const struct opt_items year_sort_order_options[] = {
        { STR(LANG_ASCENDING) },
        { STR(LANG_DESCENDING) }
    };
    static const struct opt_items wps_options[] = {
        { STR(LANG_OFF) },
        { STR(LANG_DIRECT) },
        { STR(LANG_VIA_TRACK_LIST) }
    };
    static const struct opt_items backlight_options[] = {
        { STR(LANG_ALWAYS_ON) },
        { STR(LANG_NORMAL) },
    };

    do {
        selection=rb->do_menu(&settings_menu,&selection, NULL, false);
        switch(selection) {
            case 0:
                old_val = pf_cfg.show_album_name;
                rb->set_option(rb->str(LANG_SHOW_ALBUM_TITLE),
                      &pf_cfg.show_album_name, RB_INT, album_name_options, 5, NULL);
                adjust_album_display_for_setting(old_val, pf_cfg.show_album_name);
                break;
            case 1:
                rb->set_bool(rb->str(LANG_SHOW_YEAR_IN_ALBUM_TITLE), &pf_cfg.show_year);
                break;
            case 2:
                old_val = pf_cfg.sort_albums_by;
                rb->set_option(rb->str(LANG_SORT_ALBUMS_BY),
                      &pf_cfg.sort_albums_by, RB_INT, sort_options, 4, NULL);
                if (old_val != pf_cfg.sort_albums_by &&
                    !sort_albums(pf_cfg.sort_albums_by, true))
                    pf_cfg.sort_albums_by = old_val;
                break;
            case 3:
                old_val = pf_cfg.year_sort_order;
                rb->set_option(rb->str(LANG_YEAR_SORT_ORDER),
                      &pf_cfg.year_sort_order, RB_INT, year_sort_order_options, 2, NULL);
                if (old_val != pf_cfg.year_sort_order &&
                    !sort_albums(pf_cfg.sort_albums_by, true))
                    pf_cfg.year_sort_order = old_val;
                break;
            case 4:
                old_val = pf_cfg.show_fps;
                rb->set_bool(rb->str(LANG_DISPLAY_FPS), &pf_cfg.show_fps);
                if (old_val != pf_cfg.show_fps)
                    reset_track_list();
                break;

            case 5:
                old_val = pf_cfg.slide_spacing;
                rb->set_int(rb->str(LANG_SPACING), "", 1,
                            &pf_cfg.slide_spacing,
                            NULL, 1, 0, 100, NULL );
                adjust_album_display_for_setting(old_val, pf_cfg.slide_spacing);
                break;

            case 6:
                old_val = pf_cfg.center_margin;
                rb->set_int(rb->str(LANG_CENTRE_MARGIN), "", 1,
                            &pf_cfg.center_margin,
                            NULL, 1, 0, 80, NULL );
                adjust_album_display_for_setting(old_val, pf_cfg.center_margin);
                break;

            case 7:
                old_val = pf_cfg.num_slides;
                rb->set_int(rb->str(LANG_NUMBER_OF_SLIDES), "", 1,
                        &pf_cfg.num_slides, NULL, 1, 1, MAX_SLIDES_COUNT, NULL );
                adjust_album_display_for_setting(old_val, pf_cfg.num_slides);
                break;

            case 8:
                old_val = pf_cfg.zoom;
                rb->set_int(rb->str(LANG_ZOOM), "", 1, &pf_cfg.zoom,
                            NULL, 1, 10, 300, NULL );
                adjust_album_display_for_setting(old_val, pf_cfg.zoom);
                break;

            case 9:
                old_val = pf_cfg.resize;
                rb->set_bool(rb->str(LANG_RESIZE_COVERS), &pf_cfg.resize);
                if (old_val == pf_cfg.resize) /* changed? */
                    break;
                /* fallthrough if changed, since cache needs to be rebuilt */
            case 10:
                pf_cfg.update_albumart = false;
                pf_cfg.cache_version = CACHE_REBUILD;
                rb->remove(EMPTY_SLIDE);
                configfile_save(CONFIG_FILE, config,
                                CONFIG_NUM_ITEMS, CONFIG_VERSION);
                rb->splash(HZ, ID2P(LANG_CACHE_REBUILT_NEXT_RESTART));
                break;
            case 11:
                pf_cfg.update_albumart = true;
                pf_cfg.cache_version = CACHE_REBUILD;
                rb->remove(EMPTY_SLIDE);
                configfile_save(CONFIG_FILE, config,
                                CONFIG_NUM_ITEMS, CONFIG_VERSION);
                rb->splash(HZ, ID2P(LANG_CACHE_REBUILT_NEXT_RESTART));
                break;
            case 12:
                rb->set_option(rb->str(LANG_WPS_INTEGRATION),
                               &pf_cfg.auto_wps, RB_INT, wps_options, 3, NULL);
                break;
            case 13:
                rb->set_option(rb->str(LANG_BACKLIGHT),
                        &pf_cfg.backlight_mode, RB_INT, backlight_options, 2, NULL);
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
enum {
    PF_SHOW_TRACKS_WHILE_BROWSING,
    PF_GOTO_LAST_ALBUM,
    PF_GOTO_WPS,
#if PF_PLAYBACK_CAPABLE
    PF_MENU_PLAYBACK_CONTROL,
#endif
    PF_MENU_SETTINGS,
    PF_MENU_QUIT,
};

static int main_menu(void)
{
    int selection = 0;
    int result, curr_album;

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif

    MENUITEM_STRINGLIST(main_menu, "PictureFlow Main Menu", NULL,
                        ID2P(LANG_SHOW_TRACKS_WHILE_BROWSING),
                        ID2P(LANG_GOTO_LAST_ALBUM),
                        ID2P(LANG_GOTO_WPS),
#if PF_PLAYBACK_CAPABLE
                        ID2P(LANG_PLAYBACK_CONTROL),
#endif
                        ID2P(LANG_SETTINGS),
                        ID2P(LANG_MENU_QUIT));
    while (1)  {
        switch (rb->do_menu(&main_menu,&selection, NULL, false)) {
            case PF_SHOW_TRACKS_WHILE_BROWSING:
                if (pf_state != pf_show_tracks)
                {
                    if (pf_state == pf_scrolling)
                        set_current_slide(target);

                    skip_animation_to_show_tracks();
                }
                show_tracks_while_browsing = true;
                return 0;
            case PF_GOTO_LAST_ALBUM:
                if (pf_state == pf_scrolling)
                    curr_album = target;
                else
                    curr_album = center_index;

                if (pf_state == pf_show_tracks)
                    free_borrowed_tracks();
                if (pf_state == pf_show_tracks ||
                    pf_state == pf_cover_in ||
                    pf_state == pf_cover_out)
                    skip_animation_to_idle_state();

                set_current_slide(pf_cfg.last_album);
                pf_cfg.last_album = curr_album;

                pf_state = pf_idle;
                return 0;
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
            case PF_MENU_QUIT:
                return -1;

            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;

            default:
                return 0;
        }
    }
}

#define ZOOMIN_FRAME_COUNT  19
#define ZOOMIN_FRAME_DIST   -5
#define ZOOMIN_FRAME_ANGLE  1
#define ZOOMIN_FRAME_FADE   13

#define ROTATE_FRAME_COUNT  15
#define ROTATE_FRAME_ANGLE  16

#define KEYFRAME_COUNT ZOOMIN_FRAME_COUNT + ROTATE_FRAME_COUNT

/**
   Animation step for zooming into the current cover
 */
static void update_cover_in_animation(void)
{
    cover_animation_keyframe++;

    if(cover_animation_keyframe <= ZOOMIN_FRAME_COUNT)
    {
        center_slide.distance += ZOOMIN_FRAME_DIST;
        center_slide.angle +=    ZOOMIN_FRAME_ANGLE;
        extra_fade +=            ZOOMIN_FRAME_FADE;
    }
    else if(cover_animation_keyframe <= KEYFRAME_COUNT)
        center_slide.angle += ROTATE_FRAME_ANGLE;
    else
    {
        cover_animation_keyframe = 0;
        pf_state = pf_show_tracks;
    }
}

/**
   Animation step for zooming out the current cover
 */
static void update_cover_out_animation(void)
{
    cover_animation_keyframe++;

    if(cover_animation_keyframe <= ROTATE_FRAME_COUNT)
        center_slide.angle -= ROTATE_FRAME_ANGLE;
    else if(cover_animation_keyframe <= KEYFRAME_COUNT)
    {
        center_slide.distance -= ZOOMIN_FRAME_DIST;
        center_slide.angle -=    ZOOMIN_FRAME_ANGLE;
        extra_fade -=            ZOOMIN_FRAME_FADE;
    }
    else
    {
        cover_animation_keyframe = 0;
        pf_state = pf_idle;
    }
}

/**
   Immediately show tracks and skip any animation frames
*/
static void skip_animation_to_show_tracks(void)
{
    pf_state = pf_show_tracks;
    cover_animation_keyframe = 0;

    extra_fade =            ZOOMIN_FRAME_COUNT * ZOOMIN_FRAME_FADE;
    center_slide.distance = ZOOMIN_FRAME_COUNT * ZOOMIN_FRAME_DIST;
    center_slide.angle   = (ZOOMIN_FRAME_COUNT * ZOOMIN_FRAME_ANGLE) +
                           (ROTATE_FRAME_COUNT * ROTATE_FRAME_ANGLE);
}

/**
   Immediately transition to idle state and skip any animation frames
*/
static void skip_animation_to_idle_state(void)
{
    pf_state = pf_idle;
    cover_animation_keyframe = 0;
    extra_fade = 0;
    set_current_slide(center_index);
}

/**
  Change direction during cover in/out animation
*/
static void reverse_animation(void)
{
    pf_state = pf_state == pf_cover_out ? pf_cover_in : pf_cover_out;
    cover_animation_keyframe = KEYFRAME_COUNT - cover_animation_keyframe;
}

/**
   Draw a blue gradient at y with height h
 */
static inline void draw_gradient(int y, int h)
{
    int r, inc, c;
    inc = (100 << 8) / h;
    c = 0;
    pf_tracks.sel_pulse = (pf_tracks.sel_pulse+1) % 10;
    int c2 = pf_tracks.sel_pulse - 5;
    for (r=0; r<h; r++) {
#ifdef HAVE_LCD_COLOR
        mylcd_set_foreground(G_PIX(c2+80-(c >> 9), c2+100-(c >> 9),
                                           c2+250-(c >> 8)));
#else
        mylcd_set_foreground(G_BRIGHT(c2+160-(c >> 8)));
#endif
        mylcd_hline(0, LCD_WIDTH, r+y);
        if ( r > h/2 )
            c-=inc;
        else
            c+=inc;
    }
}


static void track_list_yh(int char_height)
{
    bool needs_space = pf_cfg.show_fps || aa_cache.inspected < pf_idx.album_ct;

    switch (pf_cfg.show_album_name)
    {
        case ALBUM_NAME_HIDE:
            pf_tracks.list_y = (needs_space ? char_height : 0);
            pf_tracks.list_h = LCD_HEIGHT - pf_tracks.list_y;
            break;
        case ALBUM_NAME_BOTTOM:
            pf_tracks.list_y = (needs_space ? char_height : 0);
            pf_tracks.list_h = LCD_HEIGHT - pf_tracks.list_y - (char_height * 3);
            break;
        case ALBUM_AND_ARTIST_TOP:
            pf_tracks.list_y = char_height * 3;
            pf_tracks.list_h = LCD_HEIGHT - pf_tracks.list_y -
                           (needs_space ? char_height : 0);
            break;
        case ALBUM_AND_ARTIST_BOTTOM:
            pf_tracks.list_y = (needs_space ? char_height : 0);
            pf_tracks.list_h = LCD_HEIGHT - pf_tracks.list_y - (char_height * 3);
            break;
        case ALBUM_NAME_TOP:
        default:
            pf_tracks.list_y = char_height * 3;
            pf_tracks.list_h = LCD_HEIGHT - pf_tracks.list_y -
                           (needs_space ? char_height : 0);
            break;
    }
}

/**
    Reset the track list after a album change
 */
void reset_track_list(void)
{
    int char_height = rb->screens[SCREEN_MAIN]->getcharheight();
    int total_height;
    track_list_yh(char_height);
    pf_tracks.list_visible =
                         fmin( pf_tracks.list_h/char_height , pf_tracks.count );

    pf_tracks.list_start = 0;
    pf_tracks.sel = 0;
    pf_tracks.last_sel = -1;

    /* let the tracklist start more centered
     * if the screen isn't filled with tracks */
    total_height = pf_tracks.count*char_height;
    if (total_height < pf_tracks.list_h)
    {
        pf_tracks.list_y += (pf_tracks.list_h - total_height) / 2;
        pf_tracks.list_h = total_height;
    }
}

static void draw_album_text(void);
static void show_track_list_loading(void)
{
    int x = (LCD_WIDTH - mylcd_getstringsize(rb->str(LANG_WAIT), NULL, NULL)) / 2;
    mylcd_set_foreground(G_BRIGHT(255));
    int char_height = rb->screens[SCREEN_MAIN]->getcharheight();
    track_list_yh(char_height);
    mylcd_putsxy(x, pf_tracks.list_y + (pf_tracks.list_h  - char_height) / 2,
                 rb->str(LANG_WAIT));
    draw_album_text();
    mylcd_update();
    mylcd_clear_display();
}

/**
  Display the list of tracks
 */
static void show_track_list(void)
{
    mylcd_clear_display();
    if ( center_slide.slide_index != pf_tracks.cur_idx ) {
#ifdef HAVE_TC_RAMCACHE
        if (!rb->tagcache_is_in_ram())
#endif
            show_track_list_loading();
        create_track_index(center_slide.slide_index);
        if (pf_tracks.count == 0)
        {
            pf_state = pf_cover_out;
            free_borrowed_tracks();
            return;
        }
        reset_track_list();
    }
    int titletxt_w, titletxt_x, color, titletxt_h;
    titletxt_h = rb->screens[SCREEN_MAIN]->getcharheight();

    int titletxt_y = pf_tracks.list_y;
    int track_i;
    int fade;

    track_i = pf_tracks.list_start;
    for (; track_i < pf_tracks.list_visible + pf_tracks.list_start; track_i++)
    {
        char *trackname = get_track_name(track_i);
        if (track_i == pf_tracks.sel && !show_tracks_while_browsing) {
            if (pf_tracks.sel != pf_tracks.last_sel) {
                set_scroll_line(trackname, PF_SCROLL_TRACK);
                pf_tracks.last_sel = pf_tracks.sel;
            }
            draw_gradient(titletxt_y, titletxt_h);
            titletxt_x = get_scroll_line_offset(PF_SCROLL_TRACK);
            color = 255;
        }
        else {
            titletxt_w = mylcd_getstringsize(trackname, NULL, NULL);
            titletxt_x = (LCD_WIDTH-titletxt_w)/2;
            fade = (abs(pf_tracks.sel - track_i) * 200 / pf_tracks.count);
            color = 250 - fade;
        }
        mylcd_set_foreground(G_BRIGHT(color));
        mylcd_putsxy(titletxt_x,titletxt_y,trackname);
        titletxt_y += titletxt_h;
    }
}

static void select_next_track(void)
{
    if ( pf_tracks.sel < pf_tracks.count - 1 ) {
        pf_tracks.sel++;
        if (pf_tracks.sel==(pf_tracks.list_visible+pf_tracks.list_start))
            pf_tracks.list_start++;
    } else if (rb->global_settings->list_wraparound) {
        /* Rollover */
        pf_tracks.sel = 0;
        pf_tracks.list_start = 0;
    }
}

static void select_prev_track(void)
{
    if (pf_tracks.sel > 0 ) {
        if (pf_tracks.sel==pf_tracks.list_start) pf_tracks.list_start--;
        pf_tracks.sel--;
    } else if (rb->global_settings->list_wraparound) {
        /* Rolllover */
        pf_tracks.sel = pf_tracks.count - 1;
        pf_tracks.list_start = pf_tracks.count - pf_tracks.list_visible;
    }
}

static void select_next_album(void)
{
    if (center_index < number_of_slides - 1) {
        free_borrowed_tracks();
        target = center_index + 1;
        set_current_slide(target);
        skip_animation_to_show_tracks();
    }
}

static void select_prev_album(void)
{
    if (center_index > 0) {
        free_borrowed_tracks();
        target = center_index - 1;
        set_current_slide(target);
        skip_animation_to_show_tracks();
    }
}

#if PF_PLAYBACK_CAPABLE
static int show_id3_info(const char *selected_file)
{
    int i;
    unsigned long last_tick;
    const char *file_name;
    bool is_multiple_tracks = insert_whole_album && pf_tracks.count > 1;

    last_tick = *(rb->current_tick) + HZ/2;
    rb->splash_progress_set_delay(HZ / 2); /* wait 1/2 sec before progress */
    i = 0;
    do {
        file_name = i == 0 ? selected_file : get_track_filename(i);
        if (!rb->get_metadata(&id3, -1, file_name))
            return 0;

        if (is_multiple_tracks)
        {
            rb->splash_progress(i, pf_tracks.count,
                                "%s (%s)", rb->str(LANG_WAIT), rb->str(LANG_OFF_ABORT));
            if (TIME_AFTER(*(rb->current_tick), last_tick + HZ/4))
            {
                if (rb->action_userabort(TIMEOUT_NOBLOCK))
                    return 0;
                last_tick = *(rb->current_tick);
            }

            collect_id3(&id3, i == 0);
            rb->yield();
        }
    } while (++i < pf_tracks.count && is_multiple_tracks);

    if (is_multiple_tracks)
        finalize_id3(&id3);

    return rb->browse_id3(&id3, 0, 0, NULL, i) ? PLUGIN_USB_CONNECTED : 0;
}


static bool pf_current_playlist_insert(int position, bool queue, bool create_new)
{
    if (position == PLAYLIST_REPLACE)
    {
        if ((!create_new && rb->playlist_remove_all_tracks(NULL) == 0) ||
             (create_new && rb->playlist_create(NULL, NULL) == 0))
            position = PLAYLIST_INSERT_LAST;
        else
            return false;
    }

    if (!insert_whole_album)
        rb->playlist_insert_track(NULL, get_track_filename(pf_tracks.sel),
                                        position, queue, false);
    else
    {
        int i = 0;
        do {
            rb->yield();
            if (rb->playlist_insert_track(NULL, get_track_filename(i),
                    position, queue, false) < 0)
                break;
            if (position == PLAYLIST_INSERT_FIRST)
                position = PLAYLIST_INSERT;
        } while(++i < pf_tracks.count);
    }
    rb->playlist_sync(NULL);
    old_playlist = create_new ? center_slide.slide_index : -1;
    return true;
}


static int pf_add_to_playlist(const char* playlist, bool new_playlist)
{
    int fd;
    int result = 0;

    if (new_playlist)
        fd = rb->open_utf8(playlist, O_CREAT|O_WRONLY|O_TRUNC);
    else
        fd = rb->open(playlist, O_CREAT|O_WRONLY|O_APPEND, 0666);

    if(fd < 0)
        return -1;

    rb->reload_directory();

    if (!insert_whole_album)
    {
        if (rb->fdprintf(fd, "%s\n", get_track_filename(pf_tracks.sel)) <= 0)
            result = -1;
    }
    else
    {
        int i = 0;
        do {
            if (rb->fdprintf(fd, "%s\n", get_track_filename(i)) <= 0)
            {
                result = -1;
                break;
            }
            rb->yield();
        } while(++i < pf_tracks.count);
    }
    rb->close(fd);
    return result;
}


static bool track_list_ready(void)
{
    if (pf_state != pf_show_tracks)
    {
#ifdef HAVE_TC_RAMCACHE
        if (!rb->tagcache_is_in_ram())
#endif
            rb->splash(0, ID2P(LANG_WAIT));
        create_track_index(center_slide.slide_index);
        if (pf_tracks.count == 0)
        {
            free_borrowed_tracks();
            return false;
        }
        reset_track_list();
    }
    return true;
}


static bool context_menu_ready(void)
{
#ifdef USEGSLIB
    grey_show(false);
    rb->lcd_clear_display();
    rb->lcd_update();
#endif
    if (!track_list_ready())
    {
#ifdef USEGSLIB
        grey_show(true);
#endif
        return false;
    }
#if LCD_DEPTH > 1
#ifdef USEGSLIB
    rb->lcd_set_foreground(N_BRIGHT(0));
    rb->lcd_set_background(N_BRIGHT(255));
#endif
#endif
    insert_whole_album = (pf_state != pf_show_tracks) || show_tracks_while_browsing;
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);

    return true;
}

static void context_menu_cleanup(void)
{
    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);
    if (pf_state != pf_show_tracks)
        free_borrowed_tracks();
#ifdef USEGSLIB
    grey_show(true);
#endif
    mylcd_set_drawmode(DRMODE_FG);
}


static int context_menu(void)
{
    char album_name[MAX_PATH];
    char *file_name = get_track_filename(show_tracks_while_browsing ? 0 : pf_tracks.sel);
    int attr = FILE_ATTR_AUDIO;

    enum {
        PF_CURRENT_PLAYLIST = 0,
        PF_CATALOG,
        PF_ID3_INFO
    };
    MENUITEM_STRINGLIST(context_menu, ID2P(LANG_ONPLAY_MENU_TITLE), NULL,
                        ID2P(LANG_PLAYING_NEXT),
                        ID2P(LANG_ADD_TO_PL),
                        ID2P(LANG_MENU_SHOW_ID3_INFO));

    while (1)  {
        switch (rb->do_menu(&context_menu,
                            NULL, NULL, false)) {

            case PF_CURRENT_PLAYLIST:
                if (insert_whole_album && pf_tracks.count > 1)
                {
                    attr = ATTR_DIRECTORY;
                    file_name = NULL;
                }
                rb->onplay_show_playlist_menu(file_name, attr, &pf_current_playlist_insert);
                return 0;
            case PF_CATALOG:
                if (insert_whole_album)
                {
                    /* add a leading slash so that catalog_add_to_a_playlist
                       later prefills the name when creating a new playlist */
                    rb->snprintf(album_name, MAX_PATH, "/%s", get_album_name(center_index));
                    rb->fix_path_part(album_name, 1, sizeof(album_name) - 2);
                    file_name = album_name;
                    attr = ATTR_DIRECTORY;
                }

                rb->onplay_show_playlist_cat_menu(file_name, attr, &pf_add_to_playlist);
                return 0;
            case PF_ID3_INFO:
                return show_id3_info(file_name);
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                return 0;

        }
    }
}



/*
 * Puts selected album's tracks into a newly created playlist and starts playing
 */
static bool start_playback(bool return_to_WPS)
{
#ifdef USEGSLIB
    grey_show(false);
#if LCD_DEPTH > 1
    rb->lcd_set_background(N_BRIGHT(0));
    rb->lcd_set_foreground(N_BRIGHT(255));
#endif
    rb->lcd_clear_display();
    rb->lcd_update();
#else /* if !USEGSLIB */
    (void) return_to_WPS;
#endif

    if (!rb->warn_on_pl_erase() || !track_list_ready())
    {
#ifdef USEGSLIB
        grey_show(true);
#endif
        return false;
    }

    insert_whole_album = true;
    int start_index = pf_tracks.sel;
    bool shuffle = rb->global_settings->playlist_shuffle;
    /* can't reuse playlist if it may be out of sync with our track list */
    if (shuffle || center_slide.slide_index != old_playlist
                || (old_shuffle != shuffle))
    {
        if (!pf_current_playlist_insert(PLAYLIST_REPLACE, false, true))
        {
#ifdef USEGSLIB
            grey_show(true);
#endif
            return false;
        }
        if (shuffle)
            start_index = rb->playlist_shuffle(*rb->current_tick, pf_tracks.sel);
    }
    rb->playlist_start(start_index, 0, 0);
    old_shuffle = shuffle;
#ifdef USEGSLIB
    if (!return_to_WPS)
        grey_show(true);
#endif
    return true;
}
#endif /* PF_PLAYBACK_CAPABLE */

/**
   Draw the current album name
 */
static void draw_album_text(void)
{
    char album_and_year[MAX_PATH];

    if (pf_cfg.show_album_name == ALBUM_NAME_HIDE)
        return;

    static int prev_albumtxt_index = -1;
    static bool prev_show_year = false;
    int albumtxt_index;
    int char_height;
    int albumtxt_x, albumtxt_y, artisttxt_x;
    int album_idx = 0;

    char *albumtxt;
    char *artisttxt;
    int c;
    /* Draw album text */
    if ( pf_state == pf_scrolling ) {
        c = ((slide_frame & 0xffff )/ 255);
        if (step < 0) c = 255-c;
        if (c > 128 ) { /* half way to next slide .. still not perfect! */
            albumtxt_index = center_index+step;
            c = (c-128)*2;
        }
        else {
            albumtxt_index = center_index;
            c = (128-c)*2;
        }
    }
    else {
        albumtxt_index = center_index;
        c= 255;
    }
    albumtxt = get_album_name_idx(albumtxt_index, &album_idx);

    if (pf_cfg.show_year && pf_idx.album_index[albumtxt_index].year > 0)
    {
        rb->snprintf(album_and_year, sizeof(album_and_year), "%s – %d",
             albumtxt, pf_idx.album_index[albumtxt_index].year);
    } else
        rb->snprintf(album_and_year, sizeof(album_and_year), "%s", albumtxt);

    mylcd_set_foreground(G_BRIGHT(c));
    if (albumtxt_index != prev_albumtxt_index || pf_cfg.show_year != prev_show_year) {
        set_scroll_line(album_and_year, PF_SCROLL_ALBUM);
        prev_albumtxt_index = albumtxt_index;
        prev_show_year = pf_cfg.show_year;
    }

    char_height = rb->screens[SCREEN_MAIN]->getcharheight();
    switch(pf_cfg.show_album_name){
        case ALBUM_AND_ARTIST_TOP:
            albumtxt_y = 0;
            break;
        case ALBUM_NAME_BOTTOM:
        case ALBUM_AND_ARTIST_BOTTOM:
            albumtxt_y = (LCD_HEIGHT - (char_height * 5 / 2));
            break;
        case ALBUM_NAME_TOP:
        default:
            albumtxt_y = char_height / 2;
            break;
    }

    albumtxt_x = get_scroll_line_offset(PF_SCROLL_ALBUM);


    if ((pf_cfg.show_album_name == ALBUM_AND_ARTIST_TOP)
        || (pf_cfg.show_album_name == ALBUM_AND_ARTIST_BOTTOM)){

        if (album_idx != (int) pf_idx.album_untagged_idx)
            mylcd_putsxy(albumtxt_x, albumtxt_y, album_and_year);

        artisttxt = get_album_artist(albumtxt_index);
        set_scroll_line(artisttxt, PF_SCROLL_ARTIST);
        artisttxt_x = get_scroll_line_offset(PF_SCROLL_ARTIST);
        int y_offset = char_height + char_height/2;
        mylcd_putsxy(artisttxt_x, albumtxt_y + y_offset, artisttxt);
    } else {
        mylcd_putsxy(albumtxt_x, albumtxt_y, album_and_year);
    }
}


static void set_initial_slide(const char* selected_file)
{
    if (selected_file)
        set_current_slide(retrieve_id3(&id3, selected_file) ?
                            id3_get_index(&id3) :
                            pf_cfg.last_album);
    else
        set_current_slide(rb->audio_status() ?
                            id3_get_index(rb->audio_current_track()) :
                            pf_cfg.last_album);

}

/**
  Display an error message and wait for input.
*/
static void error_wait(const char *message)
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
static int pictureflow_main(const char* selected_file)
{
    int ret = SUCCESS;

    rb->lcd_setfont(FONT_UI);

    if ( ! rb->dir_exists( CACHE_PREFIX ) ) {
        if ( rb->mkdir( CACHE_PREFIX ) < 0 ) {
            error_wait("Could not create directory " CACHE_PREFIX);
            return PLUGIN_ERROR;
        }
    }

    rb->memset(&aa_cache, 0, sizeof(struct albumart_t));
    config_set_defaults(&pf_cfg);

    configfile_load(CONFIG_FILE, config, CONFIG_NUM_ITEMS, CONFIG_VERSION);

#ifdef HAVE_BACKLIGHT
    if(pf_cfg.backlight_mode == 0)
        backlight_ignore_timeout();
#endif

    rb->mutex_init(&buf_ctx_mutex);

    init_scroll_lines();
    init_reflect_table();

    /*Scan will trigger when no file is found or the option was activated*/
    if ((pf_cfg.cache_version != CACHE_VERSION)||(load_album_index() < 0)){
        rb->splash(HZ/2,"Creating index, please wait");
        ret = create_album_index();

        if (ret == 0){
            pf_cfg.cache_version = CACHE_REBUILD;
            if (save_album_index() < 0) {
                rb->splash(HZ, "Could not write index");
            };
        }
    }

    if (ret == ERROR_BUFFER_FULL) {
        error_wait("Not enough memory for album names");
        return PLUGIN_ERROR;
    } else if (ret == ERROR_NO_ALBUMS) {
        error_wait("No albums found. Please enable database");
        return PLUGIN_ERROR;
    } else if (ret == ERROR_USER_ABORT) {
        error_wait("User aborted.");
        return PLUGIN_ERROR;
    }

    number_of_slides = pf_idx.album_ct;

    size_t aa_bufsz = ALIGN_DOWN(pf_idx.buf_sz / 4, sizeof(long));
    if (aa_bufsz < DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(pix_t))
    {
        error_wait("Not enough memory for album art cache");
        return PLUGIN_ERROR;
    }

    ALIGN_BUFFER(pf_idx.buf, pf_idx.buf_sz, sizeof(long));
    aa_cache.buf = (char*) pf_idx.buf;
    aa_cache.buf_sz = aa_bufsz;

    pf_idx.buf += aa_bufsz;
    pf_idx.buf_sz -= aa_bufsz;

    if (!create_empty_slide(pf_cfg.cache_version != CACHE_VERSION)) {
        config_save(CACHE_REBUILD, false);
        error_wait("Could not load the empty slide");
        return PLUGIN_ERROR;
    }

    if ((pf_cfg.cache_version != CACHE_VERSION) && !create_albumart_cache()) {
        config_save(CACHE_REBUILD, false);
        error_wait("Could not create album art cache");
    } else if(aa_cache.inspected < pf_idx.album_ct) {
        rb->splash(HZ * 2, "Updating album art cache in background");
    }

    if (pf_cfg.cache_version != CACHE_VERSION)
    {
        config_save(CACHE_VERSION, pf_cfg.update_albumart);
    }

    rb->buflib_init(&buf_ctx, (void *)pf_idx.buf, pf_idx.buf_sz);

    if ((empty_slide_hid = read_pfraw(EMPTY_SLIDE, 0)) < 0)
    {
        error_wait("Unable to load empty slide image");
        return PLUGIN_ERROR;
    }

    if (!create_pf_thread()) {
        error_wait("Cannot create thread!");
        return PLUGIN_ERROR;
    }

    initialize_slide_cache();

    buffer = LCD_BUF;

    pf_state = pf_idle;

    pf_tracks.cur_idx = -1;
    pf_tracks.borrowed = 0;
    pf_tracks.used = 0;

    extra_fade = 0;
    slide_frame = 0;
    step = 0;
    target = 0;
    fade = 256;

    recalc_offsets();
    reset_slides();
    set_initial_slide(selected_file);

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

        update_scroll_lines();

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
                show_tracks_while_browsing = false;
                update_cover_out_animation();
                render_all_slides();
                instant_update = true;
                break;
            case pf_show_tracks:
                show_track_list();
                break;
            case pf_idle:
                show_tracks_while_browsing = false;
                render_all_slides();
                if (aa_cache.inspected < pf_idx.album_ct)
                {
                    buf_ctx_lock();
                    incremental_albumart_cache(false);
                    buf_ctx_unlock();
                }
                break;
        }

        /* Calculate FPS */
        if (current_update - last_update > update_interval) {
            fps = frames * HZ / (current_update - last_update);
            last_update = current_update;
            frames = 0;
        }
        /* Draw FPS or draw percentage of already built album cache */
        if (pf_cfg.show_fps || aa_cache.inspected < pf_idx.album_ct)
        {
#ifdef USEGSLIB
            mylcd_set_foreground(G_BRIGHT(255));
#else
            mylcd_set_foreground(G_PIX(255,0,0));
#endif
            if(aa_cache.inspected >= pf_idx.album_ct)
                 rb->snprintf(fpstxt, sizeof(fpstxt), "FPS: %d", fps);
            else
            {
                int progress_pct = 100 * aa_cache.inspected / pf_idx.album_ct;
                rb->snprintf(fpstxt, sizeof(fpstxt), "%d %%", progress_pct);
            }

            if (pf_cfg.show_album_name == ALBUM_NAME_TOP ||
                pf_cfg.show_album_name == ALBUM_AND_ARTIST_TOP)
                fpstxt_y = LCD_HEIGHT -
                           rb->screens[SCREEN_MAIN]->getcharheight();
            else
                fpstxt_y = 0;
            mylcd_putsxy(0, fpstxt_y, fpstxt);
        }
        draw_album_text();


        /* Copy offscreen buffer to LCD and give time to other threads */
        if (is_initial_slide == false)
            mylcd_update();
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
            if (show_tracks_while_browsing)
                show_tracks_while_browsing = false;
            else if (pf_state == pf_show_tracks)
            {
                pf_state = pf_cover_out;
                free_borrowed_tracks();
            }
            else if (pf_state == pf_cover_in)
                reverse_animation();
            else if (pf_state == pf_cover_out)
                skip_animation_to_idle_state();
            else if (pf_state == pf_idle || pf_state == pf_scrolling)
                return PLUGIN_OK;
            break;
        case PF_MENU:
#ifdef USEGSLIB
            grey_show(false);
#endif
            FOR_NB_SCREENS(i)
                rb->viewportmanager_theme_enable(i, true, NULL);
            ret = main_menu();
            FOR_NB_SCREENS(i)
                rb->viewportmanager_theme_undo(i, false);
            if ( ret == -2 ) return PLUGIN_GOTO_WPS;
            if ( ret == -1 ) return PLUGIN_OK;
            if ( ret != 0 ) return ret;
#ifdef USEGSLIB
            grey_show(true);
#endif
            mylcd_set_drawmode(DRMODE_FG);
            break;

        case PF_NEXT:
        case PF_NEXT_REPEAT:
            if ( pf_state == pf_show_tracks )
            {
                if (show_tracks_while_browsing)
                    select_next_album();
                else
                    select_next_track();
            }
            else if (pf_state == pf_cover_in)
                skip_animation_to_show_tracks();
            else if (pf_state == pf_cover_out)
                skip_animation_to_idle_state();

            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_next_slide();
            break;

        case PF_PREV:
        case PF_PREV_REPEAT:
            if ( pf_state == pf_show_tracks )
            {
                if (show_tracks_while_browsing)
                    select_prev_album();
                else
                    select_prev_track();
            }
            else if (pf_state == pf_cover_in)
                skip_animation_to_show_tracks();
            else if (pf_state == pf_cover_out)
                skip_animation_to_idle_state();

            if ( pf_state == pf_idle || pf_state == pf_scrolling )
                show_previous_slide();
            break;
        case PF_SORTING_NEXT:
            sort_albums((pf_cfg.sort_albums_by + 1) % SORT_VALUES_SIZE, false);
            break;
        case PF_SORTING_PREV:
            sort_albums((pf_cfg.sort_albums_by + (SORT_VALUES_SIZE - 1)) % SORT_VALUES_SIZE, false);
            break;
        case PF_JMP:
            if (pf_state == pf_idle || pf_state == pf_scrolling)
            {
                int new_idx = jmp_idx_next();
                if (new_idx != center_index)
                {
                    pf_state = pf_idle;
                    set_current_slide(new_idx);
                }
            }
            else if ( pf_state == pf_show_tracks )
                select_next_album();
            break;
        case PF_JMP_PREV:
            if (pf_state == pf_idle || pf_state == pf_scrolling)
            {
                int new_idx = jmp_idx_prev();
                if (new_idx != center_index)
                {
                    pf_state = pf_idle;
                    set_current_slide(new_idx);
                }
            }
            else if ( pf_state == pf_show_tracks )
                select_prev_album();
            else if (pf_state == pf_cover_in)
                reverse_animation();
            else if (pf_state == pf_cover_out)
                skip_animation_to_idle_state();
            break;
#if PF_PLAYBACK_CAPABLE
        case PF_CONTEXT:
            if (pf_state == pf_idle || pf_state == pf_scrolling ||
                pf_state == pf_show_tracks || pf_state == pf_cover_out)
            {
                if ( pf_state == pf_scrolling)
                {
                    set_current_slide(target);
                    pf_state = pf_idle;
                }
                else if (pf_state == pf_cover_out)
                    skip_animation_to_idle_state();

                if (context_menu_ready())
                {
                    ret = context_menu();
                    context_menu_cleanup();
                    if ( ret != 0 ) return ret;
                }
            }
            break;
#endif
        case PF_TRACKLIST:
            if ( pf_cfg.auto_wps == 1 && pf_state == pf_idle ) {
                pf_state = pf_cover_in;
                break;
            }
        case PF_SELECT:
            if ( pf_state == pf_idle || pf_state == pf_scrolling) {
                if (pf_state == pf_scrolling)
                    set_current_slide(target);
#if PF_PLAYBACK_CAPABLE
                if(pf_cfg.auto_wps == 1) {
                    if (start_playback(true))
                        return PLUGIN_GOTO_WPS;
                }
                else
#endif
                    pf_state = pf_cover_in;
            }
            else if (pf_state == pf_cover_out)
                reverse_animation();
            else if (pf_state == pf_cover_in)
                skip_animation_to_show_tracks();
            else if (pf_state == pf_show_tracks)
            {
                if (show_tracks_while_browsing)
                    show_tracks_while_browsing = false;
#if PF_PLAYBACK_CAPABLE
                else if(pf_cfg.auto_wps != 0) {
                    if (start_playback(true))
                        return PLUGIN_GOTO_WPS;
                }
                else
                    start_playback(false);
#endif
            }
            break;
        default:
            exit_on_usb(button);
            break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const void *parameter)
{
    struct viewport *vp_main = rb->lcd_set_viewport(NULL);
    lcd_fb = vp_main->buffer->fb_ptr;

    int ret;
    const char *file = parameter;

    void * buf;
    size_t buf_size;
    bool file_id3 = (parameter && (((char *) parameter)[0] == '/'));

    if (!check_database())
    {
        error_wait("Please enable database");
        return PLUGIN_OK;
    }

    atexit(cleanup);

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

    /* store buffer pointers and sizes */
    pf_idx.buf = buf;
    pf_idx.buf_sz = buf_size;

    ret = file_id3 ? pictureflow_main(file) : pictureflow_main(NULL);
    if ( ret == PLUGIN_OK || ret == PLUGIN_GOTO_WPS) {
        if (pf_state == pf_scrolling)
            pf_cfg.last_album = target;
        else
            pf_cfg.last_album = center_index;
        if (configfile_save(CONFIG_FILE, config, CONFIG_NUM_ITEMS,
                            CONFIG_VERSION))
        {
#ifdef USEGSLIB
            grey_show(false);
#endif
            rb->splash(HZ, ID2P(LANG_ERROR_WRITING_CONFIG));
            ret = PLUGIN_ERROR;
        }
    }
    return ret;
}
