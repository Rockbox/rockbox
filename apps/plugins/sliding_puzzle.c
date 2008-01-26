/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Vicentini Martin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "bmp.h"

#ifdef HAVE_LCD_BITMAP
PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define PUZZLE_QUIT BUTTON_OFF
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_F1
#define PUZZLE_PICTURE BUTTON_F2

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define PUZZLE_QUIT BUTTON_OFF
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_F1
#define PUZZLE_PICTURE BUTTON_F2

#elif CONFIG_KEYPAD == ONDIO_PAD
#define PUZZLE_QUIT BUTTON_OFF
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE_PICTURE_PRE BUTTON_MENU
#define PUZZLE_SHUFFLE (BUTTON_MENU | BUTTON_REPEAT)
#define PUZZLE_PICTURE (BUTTON_MENU | BUTTON_REL)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PUZZLE_QUIT BUTTON_OFF
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_SELECT
#define PUZZLE_PICTURE BUTTON_ON

#define PUZZLE_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PUZZLE_QUIT    (BUTTON_SELECT | BUTTON_MENU)
#define PUZZLE_UP      BUTTON_MENU
#define PUZZLE_DOWN    BUTTON_PLAY
#define PUZZLE_SHUFFLE (BUTTON_SELECT | BUTTON_LEFT)
#define PUZZLE_PICTURE (BUTTON_SELECT | BUTTON_RIGHT)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define PUZZLE_QUIT BUTTON_POWER
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_REC
#define PUZZLE_PICTURE BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define PUZZLE_QUIT BUTTON_POWER
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_SELECT
#define PUZZLE_PICTURE BUTTON_A

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define PUZZLE_QUIT BUTTON_POWER
#define PUZZLE_UP BUTTON_UP
#define PUZZLE_DOWN BUTTON_DOWN
#define PUZZLE_SHUFFLE BUTTON_REC
#define PUZZLE_PICTURE BUTTON_SELECT

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define PUZZLE_QUIT BUTTON_POWER
#define PUZZLE_UP BUTTON_SCROLL_UP
#define PUZZLE_DOWN BUTTON_SCROLL_DOWN
#define PUZZLE_SHUFFLE BUTTON_REW
#define PUZZLE_PICTURE BUTTON_PLAY

#endif

static struct plugin_api* rb;
#if LCD_DEPTH==1
/* for recorder, use rectangular image, 5x4 puzzle */
#define SPOTS_X 5
#define SPOTS_Y 4
#define SPOTS_WIDTH 16
#define SPOTS_HEIGHT 16
#define IMAGE_WIDTH 80
#define IMAGE_HEIGHT 64
#define IMAGE_SIZE 80
#else
/* for other targets, use a square image, 4x4 puzzle
   Puzzle image dimension is min(lcd_height,lcd_width)
   4x4 is more convenient than 5x4 for square puzzles
   Note: sliding_puzzle.bmp should be evenly divisible by SPOTS_X
   and SPOTS_Y, otherwise lcd_bitmap_part stride won't be correct */
#define SPOTS_X 4
#define SPOTS_Y 4
#define IMAGE_SIZE ( (LCD_WIDTH<LCD_HEIGHT)?LCD_WIDTH:LCD_HEIGHT )
#define IMAGE_WIDTH IMAGE_SIZE
#define IMAGE_HEIGHT IMAGE_SIZE
#define SPOTS_WIDTH (IMAGE_WIDTH/SPOTS_X)
#define SPOTS_HEIGHT (IMAGE_HEIGHT/SPOTS_Y)
#endif

#define NUM_SPOTS (SPOTS_X*SPOTS_Y)
#define HOLE_ID (NUM_SPOTS)
#define INITIAL_HOLE (HOLE_ID-1)

enum picmodes
{
    PICMODE_NUMERALS = 0,
    PICMODE_INITIAL_PICTURE,
    PICMODE_DEFAULT_PICTURE,
#ifdef HAVE_ALBUMART
    PICMODE_ALBUM_ART,
#endif
//    PICMODE_RANDOM,
    PICMODE_LAST_XXX /* placeholder */
};

static const char* const picmode_descriptions[] = {
    "Numerals",
    "Viewer Picture",
    "Default Picture",
#ifdef HAVE_ALBUMART
    "Album Art",
#endif
    "Shouldn't Get Here",
};

