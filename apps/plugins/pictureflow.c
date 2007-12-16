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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"
#include "pluginlib_actions.h"
#include "helper.h"
#include "bmp.h"
#include "picture.h"
#include "pictureflow_logo.h"
#include "pictureflow_emptyslide.h"


PLUGIN_HEADER

/******************************* Globals ***********************************/

static struct plugin_api *rb;   /* global api struct pointer */

const struct button_mapping *plugin_contexts[]
= {generic_actions, generic_directions};

#define NB_ACTION_CONTEXTS sizeof(plugin_contexts)/sizeof(plugin_contexts[0])

/* Key assignement */
#if (CONFIG_KEYPAD == IPOD_1G2G_PAD) \
 || (CONFIG_KEYPAD == IPOD_3G_PAD) \
 || (CONFIG_KEYPAD == IPOD_4G_PAD) \
 || (CONFIG_KEYPAD == SANSA_E200_PAD)
#define PICTUREFLOW_NEXT_ALBUM          PLA_UP
#define PICTUREFLOW_NEXT_ALBUM_REPEAT   PLA_UP_REPEAT
#define PICTUREFLOW_PREV_ALBUM          PLA_DOWN
#define PICTUREFLOW_PREV_ALBUM_REPEAT   PLA_DOWN_REPEAT
#else
#define PICTUREFLOW_NEXT_ALBUM          PLA_RIGHT
#define PICTUREFLOW_NEXT_ALBUM_REPEAT   PLA_RIGHT_REPEAT
#define PICTUREFLOW_PREV_ALBUM          PLA_LEFT
#define PICTUREFLOW_PREV_ALBUM_REPEAT   PLA_LEFT_REPEAT
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

/* maximum size of an slide */
#define MAX_IMG_WIDTH LCD_WIDTH
#define MAX_IMG_HEIGHT LCD_HEIGHT

#if (LCD_HEIGHT < 100)
#define PREFERRED_IMG_WIDTH 50
#define PREFERRED_IMG_HEIGHT 50
#else
#define PREFERRED_IMG_WIDTH 100
#define PREFERRED_IMG_HEIGHT 100
#endif

#define BUFFER_WIDTH LCD_WIDTH
#define BUFFER_HEIGHT LCD_HEIGHT

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

#define UNIQBUF_SIZE (64*1024)

#define EMPTY_SLIDE CACHE_PREFIX "/emptyslide.pfraw"
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

struct config_data {
    long avg_album_width;
    int spacing_between_slides;
    int extra_spacing_for_center_slide;
    int show_slides;
};

/** below we allocate the memory we want to use **/

static fb_data *buffer; /* for now it always points to the lcd framebuffer */
static PFreal rays[BUFFER_WIDTH];
static bool animation_is_active; /* an animation is currently running */
static struct slide_data center_slide;
static struct slide_data left_slides[MAX_SLIDES_COUNT];
static struct slide_data right_slides[MAX_SLIDES_COUNT];
static int slide_frame;
static int step;
static int target;
static int fade;
static int center_index; /* index of the slide that is in the center */
static int itilt;
static int zoom;
static PFreal offsetX;
static PFreal offsetY;
static bool show_fps; /* show fps in the main screen */
static int number_of_slides;

static struct slide_cache cache[SLIDE_CACHE_SIZE];
static int  slide_cache_in_use;

/* use long for aligning */
unsigned long thread_stack[THREAD_STACK_SIZE / sizeof(long)];
static int slide_cache_stack[SLIDE_CACHE_SIZE]; /* queue (as array) for scheduling load_surface */
static int slide_cache_stack_index;
struct mutex slide_cache_stack_lock;

static int empty_slide_hid;

struct thread_entry *thread_id;
struct event_queue thread_q;

static char tmp_path_name[MAX_PATH];

static long uniqbuf[UNIQBUF_SIZE];
static struct tagcache_search tcs;

static struct album_data album[MAX_ALBUMS];
static char album_names[MAX_ALBUMS*AVG_ALBUM_NAME_LENGTH];
static int album_count;

static fb_data *input_bmp_buffer;
static fb_data *output_bmp_buffer;
static int input_hid;
static int output_hid;
static struct config_data config;

static bool thread_is_running;


