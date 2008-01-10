/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Matthias Wientapper
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "pluginlib_actions.h"
#include "metronome.h"

PLUGIN_HEADER
#define METRONOME_QUIT          PLA_QUIT

/* for volume changes, PLA with scrollwheel isn't proper */

#ifdef HAVE_SCROLLWHEEL
#define METRONOME_VOL_UP        PLA_DOWN
#define METRONOME_VOL_DOWN      PLA_UP
#define METRONOME_VOL_UP_REP    PLA_DOWN_REPEAT
#define METRONOME_VOL_DOWN_REP  PLA_UP_REPEAT
#else
#define METRONOME_VOL_UP        PLA_UP
#define METRONOME_VOL_DOWN      PLA_DOWN
#define METRONOME_VOL_UP_REP    PLA_UP_REPEAT
#define METRONOME_VOL_DOWN_REP  PLA_DOWN_REPEAT
#endif
#define METRONOME_LEFT          PLA_LEFT
#define METRONOME_RIGHT         PLA_RIGHT
#define METRONOME_LEFT_REP      PLA_LEFT_REPEAT
#define METRONOME_RIGHT_REP     PLA_RIGHT_REPEAT
enum {
    METRONOME_PLAY_TAP = LAST_PLUGINLIB_ACTION+1,
#if CONFIG_KEYPAD == ONDIO_PAD
    METRONOME_PAUSE,
#endif /* ONDIO_PAD  */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    METRONOME_SYNC 
#endif /* IRIVER_H100_PAD||IRIVER_H300_PAD */
};


#if CONFIG_KEYPAD == ONDIO_PAD
#define METRONOME_TAP       PLA_START
#define METRONOME_MSG_START "start: mode"
#define METRONOME_MSG_STOP "pause: hold mode"
static const struct button_mapping ondio_action[] = 
{
    {METRONOME_PLAY_TAP, BUTTON_MENU|BUTTON_REL, BUTTON_MENU },
    {METRONOME_PAUSE, BUTTON_MENU|BUTTON_REPEAT, BUTTON_NONE },
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};
#else /* !ONDIO_PAD  */
#define METRONOME_TAP       PLA_FIRE
#define METRONOME_PLAYPAUSE PLA_START
#define METRONOME_MSG_START "press play"
#define METRONOME_MSG_STOP "press pause"

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MET_SYNC
static const struct button_mapping iriver_syncaction[] =
{
    {METRONOME_SYNC, BUTTON_REC, BUTTON_NONE },
    {CONTEXT_CUSTOM,BUTTON_NONE,BUTTON_NONE}
};
#endif /* IRIVER_H100_PAD||IRIVER_H300_PAD */
#endif /* #if CONFIG_KEYPAD == ONDIO_PAD */

const struct button_mapping *plugin_contexts[]={
    generic_directions,
#if CONFIG_KEYPAD == ONDIO_PAD
    ondio_action,
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    iriver_syncaction,
#endif
    generic_actions
};

static struct plugin_api* rb;

MEM_FUNCTION_WRAPPERS(rb);

static int bpm      = 120;
static int period   = 0;
static int minitick = 0;

static bool sound_active = false;
static bool sound_paused = true;

static char buffer[30];

static bool reset_tap = false;
static int tap_count    = 0;
static int tap_time     = 0;
static int tap_timeout  = 0;

int bpm_step_counter = 0;

#if CONFIG_CODEC != SWCODEC

#define MET_IS_PLAYING rb->mp3_is_playing()
#define MET_PLAY_STOP rb->mp3_play_stop()

void callback(unsigned char** start, size_t* size){
    (void)start; /* unused parameter, avoid warning */
    *size = NULL; /* end of data */
    sound_active = false;
    rb->led(0);
}

void play_tock(void){
    sound_active = true;
    rb->led(1);
    rb->mp3_play_data(sound, sizeof(sound), callback);
    rb->mp3_play_pause(true); /* kickoff audio */ 
}

