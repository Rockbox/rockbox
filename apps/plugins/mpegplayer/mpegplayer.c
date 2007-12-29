/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mpegplayer main entrypoint and UI implementation
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/****************************************************************************
 * NOTES:
 * 
 * mpegplayer is structured as follows:
 * 
 *                       +-->Video Thread-->Video Output-->LCD
 *                       |
 * UI-->Stream Manager-->+-->Audio Thread-->PCM buffer--Audio Device
 *         |       |     |                        |     (ref. clock)
 *         |       |     +-->Buffer Thread        |
 *    Stream Data  |             |          (clock intf./
 *     Requests    |         File Cache      drift adj.)
 *                 |          Disk I/O
 *         Stream services
 *          (timing, etc.)
 * 
 * Thread list:
 *  1) The main thread - Handles user input, settings, basic playback control
 *     and USB connect.
 * 
 *  2) Stream Manager thread - Handles playback state, events from streams
 *     such as when a stream is finished, stream commands, PCM state. The
 *     layer in which this thread run also handles arbitration of data
 *     requests between the streams and the disk buffer. The actual specific
 *     transport layer code may get moved out to support multiple container
 *     formats.
 * 
 *  3) Buffer thread - Buffers data in the background, generates notifications
 *     to streams when their data has been buffered, and watches streams'
 *     progress to keep data available during playback. Handles synchronous
 *     random access requests when the file cache is missed.
 * 
 *  4) Video thread (running on the COP for PortalPlayer targets) - Decodes
 *     the video stream and renders video frames to the LCD. Handles
 *     miscellaneous video tasks like frame and thumbnail printing.
 * 
 *  5) Audio thread (running on the main CPU to maintain consistency with the
 *     audio FIQ hander on PP) - Decodes audio frames and places them into
 *     the PCM buffer for rendering by the audio device.
 * 
 * Streams are neither aware of one another nor care about one another. All
 * streams shall have their own thread (unless it is _really_ efficient to
 * have a single thread handle a couple minor streams). All coordination of
 * the streams is done through the stream manager. The clocking is controlled
 * by and exposed by the stream manager to other streams and implemented at
 * the PCM level.
 * 
 * Notes about MPEG files:
 * 
 * MPEG System Clock is 27MHz - i.e. 27000000 ticks/second.
 * 
 * FPS is represented in terms of a frame period - this is always an
 * integer number of 27MHz ticks.
 * 
 * e.g. 29.97fps (30000/1001) NTSC video has an exact frame period of
 * 900900 27MHz ticks.
 * 
 * In libmpeg2, info->sequence->frame_period contains the frame_period.
 * 
 * Working with Rockbox's 100Hz tick, the common frame rates would need
 * to be as follows (1):
 * 
 * FPS     | 27Mhz   | 100Hz          | 44.1KHz   | 48KHz
 * --------|-----------------------------------------------------------
 * 10*     | 2700000 | 10             | 4410      | 4800
 * 12*     | 2250000 |  8.3333        | 3675      | 4000
 * 15*     | 1800000 |  6.6667        | 2940      | 3200
 * 23.9760 | 1126125 |  4.170833333   | 1839.3375 | 2002
 * 24      | 1125000 |  4.166667      | 1837.5    | 2000
 * 25      | 1080000 |  4             | 1764      | 1920
 * 29.9700 |  900900 |  3.336667      | 1471,47   | 1601.6
 * 30      |  900000 |  3.333333      | 1470      | 1600
 * 
 * *Unofficial framerates
 * 
 * (1) But we don't really care since the audio clock is used anyway and has
 *     very fine resolution ;-)
 *****************************************************************************/
