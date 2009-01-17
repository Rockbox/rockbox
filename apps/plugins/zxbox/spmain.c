/*
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "zxmisc.h"
#include "zxconfig.h"
#include "lib/configfile.h"
#include "lib/oldmenuapi.h"

#include "spperif.h"
#include "z80.h"
#include "spmain.h"
#include "sptiming.h"
#include "spscr.h"
#include "spkey_p.h"
#include "spkey.h"
#include "sptape.h"
#include "spsound.h"
#include "snapshot.h"

#include "spconf.h"

#include "interf.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef USE_GREY
#include "lib/grey.h"
#endif

#include "zxbox_keyb.h"

int endofsingle IBSS_ATTR;

int sp_nosync IBSS_ATTR = 0;

int showframe IBSS_ATTR = 1;
int load_immed = 1;

qbyte sp_int_ctr IBSS_ATTR = 0;
int intkeys[5] IBSS_ATTR;

#ifdef USE_DJGPP
#define DOS
#endif

#define GLOBALCFG "zxbox.cfg"

/* here goes settings hadling */

/*static struct zxbox_settings settings;*/
static struct zxbox_settings old_settings;

static char* noyes_options[] = { "No", "Yes" };
/*static char* kempston_options[] = { "No", "Yes" };
static char* showfps_options[] = {"No", "Yes"};*/