static int spots[NUM_SPOTS];
static int hole = INITIAL_HOLE, moves;
static char s[5];
static enum picmodes picmode = PICMODE_INITIAL_PICTURE;

static unsigned char img_buf[IMAGE_WIDTH*IMAGE_HEIGHT*sizeof(fb_data)]
__attribute__ ((aligned(16)));
#if LCD_DEPTH>1
static unsigned char temp_img_buf[LCD_WIDTH*LCD_HEIGHT*sizeof(fb_data)]
__attribute__ ((aligned(16)));
#endif
#ifdef HAVE_ALBUMART
static char albumart_path[MAX_PATH+1];
#endif
static char img_buf_path[MAX_PATH+1];

static const fb_data * puzzle_bmp_ptr;
extern const fb_data sliding_puzzle[];
/* initial_bmp_path points to selected bitmap if this game is launched
   as a viewer for a .bmp file, or NULL if game is launched regular way */
static const char * initial_bmp_path=NULL;

#ifdef HAVE_ALBUMART
const char * get_albumart_bmp_path(void)
{
    struct mp3entry* track = rb->audio_current_track();

    if (!track || !track->path || track->path[0] == '\0')
        return NULL;

    if (!rb->search_albumart_files(track, "", albumart_path, MAX_PATH ) )
        return NULL;

    albumart_path[ MAX_PATH ] = '\0';
    return albumart_path;
}
#endif

const char * get_random_bmp_path(void)
{
    return(initial_bmp_path);
}

static bool load_resize_bitmap(void)
{
    int rc;
    const char * filename = NULL;

    /* initially assume using the built-in default */
    puzzle_bmp_ptr = sliding_puzzle;

    switch( picmode ){
        /* some modes don't even need to touch disk and trivially succeed */
        case PICMODE_NUMERALS:
        case PICMODE_DEFAULT_PICTURE:
        default:
            return(true);

#ifdef HAVE_ALBUMART
        case PICMODE_ALBUM_ART:
            filename = get_albumart_bmp_path();
            break;
#endif
/*
        case PICMODE_RANDOM:
            if(NULL == (filename=get_random_bmp_path()) )
                filename = initial_bmp_path;
            break;
*/
        case PICMODE_INITIAL_PICTURE:
            filename = initial_bmp_path;
            break;
    };

    if( filename != NULL )
    {
        /* if we already loaded image before, don't touch disk */
        if( 0 == rb->strcmp( filename, img_buf_path ) )
        {
            puzzle_bmp_ptr = (const fb_data *)img_buf;
            return true;
        }

        struct bitmap main_bitmap;
        rb->memset(&main_bitmap,0,sizeof(struct bitmap));
        main_bitmap.data = img_buf;

#if LCD_DEPTH>1
        struct bitmap temp_bitmap;
        rb->memset(&temp_bitmap,0,sizeof(struct bitmap));
        temp_bitmap.data = temp_img_buf;

        main_bitmap.width = IMAGE_WIDTH;
        main_bitmap.height = IMAGE_HEIGHT;

        rc = rb->read_bmp_file( filename, &temp_bitmap, sizeof(temp_img_buf),
                                FORMAT_NATIVE );
        if( rc > 0 )
        {
            simple_resize_bitmap( &temp_bitmap, &main_bitmap );
            puzzle_bmp_ptr = (const fb_data *)img_buf;
            rb->strcpy( img_buf_path, filename );
            return true;
        }
#else
        rc = rb->read_bmp_file( filename, &main_bitmap, sizeof(img_buf),
                                FORMAT_NATIVE );
        if( rc > 0 )
        {
            puzzle_bmp_ptr = (const fb_data *)img_buf;
            rb->strcpy( img_buf_path, filename );
            return true;
        }
#endif
    }

    /* something must have failed. get_albumart_bmp_path could return
       NULL if albumart doesn't exist or couldn't be loaded, or
       read_bmp_file could have failed.  return false and caller should
       try the next mode (PICMODE_DEFAULT_PICTURE and PICMODE_NUMERALS will
       always succeed) */
    return false;
}