#include "plugin.h"
#include "mpegplayer.h"
#include "helper.h"
#include "mpeg_settings.h"
#include "mpeg2.h"
#include "video_out.h"
#include "stream_thread.h"
#include "stream_mgr.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_MENU       BUTTON_MODE
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_MENU       BUTTON_MENU
#define MPEG_PAUSE      (BUTTON_PLAY | BUTTON_REL)
#define MPEG_STOP       (BUTTON_PLAY | BUTTON_REPEAT)
#define MPEG_VOLDOWN    BUTTON_SCROLL_BACK
#define MPEG_VOLUP      BUTTON_SCROLL_FWD

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define MPEG_MENU       (BUTTON_REC | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_DOWN
#define MPEG_VOLUP      BUTTON_UP

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_MENU       BUTTON_MENU
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_SELECT
#define MPEG_PAUSE2     BUTTON_A
#define MPEG_VOLDOWN    BUTTON_LEFT
#define MPEG_VOLUP      BUTTON_RIGHT
#define MPEG_VOLDOWN2   BUTTON_VOL_DOWN
#define MPEG_VOLUP2     BUTTON_VOL_UP

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_MENU       (BUTTON_REW | BUTTON_REL)
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY
#define MPEG_VOLDOWN    BUTTON_SCROLL_DOWN
#define MPEG_VOLUP      BUTTON_SCROLL_UP

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_SCROLL_UP
#define MPEG_VOLUP      BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define MPEG_MENU       BUTTON_SELECT
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_UP
#define MPEG_VOLDOWN    BUTTON_VOL_DOWN
#define MPEG_VOLUP      BUTTON_VOL_UP

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_MENU       BUTTON_RC_HEART
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_TOUCHPAD
#define MPEG_VOLDOWN    BUTTON_RC_VOL_DOWN
#define MPEG_VOLUP      BUTTON_RC_VOL_UP

#else
#error MPEGPLAYER: Unsupported keypad
#endif

struct plugin_api* rb;

CACHE_FUNCTION_WRAPPERS(rb);
ALIGN_BUFFER_WRAPPER(rb);

static bool button_loop(void)
{
    bool ret = true;

    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();
    rb->lcd_update();

    /* Start playback at the specified starting time */
    if (stream_seek(settings.resume_time, SEEK_SET) < STREAM_OK ||
        (stream_show_vo(true), stream_play()) < STREAM_OK)
    {
        rb->splash(HZ*2, "Playback failed");
        return false;
    }

    /* Gently poll the video player for EOS and handle UI */
    while (stream_status() != STREAM_STOPPED)
    {
        int button = rb->button_get_w_tmo(HZ/2);

        switch (button)
        {
        case BUTTON_NONE:
            continue;

        case MPEG_VOLUP:
        case MPEG_VOLUP|BUTTON_REPEAT:
#ifdef MPEG_VOLUP2
        case MPEG_VOLUP2:
        case MPEG_VOLUP2|BUTTON_REPEAT:
#endif
        {
            int vol = rb->global_settings->volume;
            int maxvol = rb->sound_max(SOUND_VOLUME);

            if (vol < maxvol) {
                vol++;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;
            } /* MPEG_VOLUP*: */

        case MPEG_VOLDOWN:
        case MPEG_VOLDOWN|BUTTON_REPEAT:
#ifdef MPEG_VOLDOWN2
        case MPEG_VOLDOWN2:
        case MPEG_VOLDOWN2|BUTTON_REPEAT:
#endif
        {
            int vol = rb->global_settings->volume;
            int minvol = rb->sound_min(SOUND_VOLUME);

            if (vol > minvol) {
                vol--;
                rb->sound_set(SOUND_VOLUME, vol);
                rb->global_settings->volume = vol;
            }
            break;
            } /* MPEG_VOLDOWN*: */

        case MPEG_MENU:
        {
            int state = stream_pause(); /* save previous state */
            int result;

            /* Hide video output */
            stream_show_vo(false);
            backlight_use_settings(rb);

            result = mpeg_menu();

            /* The menu can change the font, so restore */
            rb->lcd_setfont(FONT_SYSFIXED);

            switch (result)
            {
            case MPEG_MENU_QUIT:
                stream_stop();
                break;
            default:
                /* If not stopped, show video again */
                if (state != STREAM_STOPPED)
                    stream_show_vo(true);

                /* If stream was playing, restart it */
                if (state == STREAM_PLAYING) {
                    backlight_force_on(rb);
                    stream_resume();
                }
                break;
            }
            break;
            } /* MPEG_MENU: */

        case MPEG_STOP:
        {
            stream_stop();
            break;
            } /* MPEG_STOP: */

        case MPEG_PAUSE:
#ifdef MPEG_PAUSE2
        case MPEG_PAUSE2:
#endif
        {
            if (stream_status() == STREAM_PLAYING) {
                /* Playing => Paused */
                stream_pause();
                backlight_use_settings(rb);
            }
            else if (stream_status() == STREAM_PAUSED) {
                /* Paused => Playing */
                backlight_force_on(rb);
                stream_resume();
            }

            break;
            } /* MPEG_PAUSE*: */

        case SYS_POWEROFF:
        case SYS_USB_CONNECTED:
            /* Stop and get the resume time before closing the file early */
            stream_stop();
            settings.resume_time = stream_get_resume_time();
            stream_close();
            ret = false;
        /* Fall-through */
        default:
            rb->default_event_handler(button);
            break;
        }

        rb->yield();
    } /* end while */

    rb->lcd_setfont(FONT_UI);

    return ret;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int status = PLUGIN_ERROR; /* assume failure */
    int result;
    int err;
    const char *errstring;

    if (parameter == NULL) {
        /* No file = GTFO */
        api->splash(HZ*2, "No File");
        return PLUGIN_ERROR;
    }

    /* Disable all talking before initializing IRAM */
    api->talk_disable(true);

    /* Initialize IRAM - stops audio and voice as well */
    PLUGIN_IRAM_INIT(api)

    rb = api;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

    rb->lcd_clear_display();
    rb->lcd_update();

    if (stream_init() < STREAM_OK) {
        DEBUGF("Could not initialize streams\n");
    } else {
        rb->splash(0, "Loading...");

        init_settings((char*)parameter);

        err = stream_open((char *)parameter);

        if (err >= STREAM_OK) {
            /* start menu */
            rb->lcd_clear_display();
            rb->lcd_update();
            result = mpeg_start_menu(stream_get_duration());

            if (result != MPEG_START_QUIT) {
                /* Turn off backlight timeout */
                /* backlight control in lib/helper.c */
                backlight_force_on(rb);

                /* Enter button loop and process UI */
                if (button_loop()) {
                     settings.resume_time = stream_get_resume_time();
                }

                /* Turn on backlight timeout (revert to settings) */
                backlight_use_settings(rb);
            }

            stream_close();

            rb->lcd_clear_display();
            rb->lcd_update();

            save_settings();  /* Save settings (if they have changed) */
            status = PLUGIN_OK;
        } else {
            DEBUGF("Could not open %s\n", (char*)parameter);
            switch (err)
            {
            case STREAM_UNSUPPORTED:
                errstring = "Unsupported format";
                break;
            default:
                errstring = "Error opening file: %d";
            }

            rb->splash(HZ*2, errstring, err);
        }
    }

    stream_exit();

    rb->talk_disable(false);
    return status;
}