PFreal sinTable[] = { /* 10 */
    3,      9,     15,     21,     28,     34,     40,     47,
    53,     59,     65,     72,     78,     84,     90,     97,
    103,    109,    115,    122,    128,    134,    140,    147,
    153,    159,    165,    171,    178,    184,    190,    196,
    202,    209,    215,    221,    227,    233,    239,    245,
    251,    257,    264,    270,    276,    282,    288,    294,
    300,    306,    312,    318,    324,    330,    336,    342,
    347,    353,    359,    365,    371,    377,    383,    388,
    394,    400,    406,    412,    417,    423,    429,    434,
    440,    446,    451,    457,    463,    468,    474,    479,
    485,    491,    496,    501,    507,    512,    518,    523,
    529,    534,    539,    545,    550,    555,    561,    566,
    571,    576,    581,    587,    592,    597,    602,    607,
    612,    617,    622,    627,    632,    637,    642,    647,
    652,    656,    661,    666,    671,    675,    680,    685,
    690,    694,    699,    703,    708,    712,    717,    721,
    726,    730,    735,    739,    743,    748,    752,    756,
    760,    765,    769,    773,    777,    781,    785,    789,
    793,    797,    801,    805,    809,    813,    816,    820,
    824,    828,    831,    835,    839,    842,    846,    849,
    853,    856,    860,    863,    866,    870,    873,    876,
    879,    883,    886,    889,    892,    895,    898,    901,
    904,    907,    910,    913,    916,    918,    921,    924,
    927,    929,    932,    934,    937,    939,    942,    944,
    947,    949,    951,    954,    956,    958,    960,    963,
    965,    967,    969,    971,    973,    975,    977,    978,
    980,    982,    984,    986,    987,    989,    990,    992,
    994,    995,    997,    998,    999,   1001,   1002,   1003,
    1004,   1006,   1007,   1008,   1009,   1010,   1011,   1012,
    1013,   1014,   1015,   1015,   1016,   1017,   1018,   1018,
    1019,   1019,   1020,   1020,   1021,   1021,   1022,   1022,
    1022,   1023,   1023,   1023,   1023,   1023,   1023,   1023,
    1023,   1023,   1023,   1023,   1023,   1023,   1023,   1022,
    1022,   1022,   1021,   1021,   1020,   1020,   1019,   1019,
    1018,   1018,   1017,   1016,   1015,   1015,   1014,   1013,
    1012,   1011,   1010,   1009,   1008,   1007,   1006,   1004,
    1003,   1002,   1001,    999,    998,    997,    995,    994,
    992,    990,    989,    987,    986,    984,    982,    980,
    978,    977,    975,    973,    971,    969,    967,    965,
    963,    960,    958,    956,    954,    951,    949,    947,
    944,    942,    939,    937,    934,    932,    929,    927,
    924,    921,    918,    916,    913,    910,    907,    904,
    901,    898,    895,    892,    889,    886,    883,    879,
    876,    873,    870,    866,    863,    860,    856,    853,
    849,    846,    842,    839,    835,    831,    828,    824,
    820,    816,    813,    809,    805,    801,    797,    793,
    789,    785,    781,    777,    773,    769,    765,    760,
    756,    752,    748,    743,    739,    735,    730,    726,
    721,    717,    712,    708,    703,    699,    694,    690,
    685,    680,    675,    671,    666,    661,    656,    652,
    647,    642,    637,    632,    627,    622,    617,    612,
    607,    602,    597,    592,    587,    581,    576,    571,
    566,    561,    555,    550,    545,    539,    534,    529,
    523,    518,    512,    507,    501,    496,    491,    485,
    479,    474,    468,    463,    457,    451,    446,    440,
    434,    429,    423,    417,    412,    406,    400,    394,
    388,    383,    377,    371,    365,    359,    353,    347,
    342,    336,    330,    324,    318,    312,    306,    300,
    294,    288,    282,    276,    270,    264,    257,    251,
    245,    239,    233,    227,    221,    215,    209,    202,
    196,    190,    184,    178,    171,    165,    159,    153,
    147,    140,    134,    128,    122,    115,    109,    103,
    97,     90,     84,     78,     72,     65,     59,     53,
    47,     40,     34,     28,     21,     15,      9,      3,
    -4,    -10,    -16,    -22,    -29,    -35,    -41,    -48,
    -54,    -60,    -66,    -73,    -79,    -85,    -91,    -98,
    -104,   -110,   -116,   -123,   -129,   -135,   -141,   -148,
    -154,   -160,   -166,   -172,   -179,   -185,   -191,   -197,
    -203,   -210,   -216,   -222,   -228,   -234,   -240,   -246,
    -252,   -258,   -265,   -271,   -277,   -283,   -289,   -295,
    -301,   -307,   -313,   -319,   -325,   -331,   -337,   -343,
    -348,   -354,   -360,   -366,   -372,   -378,   -384,   -389,
    -395,   -401,   -407,   -413,   -418,   -424,   -430,   -435,
    -441,   -447,   -452,   -458,   -464,   -469,   -475,   -480,
    -486,   -492,   -497,   -502,   -508,   -513,   -519,   -524,
    -530,   -535,   -540,   -546,   -551,   -556,   -562,   -567,
    -572,   -577,   -582,   -588,   -593,   -598,   -603,   -608,
    -613,   -618,   -623,   -628,   -633,   -638,   -643,   -648,
    -653,   -657,   -662,   -667,   -672,   -676,   -681,   -686,
    -691,   -695,   -700,   -704,   -709,   -713,   -718,   -722,
    -727,   -731,   -736,   -740,   -744,   -749,   -753,   -757,
    -761,   -766,   -770,   -774,   -778,   -782,   -786,   -790,
    -794,   -798,   -802,   -806,   -810,   -814,   -817,   -821,
    -825,   -829,   -832,   -836,   -840,   -843,   -847,   -850,
    -854,   -857,   -861,   -864,   -867,   -871,   -874,   -877,
    -880,   -884,   -887,   -890,   -893,   -896,   -899,   -902,
    -905,   -908,   -911,   -914,   -917,   -919,   -922,   -925,
    -928,   -930,   -933,   -935,   -938,   -940,   -943,   -945,
    -948,   -950,   -952,   -955,   -957,   -959,   -961,   -964,
    -966,   -968,   -970,   -972,   -974,   -976,   -978,   -979,
    -981,   -983,   -985,   -987,   -988,   -990,   -991,   -993,
    -995,   -996,   -998,   -999,  -1000,  -1002,  -1003,  -1004,
    -1005,  -1007,  -1008,  -1009,  -1010,  -1011,  -1012,  -1013,
    -1014,  -1015,  -1016,  -1016,  -1017,  -1018,  -1019,  -1019,
    -1020,  -1020,  -1021,  -1021,  -1022,  -1022,  -1023,  -1023,
    -1023,  -1024,  -1024,  -1024,  -1024,  -1024,  -1024,  -1024,
    -1024,  -1024,  -1024,  -1024,  -1024,  -1024,  -1024,  -1023,
    -1023,  -1023,  -1022,  -1022,  -1021,  -1021,  -1020,  -1020,
    -1019,  -1019,  -1018,  -1017,  -1016,  -1016,  -1015,  -1014,
    -1013,  -1012,  -1011,  -1010,  -1009,  -1008,  -1007,  -1005,
    -1004,  -1003,  -1002,  -1000,   -999,   -998,   -996,   -995,
    -993,   -991,   -990,   -988,   -987,   -985,   -983,   -981,
    -979,   -978,   -976,   -974,   -972,   -970,   -968,   -966,
    -964,   -961,   -959,   -957,   -955,   -952,   -950,   -948,
    -945,   -943,   -940,   -938,   -935,   -933,   -930,   -928,
    -925,   -922,   -919,   -917,   -914,   -911,   -908,   -905,
    -902,   -899,   -896,   -893,   -890,   -887,   -884,   -880,
    -877,   -874,   -871,   -867,   -864,   -861,   -857,   -854,
    -850,   -847,   -843,   -840,   -836,   -832,   -829,   -825,
    -821,   -817,   -814,   -810,   -806,   -802,   -798,   -794,
    -790,   -786,   -782,   -778,   -774,   -770,   -766,   -761,
    -757,   -753,   -749,   -744,   -740,   -736,   -731,   -727,
    -722,   -718,   -713,   -709,   -704,   -700,   -695,   -691,
    -686,   -681,   -676,   -672,   -667,   -662,   -657,   -653,
    -648,   -643,   -638,   -633,   -628,   -623,   -618,   -613,
    -608,   -603,   -598,   -593,   -588,   -582,   -577,   -572,
    -567,   -562,   -556,   -551,   -546,   -540,   -535,   -530,
    -524,   -519,   -513,   -508,   -502,   -497,   -492,   -486,
    -480,   -475,   -469,   -464,   -458,   -452,   -447,   -441,
    -435,   -430,   -424,   -418,   -413,   -407,   -401,   -395,
    -389,   -384,   -378,   -372,   -366,   -360,   -354,   -348,
    -343,   -337,   -331,   -325,   -319,   -313,   -307,   -301,
    -295,   -289,   -283,   -277,   -271,   -265,   -258,   -252,
    -246,   -240,   -234,   -228,   -222,   -216,   -210,   -203,
    -197,   -191,   -185,   -179,   -172,   -166,   -160,   -154,
    -148,   -141,   -135,   -129,   -123,   -116,   -110,   -104,
    -98,    -91,    -85,    -79,    -73,    -66,    -60,    -54,
    -48,    -41,    -35,    -29,    -22,    -16,    -10,     -4,
};