/* draws a spot at the coordinates (x,y), range of p is 1-20 */
static void draw_spot(int p, int x, int y)
{
    if (p == HOLE_ID)
    {
#if LCD_DEPTH==1
        /* the bottom-right cell of the default sliding_puzzle image is
           an appropriate hole graphic */
        rb->lcd_bitmap_part(sliding_puzzle, ((p-1)%SPOTS_X)*SPOTS_WIDTH,
                                 ((p-1)/SPOTS_X)*SPOTS_HEIGHT,
                                IMAGE_WIDTH, x, y, SPOTS_WIDTH, SPOTS_HEIGHT);
#else
        /* just draw a black rectangle */
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_set_background(LCD_BLACK);
        rb->lcd_fillrect(x,y,SPOTS_WIDTH,SPOTS_HEIGHT);
        rb->lcd_set_drawmode(DRMODE_SOLID);
#endif
    }
    else if (picmode != PICMODE_NUMERALS)
    {
        rb->lcd_bitmap_part( puzzle_bmp_ptr, ((p-1)%SPOTS_X)*SPOTS_WIDTH,
                             ((p-1)/SPOTS_X)*SPOTS_HEIGHT,
                             IMAGE_WIDTH, x, y, SPOTS_WIDTH, SPOTS_HEIGHT);
    } else {
        rb->lcd_drawrect(x, y, SPOTS_WIDTH, SPOTS_HEIGHT);
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(x+1, y+1, SPOTS_WIDTH-2, SPOTS_HEIGHT-2);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->snprintf(s, sizeof(s), "%d", p);
        rb->lcd_putsxy(x+2, y+4, (unsigned char *)s);
    }
}

/* check if the puzzle is solved */
static bool puzzle_finished(void)
{
    int i;
    for (i=0; i<NUM_SPOTS; i++)
    if (spots[i] != (i+1))
        return false;
    return true;
}

/* move a piece in any direction */
static void move_spot(int x, int y)
{
    int i;
    spots[hole] = spots[hole-x-SPOTS_X*y];
    hole -= (x+SPOTS_X*y);
    moves++;
    rb->snprintf(s, sizeof(s), "%d", moves);
    s[sizeof(s)-1] = '\0';
#if (LCD_WIDTH>IMAGE_SIZE)
    rb->lcd_putsxy(IMAGE_WIDTH+5, 20, (unsigned char *)s);
#else
    rb->lcd_putsxy(5, IMAGE_HEIGHT+20, (unsigned char *)s);
#endif

    for(i=1;i<=4;i++)
    {
       draw_spot(HOLE_ID,
                 (hole%SPOTS_X)*SPOTS_WIDTH,
                 (hole/SPOTS_X)*SPOTS_HEIGHT);
       draw_spot(spots[hole],
                 (hole%SPOTS_X)*SPOTS_WIDTH + (i*x*SPOTS_WIDTH)/5,
                 (hole/SPOTS_X)*SPOTS_HEIGHT + (i*y*SPOTS_HEIGHT)/5);
       rb->lcd_update();
       rb->sleep(HZ/50);
    }
    draw_spot(HOLE_ID,
              (hole%SPOTS_X)*SPOTS_WIDTH,
              (hole/SPOTS_X)*SPOTS_HEIGHT);
    draw_spot(spots[hole],
              ((hole%SPOTS_X)+x)*SPOTS_WIDTH,
              ((hole/SPOTS_X)+y)*SPOTS_HEIGHT);
    rb->lcd_update();

    spots[hole] = HOLE_ID;
}