#else /*  CONFIG_CODEC == SWCODEC */

#define MET_IS_PLAYING rb->pcm_is_playing()
#define MET_PLAY_STOP rb->audio_stop()


int tock;
bool need_to_play = false;

short sndbuf[sizeof(sound)*2];

/* Convert the mono "tock" sample to interleaved stereo */
void prepare_tock(void)
{
    int i;
    for(i = 0;i < (int)sizeof(sound)/2;i++) {
        sndbuf[i*2] = sound[i];
        sndbuf[i*2+1] = sound[i];
    }
}

void play_tock(void) {
    rb->pcm_play_data(NULL,(unsigned char *)sndbuf,sizeof(sndbuf));
    tock++;
}

#endif /* CONFIG_CODEC != SWCODEC */

void calc_period(void)
{
    period =  61440/bpm-1; /* (60*1024)/bpm; */
}


void metronome_draw(struct screen* display)
{
    display->clear_display();

#ifdef HAVE_LCD_BITMAP 
    display->setfont(FONT_SYSFIXED);
    display->puts(0, 0, "Metronome");
    if(display->screen_type==SCREEN_MAIN)
    {
        display->puts(0, 5, "Select to TAP");
        display->puts(0, 6, "Rec to SYNC");
    }
#ifdef HAVE_REMOTE_LCD
    else
    {
        display->puts(0, 5, "Rec to TAP");
        display->puts(0, 6, "Mode to SYNC");
    }
#endif
#endif /* HAVE_LCD_BITMAP */

    rb->snprintf(buffer, sizeof(buffer), "BPM: %d ",bpm);
#ifdef HAVE_LCD_BITMAP
    display->puts(0,3, buffer);
#else
    display->puts(0,0, buffer);
#endif /* HAVE_LCD_BITMAP */

    rb->snprintf(buffer, sizeof(buffer), "Vol: %d",
                 rb->global_settings->volume);
#ifdef HAVE_LCD_BITMAP
    display->puts(10, 3, buffer);
#else
    display->puts(0,1, buffer);
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_BITMAP
    display->drawline(0, 12, 111, 12);
    if(sound_paused)
        display->puts(0,2,METRONOME_MSG_START);
    else
        display->puts(0,2,METRONOME_MSG_STOP);
#endif /* HAVE_LCD_BITMAP */
    display->update();
}

void draw_display(void)
{
    int i;
    FOR_NB_SCREENS(i)
        metronome_draw(rb->screens[i]);
}

/* helper function to change the volume by a certain amount, +/-
   ripped from video.c */
void change_volume(int delta){
    int minvol = rb->sound_min(SOUND_VOLUME);
    int maxvol = rb->sound_max(SOUND_VOLUME);
    int vol = rb->global_settings->volume + delta;

    if (vol > maxvol) vol = maxvol;
    else if (vol < minvol) vol = minvol;
    if (vol != rb->global_settings->volume) {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
        draw_display();
    }
}

/*function to accelerate bpm change*/
void change_bpm(int direction){
        if((bpm_step_counter < 20)
            || (bpm > 389)
            || (bpm < 10))
        bpm = bpm + direction;
    else if (bpm_step_counter < 60)
        bpm = bpm + direction * 2;
    else
        bpm = bpm + direction * 9; 

    if (bpm > 400) bpm = 400;
    if (bpm < 1) bpm = 1;
    calc_period();
    draw_display();
    bpm_step_counter++;
}

void timer_callback(void)
{
    if(minitick >= period){
        minitick = 0;
        if(!sound_active && !sound_paused && !tap_count) {
#if CONFIG_CODEC == SWCODEC
            /* On SWCODEC we can't call play_tock() directly from an ISR. */
            need_to_play = true;
#else
            play_tock();
#endif
            rb->reset_poweroff_timer();
        }
    }
    else {
        minitick++;
    }

    if (tap_count) {
        tap_time++;
        if (tap_count > 1 && tap_time > tap_timeout)
            tap_count = 0;
    }
}