/** code */


bool create_bmp(struct bitmap* input_bmp, char *target_path, bool resize);
int load_surface(int);

/* There are some precision issues when not using (long long) which in turn
   takes very long to compute... I guess the best solution would be to optimize
   the computations so it only requires a single long */
static inline PFreal fmul(PFreal a, PFreal b)
{
    return ((long long) (a)) * ((long long) (b)) >> PFREAL_SHIFT;
}

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


inline PFreal fsin(int iangle)
{
    while (iangle < 0)
        iangle += IANGLE_MAX;
    return sinTable[iangle & IANGLE_MASK];
}

inline PFreal fcos(int iangle)
{
    /* quarter phase shift */
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

        if ( (album[album_count].name_idx + l) > MAX_ALBUMS*AVG_ALBUM_NAME_LENGTH )
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
char* get_album_name(int slide_index)
{
    return album_names + album[slide_index].name_idx;
}


/**
  Determine filename of the album art for the given slide_index and
  store the result in buf.
  The algorithm looks for the first track of the given album uses
  find_albumart to find the filename.
 */
bool get_albumart_for_index_from_db(int slide_index, char *buf, int buflen)
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
        char size[9];
        rb->snprintf(size, sizeof(size), ".%dx%d", PREFERRED_IMG_WIDTH, PREFERRED_IMG_HEIGHT);
        rb->strncpy( (char*)&id3.path, tcs.result, MAX_PATH );
        id3.album = get_album_name(slide_index);
        if ( rb->search_albumart_files(&id3, size, buf, buflen) )
            result = true;
        else if ( rb->search_albumart_files(&id3, "", buf, buflen) )
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

    rb->lcd_set_background(LCD_RGBPACK(0,0,0));
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
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

    rb->lcd_set_foreground(LCD_RGBPACK(100,100,100));
    rb->lcd_drawrect(x, y, w+2, bar_height);
    rb->lcd_set_foreground(LCD_RGBPACK(165, 231, 82));

    rb->lcd_fillrect(x+1, y+1, step * w / album_count, bar_height-2);
    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));
    rb->lcd_update();
    rb->yield();
}