/* initializes the puzzle */
static void puzzle_init(void)
{
    int i, r, temp, tsp[NUM_SPOTS];

    moves = 0;
    rb->lcd_clear_display();
    rb->snprintf(s, sizeof(s), "%d", moves);

#if (LCD_WIDTH>IMAGE_SIZE)
    rb->lcd_drawrect(IMAGE_WIDTH, 0, 32, 64);
    rb->lcd_putsxy(IMAGE_WIDTH+1, 10, (unsigned char *)"Moves");
    rb->lcd_putsxy(IMAGE_WIDTH+5, 20, (unsigned char *)s);
#else
    rb->lcd_drawrect(0, IMAGE_HEIGHT, 32, 64);
    rb->lcd_putsxy(1, IMAGE_HEIGHT+10, (unsigned char *)"Moves");
    rb->lcd_putsxy(5, IMAGE_HEIGHT+20, (unsigned char *)s);
#endif

    /* shuffle spots */
    for (i=NUM_SPOTS-1; i>=0; i--) {
        r = (rb->rand() % (i+1));

        temp = spots[r];
        spots[r] = spots[i];
        spots[i] = temp;

        if (spots[i]==HOLE_ID)
            hole = i;
    }

    /* test if the puzzle is solvable */
    for (i=0; i<NUM_SPOTS; i++)
        tsp[i] = spots[i];
    r=0;

    /* First, check if the problem has even or odd parity,
       depending on where the empty square is */
    if ((((SPOTS_X-1)-hole%SPOTS_X) + ((SPOTS_Y-1)-hole/SPOTS_X))%2 == 1)
        ++r;

    /* Now check how many swaps we need to solve it */
    for (i=0; i<NUM_SPOTS-1; i++) {
        while (tsp[i] != (i+1)) {
            temp = tsp[i];
            tsp[i] = tsp[temp-1];
            tsp[temp-1] = temp;
            ++r;
        }
    }

    /* if the random puzzle isn't solvable just change two spots */
    if (r%2 == 1) {
        if (spots[0]!=HOLE_ID && spots[1]!=HOLE_ID) {
            temp = spots[0];
            spots[0] = spots[1];
            spots[1] = temp;
        } else {
            temp = spots[2];
            spots[2] = spots[3];
            spots[3] = temp;
        }
    }

    /* draw spots to the lcd */
    for (i=0; i<NUM_SPOTS; i++)
        draw_spot(spots[i], (i%SPOTS_X)*SPOTS_WIDTH, (i/SPOTS_X)*SPOTS_HEIGHT);

    rb->lcd_update();
}

