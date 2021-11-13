/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (C) 2003-2005 Jörg Hohensohn
 * Copyright (C) 2020 BILGUS
 *
 *
 *
 * Usage: Start plugin, it will stay in the background.
 *
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
#include "lib/helper.h"
#include "lib/kbd_helper.h"
#include "lib/configfile.h"

/****************** constants ******************/
#define MAX_GROUPS 7
#define MAX_ANNOUNCE_WPS 63
#define ANNOUNCEMENT_TIMEOUT  10
#define GROUPING_CHAR ';'

#define EV_EXIT        MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)
#define EV_OTHINSTANCE MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFE)
#define EV_STARTUP     MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)
#define EV_TRACKCHANGE MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x02)

#define CFG_FILE "/VoiceTSR.cfg"
#define CFG_VER  1

#define THREAD_STACK_SIZE 4*DEFAULT_STACK_SIZE

#if CONFIG_RTC
    #define K_TIME     "DT D1;\n\n"
    #define K_DATE     "DD D2;\n\n"
#else
    #define K_TIME     ""
    #define K_DATE     ""
#endif

#define K_TRACK_TA "TT TA;\n"
#define K_TRACK    "TE TL TR;\n"
#define K_TRACK1   "T1 T2 T3;\n\n"
#define K_PLAYLIST "PC PN PR P1 P2;\n"
#define K_BATTERY  "BP BM B1;\n"
#define K_SLEEP    "RS R2 R3;\n"
#define K_RUNTIME  "RT R1;"

static const char keybd_layout[] =
    K_TIME K_DATE K_TRACK_TA K_TRACK K_TRACK1 K_PLAYLIST K_BATTERY K_SLEEP K_RUNTIME;

/* - each character in keybd_layout will consume one element
 * - \n does not create a key, but it also consumes one element
 * - the final null terminator is equivalent to \n
 * - since sizeof includes the null terminator we don't need +1 for that. */
static unsigned short kbd_buf[sizeof(keybd_layout)];

/****************** prototypes ******************/
void print_scroll(char* string); /* implements a scrolling screen */

int get_playtime(void); /* return the current track time in seconds */
int get_tracklength(void); /* return the total length of the current track */
int get_track(void); /* return the track number */
void get_playmsg(void); /* update the play message with Rockbox info */

void thread_create(void);
void thread(void); /* the thread running it all */
void thread_quit(void);
static int voice_general_info(bool testing);
static unsigned char* voice_info_group(unsigned char* current_token, bool testing);

int plugin_main(const void* parameter); /* main loop */
enum plugin_status plugin_start(const void* parameter); /* entry */


/****************** data types ******************/

/****************** globals ******************/
/* communication to the worker thread */
static struct
{
    bool exiting; /* signal to the thread that we want to exit */
    unsigned int id; /* worker thread id */
    struct  event_queue queue; /* thread event queue */
    long stack[THREAD_STACK_SIZE / sizeof(long)];
} gThread;

static struct
{
    int interval;
    int announce_on;
    int grouping;

    int timeout;
    int count;
    unsigned int index;
    int bin_added;

    bool show_prompt;

    unsigned char wps_fmt[MAX_ANNOUNCE_WPS+1];
} gAnnounce;

static struct configdata config[] =
{
   {TYPE_INT, 0, 10000, { .int_p = &gAnnounce.interval }, "Interval", NULL},
   {TYPE_INT, 0, 2, { .int_p = &gAnnounce.announce_on }, "Announce", NULL},
   {TYPE_INT, 0, 10, { .int_p = &gAnnounce.grouping }, "Grouping", NULL},
   {TYPE_INT, 0, 10000, { .int_p = &gAnnounce.bin_added }, "Added", NULL},
   {TYPE_BOOL, 0, 1, { .bool_p = &gAnnounce.show_prompt }, "Prompt", NULL},
   {TYPE_STRING, 0, MAX_ANNOUNCE_WPS+1,
                   { .string =  (char*)&gAnnounce.wps_fmt }, "Fmt", NULL},
};

const int gCfg_sz = sizeof(config)/sizeof(*config);
/****************** communication with Rockbox playback ******************/

#if 0
/* return the track number */
int get_track(void)
{
    //if (rb->audio_status() == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->index + 1; /* track numbers start with 1 */
}
#endif

static void playback_event_callback(unsigned short id, void *data)
{
    (void)id;
    (void)data;
    if (gThread.id > 0)
        rb->queue_post(&gThread.queue, EV_TRACKCHANGE, 0);
}