static struct configdata config[] =
{
   {TYPE_ENUM, 0, 2, { .int_p = &settings.invert_colors }, "Invert Colors",
    noyes_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.kempston }, "Map keys to kempston",
    noyes_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.showfps }, "Show Speed",
    noyes_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.sound }, "Sound", noyes_options},
   {TYPE_INT, 0, 9, { .int_p = &settings.frameskip }, "Frameskip", NULL},
   {TYPE_INT, 1, 10, { .int_p = &settings.volume }, "Volume", NULL},
   {TYPE_STRING, 0, 5, { .string =  (char*)&settings.keymap }, "Key Mapping",
    NULL},
};
int spcf_read_conf_file(const char *filename)
{
    settings.volume = 10;
    settings.showfps=1;
    settings.keymap[0]='2';
    settings.keymap[1]='w';
    settings.keymap[2]='9';
    settings.keymap[3]='0';
    settings.keymap[4]='z';
    settings.kempston = 1 ;
    settings.invert_colors=0;
    settings.sound = 0;
    settings.frameskip = 0;


    if (configfile_load(filename, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION
                       ) < 0)
    {
        /* If the loading failed, save a new config file (as the disk is
           already spinning) */
        configfile_save(filename, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }
    /* Keep a copy of the saved version of the settings - so we can check if
       the settings have changed when we quit */
    old_settings = settings;
    sound_on = settings.sound;
    showframe = settings.frameskip+1;
    int i;
    for ( i=0 ; i<5 ; i++){
        if (settings.keymap[i] == 'E')
            intkeys[i]=SK_KP_Enter;
        else if ( settings.keymap[i] == 'S' )
            intkeys[i]=SK_KP_Space;
        else
            intkeys[i] = (unsigned) settings.keymap[i];
    }
    return 1;
}

/* and here it stops (settings loading ;) )*/

/* set keys */
static void set_keys(void){
    int m;
    char c;
    int result;
    int menu_quit=0;
    static const struct menu_item items[] = {
        { "Map Up key", NULL },
        { "Map Down key", NULL },
        { "Map Left key", NULL },
        { "Map Right key", NULL },
        { "Map Fire/Jump key", NULL },
       };

    m = menu_init(items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=menu_show(m);

        switch(result)
        {
            case 0:
                if (!zx_kbd_input((char*) &c))
                {
                    settings.keymap[0]=c;
                }
                break;            
            case 1:
                if (!zx_kbd_input((char*) &c))
                {
                    settings.keymap[1]=c;
                }
                break;
            case 2:
                if (!zx_kbd_input((char*) &c))
                {
                    settings.keymap[2]=c;
                }
                break;
            case 3:
                if (!zx_kbd_input((char*) &c))
                {
                    settings.keymap[3]=c;
                }
                break;
            case 4:
                if (!zx_kbd_input((char*) &c))
                {
                    settings.keymap[4]=c;
                }
                break;
            default:
                menu_quit=1;
                break;
        }
    }

    menu_exit(m);
}

/* select predefined keymap */
static void select_keymap(void){
    int m;
    int result;
    int menu_quit=0;
    static const struct menu_item items[] = {
        { "2w90z", NULL },
        { "qaopS", NULL },
        { "7658S", NULL },
       };

    m = menu_init(items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=menu_show(m);

        switch(result)
        {
            case 0:
                rb->memcpy ( (void*)&settings.keymap[0] , (void*)items[0].desc , sizeof(items[0].desc));
                menu_quit=1;
                break;            
            case 1:
                rb->memcpy ( (void*)&settings.keymap[0] , (void*)items[1].desc , sizeof(items[1].desc));
                menu_quit=1;
                break;
            case 2:
                rb->memcpy ( (void*)&settings.keymap[0] , (void*)items[2].desc , sizeof(items[2].desc));
                menu_quit=1;
                break;
            default:
                menu_quit=1;
                break;
        }
    }

    menu_exit(m);
}

/* options menu */
static void options_menu(void){
    static const struct opt_items no_yes[2] = {
        { "No", -1 },
        { "Yes", -1 },
    };
    int m;
    int result;
    int menu_quit=0;
    int new_setting;
    static const struct menu_item items[] = {
        { "Map Keys to kempston", NULL },
        { "Display Speed", NULL },
        { "Invert Colors", NULL },
        { "Frameskip", NULL },
        { "Sound", NULL },
        { "Volume", NULL },
        { "Predefined keymap", NULL },
        { "Custom keymap", NULL },
       };
    static struct opt_items frameskip_items[] = {
        { "0", -1 },
        { "1", -1 },
        { "2", -1 },
        { "3", -1 },
        { "4", -1 },
        { "5", -1 },
        { "6", -1 },
        { "7", -1 },
        { "8", -1 },
        { "9", -1 },
       };
        

    m = menu_init(items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=menu_show(m);

        switch(result)
        {
            case 0:
                new_setting=settings.kempston;
                rb->set_option("Map Keys to kempston",&new_setting,INT, 
                               no_yes, 2, NULL);
                if (new_setting != settings.kempston )
                    settings.kempston=new_setting;
                break;            
            case 1:
                new_setting = settings.showfps;
                rb->set_option("Display Speed",&new_setting,INT, 
                               no_yes, 2, NULL);
                if (new_setting != settings.showfps )
                    settings.showfps=new_setting;
                break;
            case 2:
                new_setting = settings.invert_colors;
                rb->set_option("Invert Colors",&new_setting,INT, 
                               no_yes, 2, NULL);
                if (new_setting != settings.invert_colors )
                    settings.invert_colors=new_setting;
                rb->splash(HZ, "Restart to see effect");
                break;
            case 3:
                new_setting = settings.frameskip;
                rb->set_option("Frameskip",&new_setting,INT, 
                               frameskip_items, 10, NULL);
                if (new_setting != settings.frameskip )
                    settings.frameskip=new_setting;
                break;
            case 4:
                new_setting = settings.sound;
                rb->set_option("Sound",&new_setting,INT, 
                               no_yes, 2, NULL);
                if (new_setting != settings.sound )
                    settings.sound=new_setting;
#if CONFIG_CODEC == SWCODEC && !defined SIMULATOR
                rb->pcm_play_stop();
#endif
                break;
            case 5:
                new_setting = 9 - settings.volume;
                rb->set_option("Volume",&new_setting,INT, 
                               frameskip_items, 10, NULL);
                new_setting = 9 - new_setting;
                if (new_setting != settings.volume )
                    settings.volume=new_setting;
                break;
            case 6:
                select_keymap();
                break;
            case 7:
                set_keys();
                break;
            default:
                menu_quit=1;
                break;
        }
    }

    menu_exit(m);
}

/* menu */
static bool zxbox_menu(void)
{
#if CONFIG_CODEC == SWCODEC && !defined SIMULATOR
    rb->pcm_play_stop();
#endif
    int m;
    int result;
    int menu_quit=0;
    int exit=0;
    char c;
    static const struct menu_item items[] = {
        { "VKeyboard", NULL },
        { "Play/Pause Tape", NULL },
        { "Save quick snapshot", NULL },
        { "Load quick snapshot", NULL },
        { "Save Snapshot", NULL },
        { "Toggle \"fast\" mode", NULL },
        { "Options", NULL },
        { "Quit", NULL },
    };

    m = menu_init(items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=menu_show(m);

        switch(result)
        {
            case 0:
                if (!zx_kbd_input((char*) &c))
                {
                    press_key(c);
                }
                clear_kbd=1;
                menu_quit=1;
                break;
            case 1:
                pause_play();
                menu_quit=1;
                break;
            case 2:
                save_quick_snapshot();
                menu_quit = 1;
                break;
            case 3:
                load_quick_snapshot();
                menu_quit = 1;
                break;
            case 4:
                save_snapshot();
                break;
            case 5:
                sp_nosync=!sp_nosync;
                menu_quit=1;
                break;
            case 6:
                options_menu();
                break;
            case 7:
                menu_quit=1;
                exit=1;
                break;
            default:
                menu_quit=1;
                break;
        }
    }

    menu_exit(m);
#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

    int i;
    for ( i=0 ; i<5 ; i++){
        if (settings.keymap[i] == 'E')
            intkeys[i]=SK_KP_Enter;
        else if ( settings.keymap[i] == 'S' )
            intkeys[i]=SK_KP_Space;
        else
            intkeys[i] = (unsigned) settings.keymap[i];
    }
#ifdef USE_GREY
    grey_show(true);
#endif
    return (exit);
}


/* */
extern unsigned char loadim[];
extern unsigned long loadim_size;

#define SHOW_OFFS 1

static void update(void)
{
  update_screen();
  sp_border_update >>= 1;
  sp_imag_vert = sp_imag_horiz = 0;
}

static void run_singlemode(void)
{
  int t = 0;
  int evenframe, halfsec, updateframe;

  sp_int_ctr = 0;
  endofsingle = 0;
  spti_reset();

  while(!endofsingle) {
      video_frames++;
      if (clear_kbd){
          clear_keystates();
          clear_kbd=0;
      }
      if(exit_requested){
          if (zxbox_menu()){
              /* Save the user settings if they have changed */
                if (rb->memcmp(&settings,&old_settings,sizeof(settings))!=0) {
#ifdef USE_GREY
                    grey_show(false);
#endif
                    rb->splash(0, "Saving settings...");
                    configfile_save(GLOBALCFG, config,sizeof(config)/sizeof(*config),SETTINGS_VERSION);
                }

          return;
          }
          exit_requested = 0;
          video_frames=-1;
          start_time = *rb->current_tick;
          sound_on = settings.sound;
          showframe = settings.frameskip+1;
          spti_reset();
      }
    halfsec = !(sp_int_ctr % 25);
    evenframe = !(sp_int_ctr & 1);

    if(screen_visible) updateframe = sp_nosync ? halfsec : 
      !((sp_int_ctr+SHOW_OFFS) % showframe);
    else updateframe = 0;
    if(halfsec) {
      sp_flash_state = ~sp_flash_state;
      flash_change();
    }
    if(evenframe) {
      play_tape();
      sp_scline = 0;
    }
    spkb_process_events(evenframe);
    sp_updating = updateframe;
    t += CHKTICK;
    t = sp_halfframe(t, evenframe ? EVENHF : ODDHF);
    if(SPNM(load_trapped)) {
      SPNM(load_trapped) = 0;
      DANM(haltstate) = 0;
      qload();
    }
    z80_interrupt(0xFF);
    sp_int_ctr++;
   if(!evenframe) rec_tape();
    if(!sp_nosync) {
      if(!sound_avail) spti_wait();

      if(updateframe) update();
      play_sound(evenframe);
    }
    else if(updateframe) update();
  }
  
}


void check_params(const void* parameter)
{
  spcf_read_conf_file(GLOBALCFG);
  spcf_read_command_line(parameter);
}

static void init_load(const void *parameter)
{
  if(load_immed) snsh_z80_load_intern(loadim, loadim_size);

  check_params(parameter);
  if(spcf_init_snapshot != NULL) {
#ifndef USE_GREY
    rb->splashf(HZ, "Loading snapshot '%s'", spcf_init_snapshot);
#endif
    
    load_snapshot_file_type(spcf_init_snapshot, spcf_init_snapshot_type);
    free_string(spcf_init_snapshot);
  }
  
  if(spcf_init_tapefile != NULL) {
    /*sprintf(msgbuf, "Loading tape '%s'", spcf_init_tapefile);
    put_msg(msgbuf);*/
    start_play_file_type(spcf_init_tapefile, 0, spcf_init_tapefile_type);
    if(!load_immed) pause_play();
    free_string(spcf_init_tapefile);
  }
}

void start_spectemu(const void *parameter)
{
  spti_init();
  init_load(parameter);
  init_spect_scr();
  init_spect_sound();
  init_spect_key();
 
  run_singlemode();
}




