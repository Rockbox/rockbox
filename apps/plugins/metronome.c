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
#include "plugin.h"

#ifndef SIMULATOR 

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define METRONOME_QUIT BUTTON_OFF
#define METRONOME_PLAYPAUSE BUTTON_PLAY
#define METRONOME_VOL_UP BUTTON_UP
#define METRONOME_VOL_DOWN BUTTON_DOWN
#define METRONOME_MSG_START "press play"
#define METRONOME_MSG_STOP "press pause"

#elif CONFIG_KEYPAD == ONDIO_PAD
#define METRONOME_QUIT BUTTON_OFF
#define METRONOME_PLAYPAUSE BUTTON_MENU
#define METRONOME_VOL_UP BUTTON_UP
#define METRONOME_VOL_DOWN BUTTON_DOWN
#define METRONOME_MSG_START "start: menu"
#define METRONOME_MSG_STOP "pause: menu"

#elif CONFIG_KEYPAD == PLAYER_PAD
#define METRONOME_QUIT BUTTON_STOP
#define METRONOME_PLAYPAUSE BUTTON_PLAY
#define METRONOME_VOL_UP (BUTTON_ON | BUTTON_RIGHT)
#define METRONOME_VOL_DOWN (BUTTON_ON | BUTTON_LEFT)

#endif
static struct plugin_api* rb;

static int bpm      = 120;
static int period   = 0;
static int minitick = 0;

static bool sound_active = false;
static bool sound_paused = true;

static char buffer[30];

/*tick sound from a metronome*/
static unsigned char sound[]={
255,251, 80,196,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0, 73,110,102,111,  0,  0,  0, 15,  0,  0,  0,  4,  0,  0,  4, 19,  0, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
128,128,128,128,128,128,128,128,192,192,192,192,192,192,192,192,192,192,192,192,
192,192,192,192,192,192,192,192,192,192,192,192,192,255,255,255,255,255,255,255,
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  0,  0,  0,
 58, 76, 65, 77, 69, 51, 46, 57, 50, 32,  1,137,  0,  0,  0,  0,  0,  0,  0,  0,
  2, 64, 36,  5,191, 65,  0,  0,  0,  0,  0,  4, 19,168,187,153, 93,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,255,251, 80,196,  0,  0, 10, 81, 57, 65,184,120,
128,  1, 95,159,239,191,144, 96,  0,  0,128,  0,  0,  0,  0,  0,  0, 48, 24, 12,
  0,  0, 15,175,185,222, 39,  7,126, 46, 50,191,243,197,255,247, 62, 92,111,252,
115, 15, 27,140,153,159,247,242,124, 44, 92,164,177, 57,175,255,236, 79,164, 98,
 68,205, 69,160, 64, 15,251,127,128,168, 38, 92,138, 17, 67,196, 80,184,223,255,
255,161,117, 33, 55, 83,191,255,193,240, 33, 57,114, 17,  0,  2,  0,  0,  0, 10,
191, 10,  8,196,209, 75, 55,236,177,115,238,223,203, 61,176, 89, 78,101,219,118,
118, 65,154,126,187,239,241,127,247,245, 59, 18,219, 94,105,245,221,161,173, 91,
191, 27,255,247, 18,124, 71,117,139,133,170,221,227,251,135,110,236,255,183,219,
100, 78,184,125,212,131, 65, 95,212,245,145, 67, 77, 10,153,250,132,195,  4,138,
224,  0,  0,  0,158,107, 42, 65,227,185, 90,158, 86,  6,155, 77,255,251, 82,196,
  8,  0, 10,136,185,103,  4, 61, 48, 65,118,173, 44, 60,147, 10,152,107, 49, 74,
 18,208, 91, 23,178,196,220, 25, 64,233,230, 36,170,  0,210,219,131, 40,165, 22,
 86,195, 23,145, 33, 16,138, 99,109,117,245, 71,162, 69, 42,220,164, 91, 72,165,
 26, 69,146,150,199,222, 73,102,148, 29, 19,  5, 65, 86,  6,150,120,180, 26, 88,
 75, 18,191,255,250,171,  4,  8,132,140,  0,  0, 20,186,242,162, 66,100, 62, 86,
206, 86,255, 26,203,201,255,255,141,128,166,196,175,133,  9, 70,188,146,175,133,
 60, 17,217,229, 90, 27, 87, 93, 34, 76,113,185, 84,213, 56, 17,170, 83, 75,180,
203,116,101, 93, 90, 99, 35,174,230, 42,125, 12,128, 79, 93, 81,234, 67, 66,137,
105,157,157, 23, 71, 53, 40,250,204,105, 89,203,107,250, 63,174, 89,133, 81,140,
  0, 52, 17, 16,  0,  0, 18,111, 20, 92, 32,213, 48,179, 50, 26,244,179, 27,230,
154, 34,133,177, 88,255,251, 82,196, 13,  0, 10,125, 65, 85,244, 85,  0,  1,210,
172,231,  7, 31, 32,  0, 37, 11, 68, 69, 13, 37, 48,210, 23, 69, 36,122,143,148,
211,141, 44, 76,135, 72,148,195,135,174, 84,149,167, 30,198,183,255,213,111, 61,
 27,155,191,177,207,246,163,255,230, 30,137,219,255,182,186,185,239, 52,148,171,
 75, 63,167, 44,  0,  2, 81,190, 41,253,248,182,180, 23, 43,127,217,161, 41,191,
 36,199, 40,153,108, 66,224,131,192,217, 12, 67,217,  6,218,240,108,112, 63, 33,
 93, 12,  8, 43, 66, 42, 74, 12,215,135,100,172, 33, 65,172, 29, 98,  4, 83, 34,
 40, 17,111,133,255, 18,145, 60, 59,136,145, 92,106,144, 34,185, 58, 76,154,254,
146,144,115,  4, 84,180,142,178, 38, 79,253, 34,237,105, 31,106, 70, 73, 36,138,
 73,164,181,127,253,182, 82,217, 26,245,117,163,255,219, 50,115,223,245, 92,198,
165,173,  0, 15,249,132, 50, 19,157, 85,121,153,245, 51,255,251, 82,196,  6,131,
 74, 96,134,156, 92, 19,  0,  8,  0,  0, 52,128,  0,  0,  0, 60,226, 73, 57, 26,
249,253,122,211,128, 65, 86, 10,  1,  9,195,128, 36, 72,225,196,146,115, 73, 18,
 75, 65, 77,  9,  5,200, 43, 16,163,129, 77,  9,  5,200,110, 39,127,255,255,255,
255,255,255,255,252, 83, 66,142,140, 21,136, 46, 64,166,133, 28, 12, 21,136, 46,
 64,166,133, 21,  6, 76, 65, 77, 69, 51, 46, 57, 50, 85, 85, 85, 85, 85, 85, 85,
 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
    85, 85, 85};