/****************** config functions *****************/
static void config_set_defaults(void)
{
    gAnnounce.bin_added = 0;
    gAnnounce.interval = ANNOUNCEMENT_TIMEOUT;
    gAnnounce.announce_on = 0;
    gAnnounce.grouping = 0;
    gAnnounce.wps_fmt[0] = '\0';
    gAnnounce.show_prompt = true; 
}

static void config_reset_voice(void)
{
    /* don't want to change these so save a copy */
    int interval = gAnnounce.interval;
    int announce = gAnnounce.announce_on;
    int grouping = gAnnounce.grouping;

    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        rb->splash(100, "ERROR!");
        return;
    }

    /* restore other settings */
    gAnnounce.interval = interval;
    gAnnounce.announce_on = announce;
    gAnnounce.grouping = grouping;
}

/****************** helper fuctions ******************/
void announce(void)
{
    rb->talk_force_shutup();
    rb->sleep(HZ / 2);
    voice_general_info(false);
    if (rb->talk_id(VOICE_PAUSE, true) < 0)
        rb->beep_play(800, 100, 1000);
    //rb->talk_force_enqueue_next();
}

static void announce_test(void)
{
    rb->talk_force_shutup();
    rb->sleep(HZ / 2);
    voice_info_group(gAnnounce.wps_fmt, true);
    rb->splash(HZ, "...");
    //rb->talk_force_enqueue_next();
}

static void announce_add(const char *str)
{
    int len_cur = rb->strlen(gAnnounce.wps_fmt);
    int len_str = rb->strlen(str);
    if (len_cur + len_str > MAX_ANNOUNCE_WPS)
        return;
    rb->strcpy(&gAnnounce.wps_fmt[len_cur], str);
    announce_test();

}

static int _playlist_get_display_index(struct playlist_info *playlist)
{
    /* equivalent of the function found in playlist.c */
    if(!playlist)
        return -1;
    /* first_index should always be index 0 for display purposes */
    int index = playlist->index;
    index -= playlist->first_index;
    if (index < 0)
        index += playlist->amount;

    return index + 1;
}

static enum themable_icons icon_callback(int selected_item, void * data)
{
    (void)data;

    if(selected_item < MAX_GROUPS && selected_item >= 0)
    {
        int bin = 1 << (selected_item);
        if ((gAnnounce.bin_added & bin) == bin)
            return Icon_Submenu;
    }

    return Icon_NOICON;
}

static int announce_menu_cb(int action,
                            const struct menu_item_ex *this_item,
                            struct gui_synclist *this_list)
{
    (void)this_item;
    unsigned short* kbd_p;

    int selection = rb->gui_synclist_get_sel_pos(this_list);

    if(action == ACTION_ENTER_MENUITEM)
    {
        rb->gui_synclist_set_icon_callback(this_list, icon_callback);
    }
    else if ((action == ACTION_STD_OK))
    {
        //rb->splashf(100, "%d", selection);
        if (selection < MAX_GROUPS && selection >= 0) /* only add premade tags once */
        {
            int bin = 1 << (selection);
            if ((gAnnounce.bin_added & bin) == bin)
                return 0;

            gAnnounce.bin_added |= bin;
        }

        switch(selection) {

            case 0: /*Time*/
                announce_add("D1Dt ;");
                break;
            case 1: /*Date*/
                announce_add("D2Dd ;");
                break;
            case 2: /*Track*/
                announce_add("TT TA T1TeT2Tr ;");
                break;
            case 3: /*Playlist*/
                announce_add("P1PC P2PN ;");
                break;
            case 4: /*Battery*/
                announce_add("B1Bp ;");
                break;
            case 5: /*Sleep*/
                announce_add("R2RsR3 ;");
                break;
            case 6: /*Runtime*/
                announce_add("R1Rt ;");
                break;
            case 7: /* sep */
                break;
            case 8: /*Clear All*/
                gAnnounce.wps_fmt[0] = '\0';
                gAnnounce.bin_added = 0;
                rb->splash(HZ / 2, ID2P(LANG_RESET_DONE_CLEAR));
                break;
            case 9: /* inspect it */
                kbd_p = kbd_buf;
                if (!kbd_create_layout(keybd_layout, kbd_p, sizeof(kbd_buf)))
                    kbd_p = NULL;

                rb->kbd_input(gAnnounce.wps_fmt, MAX_ANNOUNCE_WPS, kbd_p);
                break;
            case 10: /*test it*/
                announce_test();
                break;
            case 11: /*cancel*/
                config_reset_voice();
                return ACTION_STD_CANCEL;
            case 12: /* save */
                return ACTION_STD_CANCEL;
            default:
                return action;
        }
        rb->gui_synclist_draw(this_list); /* redraw */
        return 0;
    }