/* the main game loop */
static int puzzle_loop(void)
{
    int button;
    int lastbutton = BUTTON_NONE;
    int i;
    bool load_success;

    puzzle_init();
    while(true) {
        button = rb->button_get(true);
        switch (button) {
#ifdef PUZZLE_RC_QUIT
            case PUZZLE_RC_QUIT:
#endif
            case PUZZLE_QUIT:
                /* get out of here */
                return PLUGIN_OK;

            case PUZZLE_SHUFFLE:
#ifdef PUZZLE_SHUFFLE_PICTURE_PRE
                if (lastbutton != PUZZLE_SHUFFLE_PICTURE_PRE)
                    break;
#endif
                /* mix up the pieces */
                puzzle_init();
                break;

            case PUZZLE_PICTURE:
#ifdef PUZZLE_SHUFFLE_PICTURE_PRE
                if (lastbutton != PUZZLE_SHUFFLE_PICTURE_PRE)
                    break;
#endif
                /* change picture */
                picmode = (picmode+1)%PICMODE_LAST_XXX;

                /* if load_resize_bitmap fails to load bitmap, try next picmode */
                do
                {
                    load_success = load_resize_bitmap();
                    if( !load_success )
                        picmode = (picmode+1)%PICMODE_LAST_XXX;
                }
                while( !load_success );

                /* tell the user what mode we picked in the end! */
                rb->splash(HZ,picmode_descriptions[ picmode ] );
                rb->lcd_clear_display();
#if (LCD_WIDTH>IMAGE_SIZE)
                rb->lcd_drawrect(IMAGE_WIDTH, 0, 32, 64);
                rb->lcd_putsxy(IMAGE_WIDTH+1, 10, (unsigned char *)"Moves");
#else
                rb->lcd_drawrect(0,IMAGE_HEIGHT,32,64);
                rb->lcd_putsxy(1,IMAGE_HEIGHT+10, (unsigned char *)"Moves");
#endif

                for (i=0; i<NUM_SPOTS; i++)
                    draw_spot(spots[i],
                              (i%SPOTS_X)*SPOTS_WIDTH,
                              (i/SPOTS_X)*SPOTS_HEIGHT);
                rb->lcd_update();

                break;

            case BUTTON_LEFT:
                if ((hole%SPOTS_X)<(SPOTS_X-1) && !puzzle_finished())
                    move_spot(-1, 0);
                break;

            case BUTTON_RIGHT:
                if ((hole%SPOTS_X)>0 && !puzzle_finished())
                    move_spot(1, 0);
                break;

            case PUZZLE_UP:
                if ((hole/SPOTS_X)<(SPOTS_Y-1) && !puzzle_finished())
                    move_spot(0, -1);
                break;

            case PUZZLE_DOWN:
                if ((hole/SPOTS_X)>0 && !puzzle_finished())
                    move_spot(0, 1);
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int i, w, h;

    rb = api;

    initial_bmp_path=(const char *)parameter;
    picmode = PICMODE_INITIAL_PICTURE;
    img_buf_path[0] = '\0';

    /* If launched as a viewer, just go straight to the game without
       bothering with the splash or instructions page */
    if(parameter==NULL)
    {
        /* if not launched as a viewer, use default puzzle, and show help */
        picmode = PICMODE_DEFAULT_PICTURE;

        /* print title */
        rb->lcd_getstringsize((unsigned char *)"Sliding Puzzle", &w, &h);
        w = (w+1)/2;
        h = (h+1)/2;
        rb->lcd_clear_display();
        rb->lcd_putsxy(LCD_WIDTH/2-w, (LCD_HEIGHT/2)-h,
                       (unsigned char *)"Sliding Puzzle");
        rb->lcd_update();
        rb->sleep(HZ);

        /* print instructions */
        rb->lcd_clear_display();
        rb->lcd_setfont(FONT_SYSFIXED);
#if CONFIG_KEYPAD == RECORDER_PAD || CONFIG_KEYPAD == ARCHOS_AV300_PAD
        rb->lcd_putsxy(3, 18, "[OFF] to stop");
        rb->lcd_putsxy(3, 28, "[F1] shuffle");
        rb->lcd_putsxy(3, 38, "[F2] change pic");
#elif CONFIG_KEYPAD == ONDIO_PAD
        rb->lcd_putsxy(0, 18, "[OFF] to stop");
        rb->lcd_putsxy(0, 28, "[MODE..] shuffle");
        rb->lcd_putsxy(0, 38, "[MODE] change pic");
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
        rb->lcd_putsxy(0, 18, "[S-MENU] to stop");
        rb->lcd_putsxy(0, 28, "[S-LEFT] shuffle");
        rb->lcd_putsxy(0, 38, "[S-RIGHT] change pic");
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
        rb->lcd_putsxy(0, 18, "[STOP] to stop");
        rb->lcd_putsxy(0, 28, "[SELECT] shuffle");
        rb->lcd_putsxy(0, 38, "[PLAY] change pic");
#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
        rb->lcd_putsxy(0, 18, "[OFF] to stop");
        rb->lcd_putsxy(0, 28, "[REC] shuffle");
        rb->lcd_putsxy(0, 38, "[PLAY] change pic");
#elif CONFIG_KEYPAD == GIGABEAT_PAD
        rb->lcd_putsxy(0, 18, "[OFF] to stop");
        rb->lcd_putsxy(0, 28, "[SELECT] shuffle");
        rb->lcd_putsxy(0, 38, "[A] change pic");
#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD)
        rb->lcd_putsxy(0, 18, "[OFF] to stop");
        rb->lcd_putsxy(0, 28, "[REC] shuffle");
        rb->lcd_putsxy(0, 38, "[SELECT] change pic");
#elif CONFIG_KEYPAD == IRIVER_H10_PAD
        rb->lcd_putsxy(0, 18, "[OFF] to stop");
        rb->lcd_putsxy(0, 28, "[REW] shuffle");
        rb->lcd_putsxy(0, 38, "[PLAY] change pic");
#endif
#ifdef HAVE_ALBUMART
        rb->lcd_putsxy(0,48,"    pic->albumart->num");
#else
        rb->lcd_putsxy(0,48,"    pic<->num");
#endif
        rb->lcd_update();
        rb->button_get_w_tmo(HZ*2);
    }

    hole = INITIAL_HOLE;

    if( !load_resize_bitmap() )
    {
        rb->lcd_clear_display();
        rb->splash(HZ*2,"Failed to load bitmap!");
        return PLUGIN_OK;
    }

#if LCD_DEPTH>1
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_backdrop(NULL);
#endif

    rb->lcd_clear_display();
#if (LCD_WIDTH>IMAGE_SIZE)
    rb->lcd_drawrect(IMAGE_WIDTH, 0, 32, 64);
    rb->lcd_putsxy(IMAGE_WIDTH+1, 10, (unsigned char *)"Moves");
#else
    rb->lcd_drawrect(0,IMAGE_HEIGHT,32,64);
    rb->lcd_putsxy(1,IMAGE_HEIGHT+10, (unsigned char *)"Moves");
#endif

    for (i=0; i<NUM_SPOTS; i++) {
        spots[i]=(i+1);
        draw_spot(spots[i], (i%SPOTS_X)*SPOTS_WIDTH, (i/SPOTS_X)*SPOTS_HEIGHT);
    }

    rb->lcd_update();
    rb->sleep(HZ*2);

    return puzzle_loop();
}

#endif