void led(bool on){
    if(on)
    or_b(0x40, &PBDRL);
    else
    and_b(~0x40, &PBDRL);
}

void calc_period(void){
    period =  61440/bpm-1; // (60*1024)/bpm;
}
    
void callback(unsigned char** start, int* size){
    (void)start; /* unused parameter, avoid warning */
    *size = NULL; /* end of data */
    sound_active = false;
    led(0);
}

void play_tock(void){
    sound_active = true;
    led(1);
    rb->mp3_play_data(sound, sizeof(sound), callback);
    rb->mp3_play_pause(true); /* kickoff audio */ 
}


void draw_display(void){
    rb->lcd_clear_display();

#ifdef HAVE_LCD_BITMAP 
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_putsxy(1, 1, "Metronome");
#endif

    rb->snprintf(buffer, sizeof(buffer), "BPM: %d ",bpm);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_puts(0,7, buffer);
#else
    rb->lcd_puts(0,0, buffer);
#endif
    rb->snprintf(buffer, sizeof(buffer), "Vol: %d",
                 rb->global_settings->volume);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_puts(10, 7, buffer);
#else
    rb->lcd_puts(0,1, buffer);
#endif

#ifdef HAVE_LCD_BITMAP
    rb->lcd_drawline(0, 12, 111, 12);
    if(sound_paused)
    rb->lcd_puts(0,2,METRONOME_MSG_START);
    else
    rb->lcd_puts(0,2,METRONOME_MSG_STOP);
    rb->lcd_update();
#endif
}

/* helper function to change the volume by a certain amount, +/-
   ripped from video.c */
void change_volume(int delta){
    int vol = rb->global_settings->volume + delta;
    char buffer[30];
    if (vol > 100) vol = 100;
    else if (vol < 0) vol = 0;
    if (vol != rb->global_settings->volume) {
        rb->mpeg_sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
    rb->snprintf(buffer, sizeof(buffer), "Vol: %d ", vol);
#ifdef HAVE_LCD_BITMAP
        rb->lcd_puts(10,7, buffer);
    rb->lcd_update();
#else
        rb->lcd_puts(0,1, buffer);
#endif
    }
}

void timer_callback(void){
    if(minitick>=period){
    minitick = 0;
    if(!sound_active && !sound_paused){
        play_tock();
    }
    } 
    else {
    minitick++;
    }
}

void cleanup(void *parameter)
{
    (void)parameter;

    rb->plugin_unregister_timer();
    rb->mp3_play_stop(); /* stop audio ISR */
    led(0);
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter){

    int button;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    rb->bitswap(sound, sizeof(sound));

    if (rb->mp3_is_playing())
    rb->mp3_play_stop(); // stop audio ISR

     calc_period();
     rb->plugin_register_timer((FREQ/1024), 1, timer_callback);

     draw_display();

    /* main loop */
    while (true){
                    
    button = rb->button_get(true);
    
    switch (button) {

    case METRONOME_QUIT:
        /* get out of here */
        cleanup(NULL);
        return PLUGIN_OK;

    case METRONOME_PLAYPAUSE:
        if(sound_paused)
        sound_paused = false;
        else
        sound_paused = true;
        calc_period();
        draw_display();
        break;
      
    case METRONOME_VOL_UP:
    case METRONOME_VOL_UP | BUTTON_REPEAT:
        change_volume(1);
        calc_period();
        break;

    case METRONOME_VOL_DOWN:
    case METRONOME_VOL_DOWN | BUTTON_REPEAT:
        change_volume(-1);
        calc_period();
        break;

    case BUTTON_LEFT:
        if (bpm > 1)
        bpm--;
        calc_period();
        draw_display();
        break;

    case BUTTON_LEFT | BUTTON_REPEAT:
        if (bpm > 10)
        bpm=bpm-10;
        calc_period();
        draw_display();
        break;

    case BUTTON_RIGHT:
        if(bpm < 400)
        bpm++;
        calc_period();
        draw_display();
        break;

    case BUTTON_RIGHT | BUTTON_REPEAT:
        if (bpm < 400)
        bpm=bpm+10;
        calc_period();
        draw_display();
        break;
        
    default:
        if (rb->default_event_handler_ex(button, cleanup, NULL)
            == SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;
        break;

    }
    }
}
#endif /* #ifndef SIMULATOR */