    return action;
}

static int announce_menu(void)
{
    int selection = 0;

    MENUITEM_STRINGLIST(announce_menu, "Announcements", announce_menu_cb,
                        ID2P(LANG_TIME),
                        ID2P(LANG_DATE),
                        ID2P(LANG_TRACK),
                        ID2P(LANG_PLAYLIST),
                        ID2P(LANG_BATTERY_MENU),
                        ID2P(LANG_SLEEP_TIMER),
                        ID2P(LANG_RUNNING_TIME),
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_CLEAR_ALL),
                        ID2P(LANG_ANNOUNCEMENT_FMT),
                        ID2P(LANG_VOICE),
                        ID2P(LANG_CANCEL_0),
                        ID2P(LANG_SAVE));

    selection = rb->do_menu(&announce_menu, &selection, NULL, true);
    if (selection == MENU_ATTACHED_USB)
        return PLUGIN_USB_CONNECTED;

    return 0;
}

/**
  Shows the settings menu
 */
static int settings_menu(void)
{
    int selection = 0;
    //bool old_val;

    MENUITEM_STRINGLIST(settings_menu, "Announce Settings", NULL,
                        ID2P(LANG_TIMEOUT),
                        ID2P(LANG_ANNOUNCE_ON),
                        ID2P(LANG_GROUPING),
                        ID2P(LANG_ANNOUNCEMENT_FMT),
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_MENU_QUIT),
                        ID2P(LANG_SAVE_EXIT));

    static const struct opt_items announce_options[] = {
        { STR(LANG_OFF)},
        { STR(LANG_TRACK_CHANGE)},
    };

    do {
        selection=rb->do_menu(&settings_menu,&selection, NULL, true);
        switch(selection) {

            case 0:
                rb->set_int(rb->str(LANG_TIMEOUT), "s", UNIT_SEC,
                            &gAnnounce.interval, NULL, 1, 1, 360, NULL );
                break;
            case 1:
                rb->set_option(rb->str(LANG_ANNOUNCE_ON),
                      &gAnnounce.announce_on, INT, announce_options, 2, NULL);
                break;
            case 2:
                rb->set_int(rb->str(LANG_GROUPING), "", 1,
                            &gAnnounce.grouping, NULL, 1, 0, 7, NULL );
                break;
            case 3:
                announce_menu();
                break;
            case 4: /*sep*/
                continue;
            case 5:
                return -1;
                break;
            case 6:
                configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
                return 0;
                break;

            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                return 0;
        }
    } while ( selection >= 0 );
    return 0;
}


/****************** main thread + helper ******************/
void thread(void)
{
    bool in_usb = false;
    long interval;
    long last_tick = *rb->current_tick; /* for 1 sec tick */

    struct queue_event ev;
    while (!gThread.exiting)
    {
        rb->queue_wait(&gThread.queue, &ev);
        interval = gAnnounce.interval * HZ;
        switch (ev.id)
        {
            case SYS_USB_CONNECTED:
                rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
                in_usb = true;
                break;
            case SYS_USB_DISCONNECTED:
                in_usb = false;
                /*fall through*/
            case EV_STARTUP:
                rb->beep_play(1500, 100, 1000);
                break;
            case EV_EXIT:
                return;
            case EV_OTHINSTANCE:
                if (*rb->current_tick - last_tick >= interval)
                {
                    last_tick += interval;
                    rb->sleep(HZ / 10);
                    if (!in_usb) announce();
                }
                break;
            case EV_TRACKCHANGE:
                rb->sleep(HZ / 10);
                if (!in_usb) announce();
                break;
        }
    }
}

void thread_create(void)
{
    /* put the thread's queue in the bcast list */
    rb->queue_init(&gThread.queue, true);
    gThread.id = rb->create_thread(thread, gThread.stack, sizeof(gThread.stack),
                                      0, "vTSR"
                                      IF_PRIO(, PRIORITY_BACKGROUND)
                                      IF_COP(, CPU));
    rb->queue_post(&gThread.queue, EV_STARTUP, 0);
    rb->yield();
}

