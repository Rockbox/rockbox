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

/* The main buffer storing the compressed video data */
#define BUFFER_SIZE (MEM-6)*1024*1024

static mpeg2dec_t * mpeg2dec;
static vo_open_t * output_open = NULL;
static vo_instance_t * output;
static int total_offset = 0;

extern vo_open_t vo_rockbox_open;
/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_STOP       BUTTON_OFF
#define MPEG_PAUSE      BUTTON_ON

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define MPEG_STOP       BUTTON_MENU
#define MPEG_PAUSE      BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define MPEG_STOP       BUTTON_POWER
#define MPEG_PAUSE      BUTTON_PLAY

#else
#error MPEGPLAYER: Unsupported keypad
#endif
static bool button_loop(void)
{
    int button = rb->button_get(false);
    switch (button)
    {
        case MPEG_STOP:
            return true;
        case MPEG_PAUSE:
            button = BUTTON_NONE;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            do {
                button = rb->button_get(true);
            } while (button != MPEG_PAUSE);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            break;
        default:
            if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                return true;
    }
    return false;
}

static bool decode_mpeg2 (uint8_t * current, uint8_t * end)
{
    const mpeg2_info_t * info;
    mpeg2_state_t state;
    vo_setup_result_t setup_result;

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
            /* might set nb fbuf, convert format, stride */
            /* might set fbufs */
            if (output->setup (output, info->sequence->width,
                               info->sequence->height,
                               info->sequence->chroma_width,
                               info->sequence->chroma_height, &setup_result)) {
                //fprintf (stderr, "display setup failed\n");
                return false;
            }
            if (setup_result.convert &&
                mpeg2_convert (mpeg2dec, setup_result.convert, NULL)) {
                //fprintf (stderr, "color conversion setup failed\n");
                return false;
            }
            if (output->set_fbuf) {
                uint8_t * buf[3];
                void * id;

                mpeg2_custom_fbuf (mpeg2dec, 1);
                output->set_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
                output->set_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
            } else if (output->setup_fbuf) {
                uint8_t * buf[3];
                void * id;

                output->setup_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
                output->setup_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
                output->setup_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
            }
            mpeg2_skip (mpeg2dec, (output->draw == NULL));
            break;
        case STATE_PICTURE:
            /* might skip */
            /* might set fbuf */
            if (output->set_fbuf) {
                uint8_t * buf[3];
                void * id;

                output->set_fbuf (output, buf, &id);
                mpeg2_set_buf (mpeg2dec, buf, id);
            }
            if (output->start_fbuf)
                output->start_fbuf (output, info->current_fbuf->buf,
                                    info->current_fbuf->id);
            break;
        case STATE_SLICE:
        case STATE_END:
        case STATE_INVALID_END:
            /* draw current picture */
            /* might free frame buffer */
            if (info->display_fbuf) {
                if (output->draw)
                    output->draw (output, info->display_fbuf->buf,
                                  info->display_fbuf->id);
                //print_fps (0);
            }
            if (output->discard && info->discard_fbuf)
                output->discard (output, info->discard_fbuf->buf,
                                 info->discard_fbuf->id);
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

    do {
        rb->splash(0,true,"Buffering...");
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

    output_open = vo_rockbox_open;

    if (output_open == NULL) {
        //fprintf (stderr, "output_open is NULL\n");
        return PLUGIN_ERROR;
    }

    output = output_open ();

    if (output == NULL) {
        //fprintf (stderr, "Can not open output\n");
        return PLUGIN_ERROR;
    }

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

#ifdef CONFIG_BACKLIGHT
    /* reset backlight settings */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
#endif

    return PLUGIN_OK;
}