void cleanup(void *parameter)
{
    (void)parameter;

    rb->timer_unregister();
    MET_PLAY_STOP; /* stop audio ISR */
    rb->led(0);
}

void tap(void)
{
    if (tap_count == 0 || tap_time < tap_count) {
        tap_time = 0;
    } 
    else {
        if (tap_time > 0) {
            bpm = 61440/(tap_time/tap_count);

            if (bpm > 400)
                bpm = 400;
        }

        calc_period();
        draw_display();

        tap_timeout = (tap_count+2)*tap_time/tap_count;
    }

    tap_count++;
    minitick = 0;  /* sync tock to tapping */
    play_tock();

    reset_tap = false;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter){
    int button;
#if (CONFIG_KEYPAD == ONDIO_PAD) \
    || (CONFIG_KEYPAD == IRIVER_H100_PAD) \
    || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PLA_ARRAY_COUNT 3
#else
#define PLA_ARRAY_COUNT 2
#endif
    enum plugin_status status;

    (void)parameter;
    rb = api;

    if (MET_IS_PLAYING)
        MET_PLAY_STOP; /* stop audio IS */

#if CONFIG_CODEC != SWCODEC
    rb->bitswap(sound, sizeof(sound));
#else
    prepare_tock();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->pcm_set_frequency(SAMPR_44);
#endif /* CONFIG_CODEC != SWCODEC */

    calc_period();
    rb->timer_register(1, NULL, TIMER_FREQ/1024, 1, timer_callback);

    draw_display();

    /* main loop */
    while (true){
        reset_tap = true;
#if CONFIG_CODEC == SWCODEC
        button = pluginlib_getaction(rb,1,plugin_contexts,PLA_ARRAY_COUNT);
        if (need_to_play)
        {
            need_to_play = false;
            play_tock();
        }
#else
        button = pluginlib_getaction(rb,TIMEOUT_BLOCK,
                                     plugin_contexts,PLA_ARRAY_COUNT);
#endif /* SWCODEC */
        switch (button) {

            case METRONOME_QUIT:
                /* get out of here */
                cleanup(NULL);
                status = PLUGIN_OK;
                goto metronome_exit;

#if CONFIG_KEYPAD == ONDIO_PAD
            case METRONOME_PLAY_TAP:
                if(sound_paused) {
                    sound_paused = false;
                    calc_period();
                    draw_display();
                }
                else
                    tap();
                break;

            case METRONOME_PAUSE:
                if(!sound_paused) {
                    sound_paused = true;
                    draw_display();
                }
                break;

#else
            case METRONOME_PLAYPAUSE:
                if(sound_paused)
                    sound_paused = false;
                else
                    sound_paused = true;
                calc_period();
                draw_display();
                break;
#endif /* ONDIO_PAD */

            case METRONOME_VOL_UP:
            case METRONOME_VOL_UP_REP:
                change_volume(1);
                calc_period();
                break;

            case METRONOME_VOL_DOWN:
            case METRONOME_VOL_DOWN_REP:
                change_volume(-1);
                calc_period();
                break;

            case METRONOME_LEFT:
                bpm_step_counter = 0;
            case METRONOME_LEFT_REP:
                change_bpm(-1);
                break;

            case METRONOME_RIGHT:
                bpm_step_counter = 0;
            case METRONOME_RIGHT_REP:
                change_bpm(1);
                break;

#ifdef  METRONOME_TAP
            case METRONOME_TAP:
                tap();
                break;
#endif

#ifdef MET_SYNC
            case METRONOME_SYNC:
                minitick = period;
                break;
#endif

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                {
                    status = PLUGIN_USB_CONNECTED;
                    goto metronome_exit;
                }
                reset_tap = false;
                break;

        }
        if (reset_tap) {
            tap_count = 0;
        }
    }

metronome_exit:
#if CONFIG_CODEC == SWCODEC
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
#endif
    return status;
}
