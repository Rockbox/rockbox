 /*
 * mpegplayer.c - based on  mpeg2dec.c
 *
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mpeg2dec_config.h"

#include "plugin.h"

#include "mpeg2.h"
#include "mpeg_settings.h"
#include "video_out.h"

PLUGIN_HEADER

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

struct plugin_api* rb;

static mpeg2dec_t * mpeg2dec;
static int total_offset = 0;

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_MENU       BUTTON_MODE
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define MPEG_MENU       BUTTON_MENU
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define MPEG_MENU       (BUTTON_REC | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_A
#define MPEG_PAUSE      BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_MENU       (BUTTON_REW | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY

#else
#error MPEGPLAYER: Unsupported keypad
#endif

static int tick_enabled = 0;

#define MPEG_CURRENT_TICK ((unsigned int)((*rb->current_tick - tick_offset)))

/* The value to subtract from current_tick to get the current mpeg tick */
static int tick_offset;

/* The last tick - i.e. the time to reset the tick_offset to when unpausing */
static int last_tick;

void start_timer(void)
{
    last_tick = 0;
    tick_offset = *rb->current_tick;
}

void unpause_timer(void)
{
    tick_offset = *rb->current_tick - last_tick;
}

void pause_timer(void)
{
    /* Save the current MPEG tick */
    last_tick = *rb->current_tick - tick_offset;
}


static bool button_loop(void)
{
    bool result;
    int button = rb->button_get(false);

    switch (button)
    {
        case MPEG_MENU:
            pause_timer(); 

            result = mpeg_menu();

            unpause_timer();

            return result;

        case MPEG_STOP:
            return true;

        case MPEG_PAUSE:
            pause_timer(); /* Freeze time */
            button = BUTTON_NONE;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            do {
                button = rb->button_get(true);
                if (button == MPEG_STOP)
                    return true;
            } while (button != MPEG_PAUSE);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            unpause_timer(); /* Resume time */
            break;

        default:
            if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                return true;
    }
    return false;
}

/*

NOTES:

MPEG System Clock is 27MHz - i.e. 27000000 ticks/second.

FPS is represented in terms of a frame period - this is always an
integer number of 27MHz ticks.

e.g. 29.97fps (30000/1001) NTSC video has an exact frame period of
900900 27MHz ticks.

In libmpeg2, info->sequence->frame_period contains the frame_period.

Working with Rockbox's 100Hz tick, the common frame rates would need
to be as follows:

FPS     | 27Mhz   | 100Hz
--------|----------------
10*     | 2700000 | 10
12*     | 2250000 |  8.3333
15*     | 1800000 |  6.6667
23.9760 | 1126125 |  4.170833333
24      | 1125000 |  4.166667
25      | 1080000 |  4
29.9700 |  900900 |  3.336667
30      |  900000 |  3.333333


*Unofficial framerates

*/

static uint64_t eta;

static bool decode_mpeg2 (uint8_t * current, uint8_t * end)
{
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    char str[80];
    static int skipped = 0;
    static int frame = 0;
    static int starttick = 0;
    static int lasttick;
    unsigned int eta2;
    unsigned int x;

    int fps;

    if (starttick == 0) {
        starttick=*rb->current_tick-1; /* Avoid divby0 */
        lasttick=starttick;
    }

    mpeg2_buffer (mpeg2dec, current, end);
    total_offset += end - current;

    info = mpeg2_info (mpeg2dec);
    while (1) {
        if (button_loop())
            return true;
        state = mpeg2_parse (mpeg2dec);

        switch (state) {
        case STATE_BUFFER:
            return false;
        case STATE_SEQUENCE:
            vo_setup(info->sequence->width,
                     info->sequence->height,
                     info->sequence->chroma_width,
                     info->sequence->chroma_height);
            mpeg2_skip (mpeg2dec, false);

            break;
        case STATE_PICTURE:
            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            /* draw current picture */
            if (info->display_fbuf) {
                /* We start the timer when we draw the first frame */
                if (!tick_enabled) {
                    start_timer();
                    tick_enabled = 1 ;
                }

                eta += (info->sequence->frame_period);
                eta2 = eta / (27000000 / HZ);

                if (settings.limitfps) {
                    if (eta2 > MPEG_CURRENT_TICK) {
                        rb->sleep(eta2-MPEG_CURRENT_TICK);
                    }              
                }
                x = MPEG_CURRENT_TICK;

                /* If we are more than 1/20 second behind schedule (and 
                   more than 1/20 second into the decoding), skip frame */
                if (settings.skipframes && (x > HZ/20) && 
                   (eta2 < (x - (HZ/20)))) {
                    skipped++;
                } else {
                    vo_draw_frame(info->display_fbuf->buf);
                }

                /* Calculate fps */
                frame++;
                if (settings.showfps && (*rb->current_tick-lasttick>=HZ)) {
                    fps=(frame*(HZ*10))/x;
                    rb->snprintf(str,sizeof(str),"%d.%d %d %d %d",
                                 (fps/10),fps%10,skipped,x,eta2);
                    rb->lcd_putsxy(0,0,str);
                    rb->lcd_update_rect(0,0,LCD_WIDTH,8);
        
                    lasttick = *rb->current_tick;
                }
            }
            break;
        default:
            break;
        }

        rb->yield();
    }
}

static void es_loop (int in_file, uint8_t* buffer, size_t buffer_size)
{
    uint8_t * end;

    if (buffer==NULL)
        return;

    eta = 0;
    do {
        rb->splash(0,true,"Buffering...");
        save_settings();  /* Save settings (if they have changed) */
        end = buffer + rb->read (in_file, buffer, buffer_size);
        if (decode_mpeg2 (buffer, end))
            break;
    } while (end == buffer + buffer_size);
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;
    void* audiobuf;
    int audiosize;
    int in_file;
    uint8_t* buffer;
    size_t buffer_size;

    rb = api;

    /* This also stops audio playback - so we do it before using IRAM */
    audiobuf = rb->plugin_get_audio_buffer(&audiosize);

    /* Initialise our malloc buffer */
    mpeg2_alloc_init(audiobuf,audiosize);

    /* Grab most of the buffer for the compressed video - leave 2MB */
    buffer_size = audiosize - 2*1024*1024;
    buffer = mpeg2_malloc(buffer_size,0);

    if (buffer == NULL)
        return PLUGIN_ERROR;

#ifdef USE_IRAM
    rb->memcpy(iramstart, iramcopy, iramend-iramstart);
    rb->memset(iedata, 0, iend - iedata);
#endif

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    rb->lcd_update();

    if (parameter == NULL) {
        return PLUGIN_ERROR;
    }

    in_file = rb->open((char*)parameter,O_RDONLY);

    if (in_file < 0) {
        //fprintf(stderr,"Could not open %s\n",argv[1]);
        return PLUGIN_ERROR;
    }

    init_settings();

    mpeg2dec = mpeg2_init ();

    if (mpeg2dec == NULL)
        return PLUGIN_ERROR;

    /* make sure the backlight is always on when viewing video
       (actually it should also set the timeout when plugged in,
       but the function backlight_set_timeout_plugged is not
       available in plugins) */
#ifdef CONFIG_BACKLIGHT
    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    es_loop (in_file, buffer, buffer_size);

    mpeg2_close (mpeg2dec);

    rb->close (in_file);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    save_settings();  /* Save settings (if they have changed) */

#ifdef CONFIG_BACKLIGHT
    /* reset backlight settings */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#endif

    return PLUGIN_OK;
}