bool allocate_buffers(void)
{
    int input_size = MAX_IMG_WIDTH * MAX_IMG_HEIGHT * sizeof( fb_data );
    int output_size = MAX_IMG_WIDTH * MAX_IMG_HEIGHT * sizeof( fb_data ) * 2;

    input_hid = rb->bufalloc(NULL, input_size, TYPE_BITMAP);

    if (input_hid < 0)
        return false;

    if (rb->bufgetdata(input_hid, 0, (void *)&input_bmp_buffer) < input_size) {
        rb->bufclose(input_hid);
        return false;
    }

    output_hid = rb->bufalloc(NULL, output_size, TYPE_BITMAP);

    if (output_hid < 0) {
        rb->bufclose(input_hid);
        return false;
    }

    if (rb->bufgetdata(output_hid, 0, (void *)&output_bmp_buffer) < output_size) {
        rb->bufclose(output_hid);
        return false;
    }
    return true;
}


bool free_buffers(void)
{
    rb->bufclose(input_hid);
    rb->bufclose(output_hid);
    return true;
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

    for (i=0; i < album_count; i++)
    {
        draw_progressbar(i);
        if (!get_albumart_for_index_from_db(i, tmp_path_name, MAX_PATH))
            continue;

        input_bmp.data = (char *)input_bmp_buffer;
        ret = rb->read_bmp_file(tmp_path_name, &input_bmp,
                                sizeof(fb_data)*MAX_IMG_WIDTH*MAX_IMG_HEIGHT,
                                FORMAT_NATIVE);
        if (ret <= 0) {
            rb->splash(HZ, "couldn't read bmp");
            continue; /* skip missing/broken files */
        }


        rb->snprintf(tmp_path_name, sizeof(tmp_path_name), CACHE_PREFIX "/%d.pfraw", i);
        if (!create_bmp(&input_bmp, tmp_path_name, false)) {
            rb->splash(HZ, "couldn't write bmp");
        }
        config.avg_album_width += input_bmp.width;
        slides++;
        if ( rb->button_get(false) == PICTUREFLOW_MENU ) return false;
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
int slide_stack_get_index(int slide_index)
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
void slide_stack_push(int slide_index)
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
                /* but maybe it is possible to swap the given slide_index to the second place */
                tmp = slide_cache_stack[ slide_cache_stack_index -1 ];
                slide_cache_stack[ slide_cache_stack_index - 1  ] = slide_cache_stack[ i  ];
                slide_cache_stack[ i ] = tmp;
            }
        }
        else {
            /* if the center_index is not on top (i.e. already loaded) bring the slide_index to the top */
            slide_cache_stack[ slide_cache_stack_index  ] = slide_cache_stack[ i  ];
            slide_cache_stack[ i ] = tmp;
        }
    }
    else {
        /* slide_index is not on the stack */
        if ( slide_cache_stack_index >= SLIDE_CACHE_SIZE-1 ) {
            /* if we exceeded the stack size, clear the first half of the stack */
            slide_cache_stack_index = SLIDE_CACHE_SIZE/2;
            for (i = 0; i <= slide_cache_stack_index ; i++)
                slide_cache_stack[ i ] = slide_cache_stack[ i + slide_cache_stack_index  ];
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
inline int slide_stack_pop(void)
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
  Thus we have to queue the loading request in our thread while discarding the oldest slide.
 */
void request_surface(int slide_index)
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
    rb->queue_init(&thread_q, true);    /* put the thread's queue in the bcast list */
    if ((thread_id = rb->create_thread(
                           thread,
                           thread_stack,
                           sizeof(thread_stack),
                            0,
                           "Picture load thread"
                               IF_PRIO(, PRIORITY_BACKGROUND)
                               IF_COP(, CPU)
                                      )
        ) == NULL) {
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
        fb_data *d = (fb_data*)( bm->data ) + (y*bm->width);
        rb->write( fh, d, sizeof( fb_data ) * bm->width );
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

    int size =  sizeof(struct bitmap) + sizeof( fb_data ) * bmph.width * bmph.height;

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
        fb_data *d = (fb_data*)( bm->data ) + (y*bm->width);
        rb->read( fh, d , sizeof( fb_data ) * bm->width );
    }
    rb->close( fh );
    return hid;
}


/**
  Create the slide with its reflection for the given slide_index and filename
  and store it as pfraw in CACHE_PREFIX/[slide_index].pfraw
 */
bool create_bmp(struct bitmap *input_bmp, char *target_path, bool resize)
{
    struct bitmap output_bmp;

    output_bmp.format = input_bmp->format;
    output_bmp.data = (char *)output_bmp_buffer;

    if ( resize ) {
        /* resize image */
        output_bmp.width = config.avg_album_width;
        output_bmp.height = config.avg_album_width;
        simple_resize_bitmap(input_bmp, &output_bmp);

        /* Resized bitmap is now in the output buffer,
           copy it back to the input buffer */
        rb->memcpy(input_bmp_buffer, output_bmp_buffer,
                   config.avg_album_width * config.avg_album_width * sizeof(fb_data));
        input_bmp->data = (char *)input_bmp_buffer;
        input_bmp->width = output_bmp.width;
        input_bmp->height = output_bmp.height;
    }

    output_bmp.width = input_bmp->width * 2;
    output_bmp.height = input_bmp->height;

    fb_data *src = (fb_data *)input_bmp->data;
    fb_data *dst = (fb_data *)output_bmp.data;

    /* transpose the image, this is to speed-up the rendering
       because we process one column at a time
       (and much better and faster to work row-wise, i.e in one scanline) */
    int hofs = input_bmp->width / 3;
    rb->memset(dst, 0, sizeof(fb_data) * output_bmp.width * output_bmp.height);
    int x, y;
    for (x = 0; x < input_bmp->width; x++)
        for (y = 0; y < input_bmp->height; y++)
            dst[output_bmp.width * x + (hofs + y)] =
                    src[y * input_bmp->width + x];

    /* create the reflection */
    int ht = input_bmp->height - hofs;
    int hte = ht;
    for (x = 0; x < input_bmp->width; x++) {
        for (y = 0; y < ht; y++) {
            fb_data color = src[x + input_bmp->width * (input_bmp->height - y - 1)];
            int r = RGB_UNPACK_RED(color) * (hte - y) / hte * 3 / 5;
            int g = RGB_UNPACK_GREEN(color) * (hte - y) / hte * 3 / 5;
            int b = RGB_UNPACK_BLUE(color) * (hte - y) / hte * 3 / 5;
            dst[output_bmp.height + hofs + y + output_bmp.width * x] =
                    LCD_RGBPACK(r, g, b);
        }
    }
    return save_pfraw(target_path, &output_bmp);
}


/**
  Load the surface for the given slide_index into the cache at cache_index.
 */
static bool load_and_prepare_surface(int slide_index, int cache_index)
{
    rb->snprintf(tmp_path_name, sizeof(tmp_path_name), CACHE_PREFIX "/%d.pfraw", slide_index);

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
int load_surface(int slide_index)
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
struct bitmap *get_slide(int hid)
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
struct bitmap *surface(int slide_index)
{
    if (slide_index < 0)
        return 0;
    if (slide_index >= number_of_slides)
        return 0;

    int i;
    for (i = 0; i < slide_cache_in_use; i++) { /* maybe do the inverse mapping => implies dynamic allocation? */
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
    center_slide.slide_index = center_index;

    int i;
    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx = -(offsetX + config.spacing_between_slides * i * PFREAL_ONE);
        si->cy = offsetY;
        si->slide_index = center_index - 1 - i;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx = offsetX + config.spacing_between_slides * i * PFREAL_ONE;
        si->cy = offsetY;
        si->slide_index = center_index + 1 + i;
    }
}


/**
 Updates look-up table and other stuff necessary for the rendering.
 Call this when the viewport size or slide dimension is changed.
 */
void recalc_table(void)
{
    int w = (BUFFER_WIDTH + 1) / 2;
    int h = (BUFFER_HEIGHT + 1) / 2;
    int i;
    for (i = 0; i < w; i++) {
        PFreal gg = (PFREAL_HALF + i * PFREAL_ONE) / (2 * h);
        rays[w - i - 1] = -gg;
        rays[w + i] = gg;
    }

    itilt = 70 * IANGLE_MAX / 360;      /* approx. 70 degrees tilted */

    offsetX = config.avg_album_width / 2 * (PFREAL_ONE - fcos(itilt));
    offsetY = config.avg_album_width / 2 * fsin(itilt);
    offsetX += config.avg_album_width * PFREAL_ONE;
    offsetY += config.avg_album_width * PFREAL_ONE / 4;
    offsetX += config.extra_spacing_for_center_slide << PFREAL_SHIFT;
}


/**
  Render a single slide
*/
void render_slide(struct slide_data *slide, struct rect *result_rect,
                  int alpha, int col1, int col2)
{
    rb->memset(result_rect, 0, sizeof(struct rect));
    struct bitmap *bmp = surface(slide->slide_index);
    if (!bmp) {
        return;
    }
    fb_data *src = (fb_data *)bmp->data;

    int sw = bmp->height;
    int sh = bmp->width;

    int h = LCD_HEIGHT;
    int w = LCD_WIDTH;

    if (col1 > col2) {
        int c = col2;
        col2 = col1;
        col1 = c;
    }

    col1 = (col1 >= 0) ? col1 : 0;
    col2 = (col2 >= 0) ? col2 : w - 1;
    col1 = fmin(col1, w - 1);
    col2 = fmin(col2, w - 1);

    int distance = h * 100 / zoom;
    PFreal sdx = fcos(slide->angle);
    PFreal sdy = fsin(slide->angle);
    PFreal xs = slide->cx - bmp->width * sdx / 4;
    PFreal ys = slide->cy - bmp->width * sdy / 4;
    PFreal dist = distance * PFREAL_ONE;


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
            hity = -fdiv(( rays[x] * distance
                         - slide->cx
                         + slide->cy * sdx / sdy), fk);
        }

        dist = distance * PFREAL_ONE + hity;
        if (dist < 0)
            continue;

        PFreal hitx = fmul(dist, rays[x]);

        PFreal hitdist = fdiv(hitx - slide->cx, sdx);

        int column = sw / 2 + (hitdist >> PFREAL_SHIFT);
        if (column >= sw)
            break;

        if (column < 0)
            continue;

        result_rect->right = x;
        if (!flag)
            result_rect->left = x;
        flag = true;

        int y1 = h / 2;
        int y2 = y1 + 1;
        fb_data *pixel1 = &buffer[y1 * BUFFER_WIDTH + x];
        fb_data *pixel2 = &buffer[y2 * BUFFER_WIDTH + x];
        int pixelstep = pixel2 - pixel1;

        int center = (sh / 2);
        int dy = dist / h;
        int p1 = center * PFREAL_ONE - dy / 2;
        int p2 = center * PFREAL_ONE + dy / 2;


        const fb_data *ptr = &src[column * bmp->width];
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
                fb_data c1 = ptr[p1 >> PFREAL_SHIFT];
                fb_data c2 = ptr[p2 >> PFREAL_SHIFT];

                int r1 = RGB_UNPACK_RED(c1) * alpha / 256;
                int g1 = RGB_UNPACK_GREEN(c1) * alpha / 256;
                int b1 = RGB_UNPACK_BLUE(c1) * alpha / 256;
                int r2 = RGB_UNPACK_RED(c2) * alpha / 256;
                int g2 = RGB_UNPACK_GREEN(c2) * alpha / 256;
                int b2 = RGB_UNPACK_BLUE(c2) * alpha / 256;

                *pixel1 = LCD_RGBPACK(r1, g1, b1);
                *pixel2 = LCD_RGBPACK(r2, g2, b2);
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
static inline void set_current_slide(int slide_index)
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
    if (!animation_is_active) {
        step = (target < center_slide.slide_index) ? -1 : 1;
        animation_is_active = true;
    }
}

/**
  Go to the previous slide
 */
void show_previous_slide(void)
{
    if (step >= 0) {
        if (center_index > 0) {
            target = center_index - 1;
            start_animation();
        }
    } else {
        target = fmax(0, center_index - 2);
    }
}


/**
  Go to the next slide
 */
void show_next_slide(void)
{
    if (step <= 0) {
        if (center_index < number_of_slides - 1) {
            target = center_index + 1;
            start_animation();
        }
    } else {
        target = fmin(center_index + 2, number_of_slides - 1);
    }
}


/**
  Return true if the rect has size 0
*/
bool is_empty_rect(struct rect *r)
{
    return ((r->left == 0) && (r->right == 0) && (r->top == 0)
            && (r->bottom == 0));
}


/**
  Render the slides. Updates only the offscreen buffer.
*/
void render(void)
{
    rb->lcd_set_background(LCD_RGBPACK(0,0,0));
    rb->lcd_clear_display(); /* TODO: Optimizes this by e.g. invalidating rects */

    int nleft = config.show_slides;
    int nright = config.show_slides;

    struct rect r;
    render_slide(&center_slide, &r, 256, -1, -1);
#ifdef DEBUG_DRAW
    rb->lcd_drawrect(r.left, r.top, r.right - r.left, r.bottom - r.top);
#endif
    int c1 = r.left;
    int c2 = r.right;
    int index;
    if (step == 0) {
        /* no animation, boring plain rendering */
        for (index = 0; index < nleft - 1; index++) {
            int alpha = (index < nleft - 2) ? 256 : 128;
            render_slide(&left_slides[index], &r, alpha, 0, c1 - 1);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                rb->lcd_drawrect(r.left, r.top, r.right - r.left, r.bottom - r.top);
#endif
                c1 = r.left;
            }
        }
        for (index = 0; index < nright - 1; index++) {
            int alpha = (index < nright - 2) ? 256 : 128;
            render_slide(&right_slides[index], &r, alpha, c2 + 1,
                         BUFFER_WIDTH);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                rb->lcd_drawrect(r.left, r.top, r.right - r.left, r.bottom - r.top);
#endif
                c2 = r.right;
            }
        }
    } else {
        if ( step < 0 ) c1 = BUFFER_WIDTH;
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
                rb->lcd_drawrect(r.left, r.top, r.right - r.left, r.bottom - r.top);
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
                         BUFFER_WIDTH);
            if (!is_empty_rect(&r)) {
#ifdef DEBUG_DRAW
                rb->lcd_drawrect(r.left, r.top, r.right - r.left, r.bottom - r.top);
#endif
                c2 = r.right;
            }
        }
    }
}


