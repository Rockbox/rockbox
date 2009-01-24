/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gameboy emulator based on gnuboy
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
#include "loader.h"
#include "rockmacros.h"
#include "input.h"
#include "emu.h"
#include "hw.h"
#include "pcm.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

int shut,cleanshut;
char *errormsg;

#define optionname "options"

void die(char *message, ...)
{
    shut=1;
    errormsg=message;
}

struct options options;

void *audio_bufferbase;
void *audio_bufferpointer;
size_t audio_buffer_free;

void *my_malloc(size_t size)
{
    void *alloc;

    if (size + 4 > audio_buffer_free)
        return 0;
    alloc = audio_bufferpointer;
    audio_bufferpointer += size + 4;
    audio_buffer_free -= size + 4;
    return alloc;
}

static void setoptions (void)
{
   int fd;
   DIR* dir;
   char optionsave[sizeof(savedir)+sizeof(optionname)];

    dir=rb->opendir(savedir);
    if(!dir)
      rb->mkdir(savedir);
    else
      rb->closedir(dir);

   snprintf(optionsave, sizeof(optionsave), "%s/%s", savedir, optionname);

    fd = open(optionsave, O_RDONLY);
    if(fd < 0) /* no options to read, set defaults */
    {
#ifdef HAVE_TOUCHSCREEN
        options.LEFT=BUTTON_MIDLEFT;
        options.RIGHT=BUTTON_MIDRIGHT;
#else
        options.LEFT=BUTTON_LEFT;
        options.RIGHT=BUTTON_RIGHT;
#endif

#if CONFIG_KEYPAD == IRIVER_H100_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_ON;
        options.B=BUTTON_OFF;
        options.START=BUTTON_REC;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MODE;

#elif CONFIG_KEYPAD == IRIVER_H300_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_REC;
        options.B=BUTTON_MODE;
        options.START=BUTTON_ON;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_OFF;

#elif CONFIG_KEYPAD == RECORDER_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_F1;
        options.B=BUTTON_F2;
        options.START=BUTTON_F3;
        options.SELECT=BUTTON_PLAY;
        options.MENU=BUTTON_OFF;

#elif CONFIG_KEYPAD == IPOD_4G_PAD
        options.UP=BUTTON_MENU;
        options.DOWN=BUTTON_PLAY;

        options.A=BUTTON_NONE;
        options.B=BUTTON_NONE;
        options.START=BUTTON_SELECT;
        options.SELECT=BUTTON_NONE;
        options.MENU=(BUTTON_SELECT | BUTTON_REPEAT);

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_EQ;
        options.START=BUTTON_MODE;
        options.SELECT=(BUTTON_SELECT | BUTTON_REL);
        options.MENU=(BUTTON_SELECT | BUTTON_REPEAT);

#elif CONFIG_KEYPAD == GIGABEAT_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_VOL_UP;
        options.B=BUTTON_VOL_DOWN;
        options.START=BUTTON_A;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MENU;
      
#elif CONFIG_KEYPAD == SANSA_E200_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_SELECT;
        options.B=BUTTON_REC;
        options.START=BUTTON_SCROLL_BACK;
        options.SELECT=BUTTON_SCROLL_FWD;
        options.MENU=BUTTON_POWER;

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_SELECT;
        options.B=BUTTON_HOME;
        options.START=BUTTON_SCROLL_BACK;
        options.SELECT=BUTTON_SCROLL_FWD;
        options.MENU=BUTTON_POWER;

#elif CONFIG_KEYPAD == SANSA_C200_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_SELECT;
        options.B=BUTTON_REC;
        options.START=BUTTON_VOL_DOWN;
        options.SELECT=BUTTON_VOL_UP;
        options.MENU=BUTTON_POWER;

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_REC;
        options.START=BUTTON_SELECT;
        options.SELECT=BUTTON_NONE;
        options.MENU=BUTTON_POWER;  

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
        options.UP=BUTTON_SCROLL_UP;
        options.DOWN=BUTTON_SCROLL_DOWN;

        options.A=BUTTON_PLAY;
        options.B=BUTTON_FF;
        options.START=BUTTON_REW;
        options.SELECT=BUTTON_NONE;
        options.MENU=BUTTON_POWER;
        
#elif CONFIG_KEYPAD == MROBE500_PAD
        options.UP=BUTTON_RC_PLAY;
        options.DOWN=BUTTON_RC_DOWN;
        options.LEFT=BUTTON_RC_REW;
        options.RIGHT=BUTTON_RC_FF;

        options.A=BUTTON_RC_VOL_DOWN;
        options.B=BUTTON_RC_VOL_UP;
        options.START=BUTTON_RC_HEART;
        options.SELECT=BUTTON_RC_MODE;
        options.MENU=BUTTON_POWER;

#elif CONFIG_KEYPAD == COWOND2_PAD
        options.A=BUTTON_PLUS;
        options.B=BUTTON_MINUS;
        options.MENU=BUTTON_MENU;

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_VOL_UP;
        options.B=BUTTON_VOL_DOWN;
        options.START=BUTTON_PLAY;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MENU;
#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_CUSTOM;
        options.B=BUTTON_PLAY;
        options.START=BUTTON_BACK;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MENU;

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
        options.UP=BUTTON_UP;
        options.DOWN=BUTTON_DOWN;

        options.A=BUTTON_VOL_UP;
        options.B=BUTTON_VOL_DOWN;
        options.START=BUTTON_VIEW;
        options.SELECT=BUTTON_SELECT;
        options.MENU=BUTTON_MENU;
#else
#error No Keymap Defined!
#endif

#ifdef HAVE_TOUCHSCREEN
        options.UP=BUTTON_TOPMIDDLE;
        options.DOWN=BUTTON_BOTTOMMIDDLE;
        options.START=BUTTON_TOPRIGHT;
        options.SELECT=BUTTON_CENTER;
#if CONFIG_KEYPAD != COWOND2_PAD
        options.A=BUTTON_BOTTOMLEFT;
        options.B=BUTTON_BOTTOMRIGHT;
        options.MENU=BUTTON_TOPLEFT;
#endif
#endif

      options.maxskip=4;
      options.fps=0;
      options.showstats=0;
#if (LCD_WIDTH>=160) && (LCD_HEIGHT>=144)
      options.scaling=0;
#else
      options.scaling=1;
#endif
      options.sound=1;
      options.pal=0;
   }
   else
      read(fd,&options, sizeof(options));

    close(fd);
}