void thread_quit(void)
{
    if (!gThread.exiting) {
        rb->queue_post(&gThread.queue, EV_EXIT, 0);
        rb->thread_wait(gThread.id);
        /* we don't want any more events */
        rb->remove_event(PLAYBACK_EVENT_TRACK_CHANGE, playback_event_callback);

        /* remove the thread's queue from the broadcast list */
        rb->queue_delete(&gThread.queue);
        gThread.exiting = true;
    }
}

/* callback to end the TSR plugin, called before a new one gets loaded */
static bool exit_tsr(bool reenter)
{
    if (reenter)
    {
        rb->queue_post(&gThread.queue, EV_OTHINSTANCE, 0);
        return false; /* dont let it start again */
    }
    thread_quit();

    return true;
}


/****************** main ******************/

int plugin_main(const void* parameter)
{
    (void)parameter;
    bool settings = false;
    int i = 0;

    rb->memset(&gThread, 0, sizeof(gThread));

    gAnnounce.index = 0;
    gAnnounce.timeout = 0;

    rb->splash(HZ / 2, "Announce Status");

    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        /* If the loading failed, save a new config file */
        config_set_defaults();
        configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);

        rb->splash(HZ, ID2P(LANG_HOLD_FOR_SETTINGS));
    }

    if (gAnnounce.show_prompt)
    {
        if (rb->mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) != CHANNEL_PLAYING)
        {
            rb->talk_id(LANG_HOLD_FOR_SETTINGS, false);
        }
        rb->splash(HZ, ID2P(LANG_HOLD_FOR_SETTINGS));
    }

    rb->button_clear_queue();
    if (rb->button_get_w_tmo(HZ) > BUTTON_NONE)
    {
        while ((rb->button_get(false) & BUTTON_REL) != BUTTON_REL)
        {
            if (i & 1)
                rb->beep_play(800, 100, 1000);

            if (++i > 15)
            {
                settings = true;
                break;
            }
            rb->sleep(HZ / 5);
        }
    }

    if (settings)
    {
        rb->splash(100, ID2P(LANG_SETTINGS));
        int ret = settings_menu();
        if (ret < 0)
            return 0;
    }

    gAnnounce.timeout = *rb->current_tick;
    rb->plugin_tsr(exit_tsr); /* stay resident */

    if (gAnnounce.announce_on == 1)
        rb->add_event(PLAYBACK_EVENT_TRACK_CHANGE, playback_event_callback);

    thread_create();

    return 0;
}


/***************** Plugin Entry Point *****************/


enum plugin_status plugin_start(const void* parameter)
{
    /* now go ahead and have fun! */
    if (rb->usb_inserted() == true)
        return PLUGIN_USB_CONNECTED;
    int ret = plugin_main(parameter);
    return (ret==0) ? PLUGIN_OK : PLUGIN_ERROR;
}

static int voice_general_info(bool testing)
{
    unsigned char* infotemplate = gAnnounce.wps_fmt;

    if (gAnnounce.index >= rb->strlen(gAnnounce.wps_fmt))
        gAnnounce.index = 0;

    long current_tick = *rb->current_tick;

    if (*infotemplate == 0)
    {
    #if CONFIG_RTC
            /* announce the time */
            voice_info_group("D1Dt ", false);
    #else
            /* announce elapsed play for this track */
            voice_info_group("T1Te ", false);
    #endif
        return -1;
    }

    if (TIME_BEFORE(current_tick, gAnnounce.timeout))
    {
        return -2;
    }

    gAnnounce.timeout = current_tick + gAnnounce.interval * HZ;

    rb->talk_shutup();

    gAnnounce.count = 0;
    infotemplate = voice_info_group(&infotemplate[gAnnounce.index], testing);
    gAnnounce.index = (infotemplate - gAnnounce.wps_fmt) + 1;

    return 0;
}