/**
  Updates the animation effect. Call this periodically from a timer.
*/
void update_animation(void)
{
    if (!animation_is_active)
        return;
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
        animation_is_active = false;
        step = 0;
        fade = 256;
        return;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &left_slides[i];
        si->angle = itilt;
        si->cx =
            -(offsetX + config.spacing_between_slides * i * PFREAL_ONE + step * config.spacing_between_slides * ftick);
        si->cy = offsetY;
    }

    for (i = 0; i < config.show_slides; i++) {
        struct slide_data *si = &right_slides[i];
        si->angle = -itilt;
        si->cx =
            offsetX + config.spacing_between_slides * i * PFREAL_ONE - step * config.spacing_between_slides * ftick;
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
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    int i;
    for (i = 0; i < slide_cache_in_use; i++) {
        rb->bufclose(cache[i].hid);
    }
    if ( empty_slide_hid != - 1)
        rb->bufclose(empty_slide_hid);
}


int create_empty_slide(bool force)
{
    if ( force || ! rb->file_exists( EMPTY_SLIDE ) )  {
        struct bitmap input_bmp;
        input_bmp.width = BMPWIDTH_pictureflow_emptyslide;
        input_bmp.height = BMPHEIGHT_pictureflow_emptyslide;
        input_bmp.format = FORMAT_NATIVE;
        input_bmp.data = (char*) &pictureflow_emptyslide;
        if ( ! create_bmp(&input_bmp, EMPTY_SLIDE, true) ) return false;
    }

    empty_slide_hid = read_pfraw( EMPTY_SLIDE );
    if (empty_slide_hid == -1 ) return false;

    return true;
}