static void savesettings(void)
{
    int fd;
    char optionsave[sizeof(savedir)+sizeof(optionname)];

    if(options.dirty)
    {
        options.dirty=0;
        snprintf(optionsave, sizeof(optionsave), "%s/%s", savedir, optionname);
        fd = open(optionsave, O_WRONLY|O_CREAT|O_TRUNC);
        write(fd,&options, sizeof(options));
        close(fd);
    }
}

void doevents(void)
{
    event_t ev;
    int st;

    ev_poll();
    while (ev_getevent(&ev))
    {
        if (ev.type != EV_PRESS && ev.type != EV_RELEASE)
            continue;
        st = (ev.type != EV_RELEASE);
        pad_set(ev.code, st);
    }
}

static int gnuboy_main(const char *rom)
{
    rb->lcd_puts(0,0,"Init video");
    vid_init();
    rb->lcd_puts(0,1,"Init sound");
    pcm_init();
    rb->lcd_puts(0,2,"Loading rom");
    loader_init(rom);
    if(shut)
        return PLUGIN_ERROR;
    rb->lcd_puts(0,3,"Emu reset");
    emu_reset();
    rb->lcd_puts(0,4,"Emu run");
    rb->lcd_clear_display();
    rb->lcd_update();
    emu_run();

    /* never reached */
    return PLUGIN_OK;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    PLUGIN_IRAM_INIT(rb)

    rb->lcd_setfont(0);

    rb->lcd_clear_display();

    if (!parameter)
    {
        rb->splash(HZ*3, "Play gameboy ROM file! (.gb/.gbc)");
        return PLUGIN_OK;
    }
    if(rb->audio_status())
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_buffer(&audio_buffer_free);
        plugbuf=true;
    }
    else
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_audio_buffer(&audio_buffer_free);
        plugbuf=false;
    }
#if MEM <= 8 && !defined(SIMULATOR)
    /* loaded as an overlay plugin, protect from overwriting ourselves */
    if ((unsigned)(plugin_start_addr - (unsigned char *)audio_bufferbase)
        < audio_buffer_free)
        audio_buffer_free = plugin_start_addr - (unsigned char *)audio_bufferbase;
#endif
    setoptions();

    shut=0;
    cleanshut=0;

#ifdef HAVE_WHEEL_POSITION
    rb->wheel_send_events(false);
#endif

    gnuboy_main(parameter);

#ifdef HAVE_WHEEL_POSITION
    rb->wheel_send_events(true);
#endif

    if(shut&&!cleanshut)
    {
        rb->splash(HZ/2, errormsg);
        return PLUGIN_ERROR;
    }
    if(!rb->audio_status())
        pcm_close();
    rb->splash(HZ/2, "Closing Rockboy");

    savesettings();

    cleanup();

    return PLUGIN_OK;
}
