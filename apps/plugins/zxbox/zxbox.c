/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Anton Romanov
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "zxconfig.h"

PLUGIN_HEADER

struct plugin_api* rb;

#include "spkey_p.h"

spkeyboard kb_mkey;
bool exit_requested=false;
bool clear_kbd=0;
extern bool zxbox_menu(void);

/* DUMMIES ... to clean */
unsigned int scrmul=0;
int privatemap;
int use_shm = 0;
int small_screen,pause_on_iconify;
int vga_pause_bg;

#ifdef USE_IRAM
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
extern char iedata[];
extern char iend[];
#endif

#include "keymaps.h"
#include "zxvid_com.h"
#include "spmain.h"
#include "spperif.h"

struct zxbox_settings settings;

/* doesn't fit into .ibss */
unsigned char image_array [ HEIGHT * WIDTH ];

static int previous_state;

#ifdef USE_GRAY
static unsigned char *gbuf;
static unsigned int gbuf_size = 0;
#endif

long video_frames = 0;
long start_time = 0;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{

    rb = api;
#if CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif
    rb->splash(HZ, true, "Welcome to ZXBox");
    sp_init();

#ifdef USE_IRAM
   /* We need to stop audio playback in order to use IRAM */
   rb->audio_stop();

   rb->memcpy(iramstart, iramcopy, iramend-iramstart);
   rb->memset(iedata, 0, iend - iedata);
#endif

#ifdef USE_GRAY
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);
#ifdef USE_BUFFERED_GRAY
    gray_init(rb, gbuf, gbuf_size, true, LCD_WIDTH, LCD_HEIGHT, 15, 0, NULL);
#else
    gray_init(rb, gbuf, gbuf_size, false, LCD_WIDTH, LCD_HEIGHT, 15, 0, NULL);
#endif /* USE_BUFFERED_GRAY */
    /* switch on grayscale overlay */
   gray_show(true);
#endif /* USE_GRAY */


#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    start_time = *rb->current_tick;
    start_spectemu(parameter);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif

#ifdef USE_GRAY
gray_show(false);
gray_release();
#endif 

#if CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif

return PLUGIN_OK;
}

void init_spect_key(void)
{
  clear_keystates();
  init_basekeys();
}

void spkb_process_events( int evenframe )
{

    if(evenframe){
        int ki;
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD)
        if (rb->button_hold())
        {
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(false);
#endif
            exit_requested=1;
#ifdef USE_GRAY
            gray_show(false);
#endif
            return;
        }
#endif
        int buttons = rb->button_status();
        if ( buttons == previous_state )
            return;
        previous_state = buttons;
#if (CONFIG_KEYPAD != IPOD_4G_PAD) && \
      (CONFIG_KEYPAD != IPOD_3G_PAD)
        if (buttons & ZX_MENU)
        {
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
            rb->cpu_boost(false);
#endif
            exit_requested=1;
#ifdef USE_GRAY
            gray_show(false);
#endif
            return;
        }
#endif
        spkb_state_changed = 1;
        if (settings.kempston){
            if ( buttons & ZX_RIGHT ){
                 ki = KS_TO_KEY(SK_KP_Right);
                spkb_kbstate[ki].state = 1;
            }
            else if (buttons & ZX_LEFT){
                 ki = KS_TO_KEY(SK_KP_Left);
                spkb_kbstate[ki].state = 1;

            }
            else{
                 ki = KS_TO_KEY(SK_KP_Right);
                spkb_kbstate[ki].state = 0;
                 ki = KS_TO_KEY(SK_KP_Left);
                spkb_kbstate[ki].state = 0;

            }
            if ( buttons & ZX_UP ){
                 ki = KS_TO_KEY(SK_KP_Up);
                spkb_kbstate[ki].state = 1;
            }
            else if (buttons & ZX_DOWN){
                 ki = KS_TO_KEY(SK_KP_Down);
                spkb_kbstate[ki].state = 1;
            }
            else{
                 ki = KS_TO_KEY(SK_KP_Up);
                spkb_kbstate[ki].state = 0;
                 ki = KS_TO_KEY(SK_KP_Down);
                spkb_kbstate[ki].state = 0;
            }

            if ( buttons & ZX_SELECT ){
                 ki = KS_TO_KEY(SK_KP_Insert);
                spkb_kbstate[ki].state = 1;
            }
            else{
                 ki = KS_TO_KEY(SK_KP_Insert);
                spkb_kbstate[ki].state = 0;
            }

        }
        else {

            if ( buttons & ZX_RIGHT ){
                ki = KS_TO_KEY(intkeys[3]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[3]);
                spkb_kbstate[ki].state = 0;
            }
    
            if ( buttons & ZX_LEFT ){
                ki = KS_TO_KEY(intkeys[2]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[2]);
                spkb_kbstate[ki].state = 0;
            }
    
            if ( buttons & ZX_UP ){
                ki = KS_TO_KEY(intkeys[0]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[0]);
                spkb_kbstate[ki].state = 0;
            }
            
            if ( buttons & ZX_DOWN ){
                ki = KS_TO_KEY(intkeys[1]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[1]);
                spkb_kbstate[ki].state = 0;
            }
            
            if ( buttons & ZX_SELECT ){
                ki = KS_TO_KEY(intkeys[4]);
                spkb_kbstate[ki].state = 1;
            }
            else{
                ki = KS_TO_KEY(intkeys[4]);
                spkb_kbstate[ki].state = 0;
            }
        }
        process_keys();
    }
}

void press_key(int c){
    spkb_state_changed = 1;
    int ki;
    if ( c == 'E' )
        ki = KS_TO_KEY(SK_KP_Enter);
    else if (c == 'S' )
        ki = KS_TO_KEY(SK_KP_Space);
    else
        ki = KS_TO_KEY(c);
    spkb_kbstate[ki].state = 1;
    process_keys();
}

void spkey_textmode(void)
{
}

void spkey_screenmode(void)
{
}

void spscr_refresh_colors(void)
{
}

void resize_spect_scr(int s)
{
    /* just to disable warning */
    (void)s;
}

int display_keyboard(void)
{
  return 0;
}