/**
  Shows the settings menu
 */
int settings_menu(void) {
    int selection = 0;

    MENUITEM_STRINGLIST(settings_menu,"PictureFlow Settings",NULL,"Show FPS", "Spacing", "Center margin", "Number of slides", "Rebuild cache");

    do {
        selection=rb->do_menu(&settings_menu,&selection);
        switch(selection) {
            case 0:
                rb->set_bool("Show FPS", &show_fps);
                break;

            case 1:
                rb->set_int("Spacing between slides", "", 1, &(config.spacing_between_slides),
                            NULL, 1, 0, 100, NULL );
                recalc_table();
                reset_slides();
                break;

            case 2:
                rb->set_int("Center margin", "", 1, &(config.extra_spacing_for_center_slide),
                            NULL, 1, -50, 50, NULL );
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

    rb->lcd_set_foreground(LCD_RGBPACK(255,255,255));

    MENUITEM_STRINGLIST(main_menu,"PictureFlow Main Menu",NULL,
                        "Settings", "Return", "Quit");

    while (1)  {
        switch (rb->do_menu(&main_menu,&selection)) {
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

bool read_pfconfig(void)
{
    /* defaults */
    config.spacing_between_slides = 40;
    config.extra_spacing_for_center_slide = 0;
    config.show_slides = 3;
    int fh = rb->open( CONFIG_FILE, O_RDONLY );
    if ( fh < 0 ) { /* no config yet */
        return true;
    }
    int ret = rb->read(fh, &config, sizeof(struct config_data));
    rb->close(fh);
    return ( ret == sizeof(struct config_data) );
}

bool write_pfconfig(void)
{
    int fh = rb->creat( CONFIG_FILE );
    if( fh < 0 ) return false;
    rb->write( fh, &config, sizeof( struct config_data ) );
    rb->close( fh );
    return true;
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
            rb->splash(HZ, "Could not create directory");
            return PLUGIN_ERROR;
        }
    }

    if (!read_pfconfig()) {
        rb->splash(HZ, "Error in config. Please delete " CONFIG_FILE);
        return PLUGIN_ERROR;
    }

    if (!allocate_buffers()) {
        rb->splash(HZ, "Could not allocate temporary buffers");
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

    if (!free_buffers()) {
        rb->splash(HZ, "Could not free temporary buffers");
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
    buffer = rb->lcd_framebuffer;
    animation_is_active = false;
    zoom = 100;
    center_index = 0;
    slide_frame = 0;
    step = 0;
    target = 0;
    fade = 256;
    show_fps = false;

    recalc_table();
    reset_slides();

    char fpstxt[10];
    char *albumtxt;
    int button;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    int frames = 0;
    long last_update = *rb->current_tick;
    long current_update;
    long update_interval = 100;
    int fps = 0;
    int albumtxt_w, albumtxt_h;
    int albumtxt_x = 0, albumtxt_y = 0;
    int albumtxt_dir = -1;
    int c;
    int prev_center_index = -1;

    while (true) {
        current_update = *rb->current_tick;
        frames++;
        update_animation();
        render();

        if (current_update - last_update > update_interval) {
            fps = frames * HZ / (current_update - last_update);
            last_update = current_update;
            frames = 0;
        }

        if (show_fps) {
            rb->lcd_set_foreground(LCD_RGBPACK(255, 0, 0));
            rb->snprintf(fpstxt, sizeof(fpstxt), "FPS: %d", fps);
            rb->lcd_putsxy(0, 0, fpstxt);
        }

        albumtxt = get_album_name(center_index);
        if ( animation_is_active ) {
            c = ((slide_frame & 0xffff )/ 256);
            if (step > 0) c = 255-c;
        }
        else c= 255;
        rb->lcd_set_foreground(LCD_RGBPACK(c,c,c));
        rb->lcd_getstringsize(albumtxt, &albumtxt_w, &albumtxt_h);
        if (center_index != prev_center_index) {
            albumtxt_x = 0;
            albumtxt_dir = -1;
            albumtxt_y = LCD_HEIGHT-albumtxt_h-10;
            prev_center_index = center_index;
        }
        if (albumtxt_w > LCD_WIDTH && ! animation_is_active ) {
            rb->lcd_putsxy(albumtxt_x, albumtxt_y , albumtxt);
            if ( albumtxt_w + albumtxt_x <= LCD_WIDTH ) albumtxt_dir = 1;
            else if ( albumtxt_x >= 0 ) albumtxt_dir = -1;
            albumtxt_x += albumtxt_dir;
        }
        else {
            rb->lcd_putsxy((LCD_WIDTH - albumtxt_w) /2, albumtxt_y , albumtxt);
        }

        rb->lcd_update();
        rb->yield();

        button = pluginlib_getaction(rb, animation_is_active ? 0 : HZ/16,
                                     plugin_contexts, NB_ACTION_CONTEXTS);

        switch (button) {
        case PICTUREFLOW_QUIT:
            return PLUGIN_OK;

        case PICTUREFLOW_MENU:
            ret = main_menu();
            if ( ret == -1 ) return PLUGIN_OK;
            if ( ret != 0 ) return i;
            break;

        case PICTUREFLOW_NEXT_ALBUM:
        case PICTUREFLOW_NEXT_ALBUM_REPEAT:
            show_next_slide();
            break;

        case PICTUREFLOW_PREV_ALBUM:
        case PICTUREFLOW_PREV_ALBUM_REPEAT:
            show_previous_slide();
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

enum plugin_status plugin_start(struct plugin_api *api, void *parameter)
{
    int ret;

    rb = api;                   /* copy to global api pointer */
    (void) parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(rb);     /* backlight control in lib/helper.c */
    ret = main();
    if ( ret == PLUGIN_OK ) {
        if (!write_pfconfig()) {
            rb->splash(HZ, "Error writing config.");
            return PLUGIN_ERROR;
        }
    }

    end_pf_thread();
    cleanup(NULL);
    return ret;
}