static unsigned char* voice_info_group(unsigned char* current_token, bool testing)
{
    unsigned char current_char;
    bool skip_next_group = false;
    gAnnounce.count = 0;

    while (*current_token != 0)
    {
        //rb->splash(10, current_token);
        current_char = toupper(*current_token);
        if (current_char == 'D')
        {
            /*
                Date and time functions
            */
            current_token++;

            current_char = toupper(*current_token);

#if CONFIG_RTC
            struct tm *tm = rb->get_time();

            if (true) //(valid_time(tm))
            {
                if (current_char == 'T')
                {
                    rb->talk_time(tm, true);
                }
                else if (current_char == 'D')
                {
                    rb->talk_date(tm, true);
                }
                /* prefix suffix connectives */
                else if (current_char == '1')
                {
                    rb->talk_id(LANG_TIME, true);
                }
                else if (current_char == '2')
                {
                    rb->talk_id(LANG_DATE, true);
                }
            }
#endif
        }
        else if (current_char == 'R')
        {
            /*
                Sleep timer and runtime
            */
            int sleep_remaining = rb->get_sleep_timer();
            int runtime;

            current_token++;
            current_char = toupper(*current_token);
            if (current_char == 'T')
            {
                runtime = rb->global_status->runtime;
                talk_val(runtime, UNIT_TIME, true);
            }
            /* prefix suffix connectives */
            else if (current_char == '1')
            {
                rb->talk_id(LANG_RUNNING_TIME, true);
            }
            else if (testing || sleep_remaining > 0)
            {
                if (current_char == 'S')
                {
                    talk_val(sleep_remaining, UNIT_TIME, true);
                }
                /* prefix suffix connectives */
                else if (current_char == '2')
                {
                    rb->talk_id(LANG_SLEEP_TIMER, true);
                }
                else if (current_char == '3')
                {
                    rb->talk_id(LANG_REMAIN, true);
                }
            }
            else if (sleep_remaining == 0)
            {
                skip_next_group = true;
            }

        }
        else if (current_char == 'T')
        {
            /*
                Current track information
            */
            current_token++;

            current_char = toupper(*current_token);

            struct mp3entry* id3 = rb->audio_current_track();

            int elapsed_length = id3->elapsed / 1000;
            int track_length = id3->length / 1000;
            int track_remaining = track_length - elapsed_length;

            if (current_char == 'E')
            {
                talk_val(elapsed_length, UNIT_TIME, true);
            }
            else if (current_char == 'L')
            {
                talk_val(track_length, UNIT_TIME, true);
            }
            else if (current_char == 'R')
            {
                talk_val(track_remaining, UNIT_TIME, true);
            }
            else if (current_char == 'T' && id3->title)
            {
                rb->talk_spell(id3->title, true);
            }
            else if (current_char == 'A' && id3->artist)
            {
                rb->talk_spell(id3->artist, true);
            }
            else if (current_char == 'A' && id3->albumartist)
            {
                rb->talk_spell(id3->albumartist, true);
            }
            /* prefix suffix connectives */
            else if (current_char == '1')
            {
                rb->talk_id(LANG_ELAPSED, true);
            }
            else if (current_char == '2')
            {
                rb->talk_id(LANG_REMAIN, true);
            }
            else if (current_char == '3')
            {
                rb->talk_id(LANG_OF, true);
            }
        }
        else if (current_char == 'P')
        {
            /*
                Current playlist information
            */
            current_token++;

            current_char = toupper(*current_token);
            struct playlist_info *pl;
            int current_track = 0;
            int remaining_tracks = 0;
            int total_tracks = rb->playlist_amount();

            if (!isdigit(current_char)) {
                pl = rb->playlist_get_current();
                current_track = _playlist_get_display_index(pl);
                remaining_tracks = total_tracks - current_track;
            }

            if (total_tracks > 0 || testing)
            {
                if (current_char == 'C')
                {
                    rb->talk_number(current_track, true);
                }
                else if (current_char == 'N')
                {
                    rb->talk_number(total_tracks, true);
                }
                else if (current_char == 'R')
                {
                    rb->talk_number(remaining_tracks, true);
                }
                /* prefix suffix connectives */
                else if (current_char == '1')
                {
                    rb->talk_id(LANG_TRACK, true);
                }
                else if (current_char == '2')
                {
                    rb->talk_id(LANG_OF, true);
                }
            }
            else if (total_tracks == 0)
                skip_next_group = true;
        }
        else if (current_char == 'B')
        {
            /*
                Battery
            */
            current_token++;

            current_char = toupper(*current_token);

            if (current_char == 'P')
            {
                talk_val(rb->battery_level(), UNIT_PERCENT, true);
            }
            else if (current_char == 'M')
            {
                talk_val(rb->battery_time() * 60, UNIT_TIME, true);
            }
            /* prefix suffix connectives */
            else if (current_char == '1')
            {
                rb->talk_id(LANG_BATTERY_TIME, true);
            }
        }
        else if (current_char == ' ')
        {
            /*
                Catch your breath
            */
            rb->talk_id(VOICE_PAUSE, true);
        }
        else if (current_char == GROUPING_CHAR && gAnnounce.grouping > 0)
        {
            gAnnounce.count++;

            if (gAnnounce.count >= gAnnounce.grouping && !testing && !skip_next_group)
                break;

            skip_next_group = false;

        }
        current_token++;
    }

    return current_token;
}
