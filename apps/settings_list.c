/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:  $
 *
 * Copyright (C) 2007 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include <stdbool.h>

#include "lang.h"
#include "lcd.h"
#include "settings.h"
#include "settings_list.h"
#include "sound.h"

/* some sets of values which are used more than once, to save memory */
static const char off_on[] = "off,on";
static const char off_on_ask[] = "off,on,ask";
static const char off_number_spell_hover[] = "off,number,spell,hover";
#ifdef HAVE_LCD_BITMAP
static const char graphic_numeric[] = "graphic,numeric";
#endif

#ifdef HAVE_RECORDING
/* keep synchronous to trig_durations and
   trigger_times in settings_apply_trigger */
static const char trig_durations_conf [] =
                  "0s,1s,2s,5s,10s,15s,20s,25s,30s,1min,2min,5min,10min";
/* these should be in the config.h files */
#if CONFIG_CODEC == MAS3587F
# define DEFAULT_REC_MIC_GAIN 8
# define DEFAULT_REC_LEFT_GAIN 2
# define DEFAULT_REC_RIGHT_GAIN 2
#elif CONFIG_CODEC == SWCODEC
# ifdef HAVE_UDA1380
#  define DEFAULT_REC_MIC_GAIN 16
#  define DEFAULT_REC_LEFT_GAIN 0
#  define DEFAULT_REC_RIGHT_GAIN 0
# elif defined(HAVE_TLV320)
#  define DEFAULT_REC_MIC_GAIN 0
#  define DEFAULT_REC_LEFT_GAIN 0
#  define DEFAULT_REC_RIGHT_GAIN 0
# elif defined(HAVE_WM8975)
#  define DEFAULT_REC_MIC_GAIN 16
#  define DEFAULT_REC_LEFT_GAIN 0
#  define DEFAULT_REC_RIGHT_GAIN 0
# elif defined(HAVE_WM8758)
#  define DEFAULT_REC_MIC_GAIN 16
#  define DEFAULT_REC_LEFT_GAIN 0
#  define DEFAULT_REC_RIGHT_GAIN 0
# elif defined(HAVE_WM8731)
#  define DEFAULT_REC_MIC_GAIN 16
#  define DEFAULT_REC_LEFT_GAIN 0
#  define DEFAULT_REC_RIGHT_GAIN 0
# endif
#endif

#endif /* HAVE_RECORDING */

#if defined(CONFIG_BACKLIGHT)
static const char backlight_times_conf [] =
                  "off,on,1,2,3,4,5,6,7,8,9,10,15,20,25,30,45,60,90";
#endif


#define GS(a) &global_settings.a

#define NVRAM(bytes) (bytes<<F_NVRAM_MASK_SHIFT)
/** NOTE: NVRAM_CONFIG_VERSION is in settings_list.h
     and you may need to update it if you edit this file */

#define UNUSED {.RESERVED=NULL}
#define INT(a) {.int_ = a}
#define UINT(a) {.uint_ = a}
#define BOOL(a) {.bool_ = a}
#define CHARPTR(a) {.charptr = a}
#define UCHARPTR(a) {.ucharptr = a}
#define NODEFAULT INT(0)

#define SOUND_SETTING(flags,var,setting) \
        {flags|F_T_INT|F_T_SOUND, GS(var), NODEFAULT,#var,NULL,\
            {.sound_setting=(struct sound_setting[]){{setting}}} }

#define BOOL_SETTING(flags,var,default,name,cfgvals,yes,no,opt_cb) \
        {flags|F_T_BOOL, GS(var), BOOL(default),name,cfgvals,  \
            {.bool_setting=(struct bool_setting[]){{opt_cb,yes,no}}} }

#define OFFON_SETTING(flags,var,default,name,cb) \
        {flags|F_T_BOOL, GS(var), BOOL(default),name,off_on,  \
            {.bool_setting=(struct bool_setting[])             \
            {{cb,LANG_SET_BOOL_YES,LANG_SET_BOOL_NO}}} }

#define SYSTEM_SETTING(flags,var,default) \
        {flags|F_T_INT, GS(var), INT(default), NULL, NULL, UNUSED}
        
#define FILENAME_SETTING(flags,var,name,default,prefix,suffix,len) \
        {flags|F_T_UCHARPTR, GS(var), CHARPTR(default),name,NULL,\
            {.filename_setting=(struct filename_setting[]){{prefix,suffix,len}}} }
const struct settings_list settings[] = {
    /* sound settings */
    SOUND_SETTING(0,volume,SOUND_VOLUME),
    SOUND_SETTING(0,balance,SOUND_BALANCE),
    SOUND_SETTING(0,bass,SOUND_BASS),
    SOUND_SETTING(0,treble,SOUND_TREBLE),
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    { F_T_INT, GS(loudness), INT(0), "loudness", NULL, UNUSED },
    { F_T_INT, GS(avc), INT(0), "auto volume", "off,20ms,2,4,8", UNUSED },
    OFFON_SETTING(0,superbass,false,"superbass",NULL),
#endif
    { F_T_INT, GS(channel_config), INT(0), "channels",
         "stereo,mono,custom,mono left,mono right,karaoke", UNUSED },
    { F_T_INT, GS(stereo_width), INT(100), "stereo width", NULL, UNUSED },
    /* playback */
    OFFON_SETTING(0,resume,false,"resume", NULL),
    OFFON_SETTING(0,playlist_shuffle,false,"shuffle", NULL),
    SYSTEM_SETTING(NVRAM(4),resume_index,-1),
    SYSTEM_SETTING(NVRAM(4),resume_first_index,0),
    SYSTEM_SETTING(NVRAM(4),resume_offset,-1),
    SYSTEM_SETTING(NVRAM(4),resume_seed,-1),
    {F_T_INT, GS(repeat_mode), INT(REPEAT_ALL), "repeat",
         "off,all,one,shuffle,ab" , UNUSED},
    /* LCD */
#ifdef HAVE_LCD_CONTRAST
    {F_T_INT, GS(contrast), INT(DEFAULT_CONTRAST_SETTING),
         "contrast", NULL , UNUSED},
#endif
#ifdef CONFIG_BACKLIGHT
    {F_T_INT, GS(backlight_timeout), INT(6),
        "backlight timeout",backlight_times_conf , UNUSED},
#ifdef CONFIG_CHARGING
    {F_T_INT, GS(backlight_timeout_plugged), INT(11),
        "backlight timeout plugged",backlight_times_conf , UNUSED},
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_LCD_BITMAP
    OFFON_SETTING(0,invert,false,"invert", NULL),
    OFFON_SETTING(0,flip_display,false,"flip display", NULL),
    /* display */
    OFFON_SETTING(0,invert_cursor,true,"invert cursor", NULL),
    OFFON_SETTING(0,statusbar,true,"statusbar", NULL),
    OFFON_SETTING(0,scrollbar,true,"scrollbar", NULL),
#if CONFIG_KEYPAD == RECORDER_PAD
    OFFON_SETTING(0,buttonbar,true,"buttonbar", NULL),
#endif
    {F_T_INT,GS(volume_type),INT(0),"volume display",graphic_numeric,UNUSED},
    {F_T_INT,GS(battery_display),INT(0),"battery display",graphic_numeric,UNUSED},
    {F_T_INT,GS(timeformat),INT(0),"time format","24hour,12hour",UNUSED},
#endif /* HAVE_LCD_BITMAP */
    OFFON_SETTING(0,show_icons,true,"show icons", NULL),
    /* system */
    {F_T_INT,GS(poweroff),INT(10),"idle poweroff",
        "off,1,2,3,4,5,6,7,8,9,10,15,30,45,60",UNUSED},
    SYSTEM_SETTING(NVRAM(4),runtime,0),
    SYSTEM_SETTING(NVRAM(4),topruntime,0),
#if MEM > 1
    {F_T_INT,GS(max_files_in_playlist),INT(10000),
        "max files in playlist",NULL,UNUSED},
    {F_T_INT,GS(max_files_in_dir),INT(400),
        "max files in dir",NULL,UNUSED},
#else
    {F_T_INT,GS(max_files_in_playlist),INT(1000),
        "max files in playlist",NULL,UNUSED},
    {F_T_INT,GS(max_files_in_dir),INT(200),
        "max files in dir",NULL,UNUSED},
#endif
    {F_T_INT,GS(battery_capacity),INT(BATTERY_CAPACITY_DEFAULT),
        "battery capacity",NULL,UNUSED},
#ifdef CONFIG_CHARGING
    OFFON_SETTING(0,car_adapter_mode,false,"car adapter mode", NULL),
#endif
    /* tuner */
#ifdef CONFIG_TUNER
    OFFON_SETTING(0,fm_force_mono,false,"force fm mono", NULL),
    SYSTEM_SETTING(NVRAM(4),last_frequency,0),
#endif

#if BATTERY_TYPES_COUNT > 1
    {F_T_INT,GS(battery_type),INT(0),
        "battery type","alkaline,nimh",UNUSED},
#endif
#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    {F_T_INT,GS(remote_contrast),INT(DEFAULT_REMOTE_CONTRAST_SETTING),
        "remote contrast",NULL,UNUSED},
    OFFON_SETTING(0,remote_invert,false,"remote invert", NULL),
    OFFON_SETTING(0,remote_flip_display,false,"remote flip display", NULL),
    {F_T_INT,GS(remote_backlight_timeout),INT(6),
        "remote backlight timeout",backlight_times_conf,UNUSED},
#ifdef CONFIG_CHARGING
    {F_T_INT,GS(remote_backlight_timeout_plugged),INT(11),
        "remote backlight timeout plugged",backlight_times_conf,UNUSED},
#endif
#ifdef HAVE_REMOTE_LCD_TICKING
    OFFON_SETTING(0,remote_reduce_ticking,false,"remote reduce ticking", NULL),
#endif
#endif

#ifdef CONFIG_BACKLIGHT
    OFFON_SETTING(0,bl_filter_first_keypress,false,
        "backlight filters first keypress", NULL),
#ifdef HAVE_REMOTE_LCD
    OFFON_SETTING(0,remote_bl_filter_first_keypress,false,
        "backlight filters first remote keypress", NULL),
#endif
#endif /* CONFIG_BACKLIGHT */

/** End of old RTC config block **/

#ifdef CONFIG_BACKLIGHT
    OFFON_SETTING(0,caption_backlight,false,"caption backlight",NULL),
#ifdef HAVE_REMOTE_LCD
    OFFON_SETTING(0,remote_caption_backlight,false,"remote caption backlight",NULL),
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    {F_T_INT,GS(brightness), INT(DEFAULT_BRIGHTNESS_SETTING), "brightness", NULL ,UNUSED},
#endif
#ifdef HAVE_BACKLIGHT_PWM_FADING
    /* backlight fading */
    {F_T_INT,GS(backlight_fade_in),INT(1),
        "backlight fade in","off,500ms,1s,2s",UNUSED},
    {F_T_INT,GS(backlight_fade_out),INT(1),
        "backlight fade out","off,500ms,1s,2s,3s,4s,5s,10s",UNUSED},
#endif
    {F_T_INT,GS(scroll_speed),INT(9),"scroll speed",NULL,UNUSED},
    {F_T_INT,GS(scroll_delay),INT(100),"scroll delay",NULL,UNUSED},
    {F_T_INT,GS(bidir_limit),INT(50),"bidir limit",NULL,UNUSED},
#ifdef HAVE_REMOTE_LCD
    {F_T_INT,GS(remote_scroll_speed),INT(9),"remote scroll speed",NULL,UNUSED},
    {F_T_INT,GS(remote_scroll_step),INT(6),"remote scroll step",NULL,UNUSED},
    {F_T_INT,GS(remote_scroll_delay),INT(100),"remote scroll delay",NULL,UNUSED},
    {F_T_INT,GS(remote_bidir_limit),INT(50),"remote bidir limit",NULL,UNUSED},
#endif
#ifdef HAVE_LCD_BITMAP
    OFFON_SETTING(0,offset_out_of_view,false,"Screen Scrolls Out Of View",NULL),
    {F_T_INT,GS(scroll_step),INT(6),"scroll step",NULL,UNUSED},
    {F_T_INT,GS(screen_scroll_step),INT(16),"screen scroll step",NULL,UNUSED},
#endif /* HAVE_LCD_BITMAP */
    OFFON_SETTING(0,scroll_paginated,false,"scroll paginated",NULL),
#ifdef HAVE_LCD_COLOR
    {F_T_INT|F_RGB,GS(fg_color),INT(LCD_DEFAULT_FG),"foreground color",NULL,UNUSED},
    {F_T_INT|F_RGB,GS(bg_color),INT(LCD_DEFAULT_BG),"background color",NULL,UNUSED},
#endif
    /* more playback */
    OFFON_SETTING(0,play_selected,true,"play selected",NULL),
    OFFON_SETTING(0,fade_on_stop,true,"volume fade",NULL),
    {F_T_INT,GS(ff_rewind_min_step),INT(FF_REWIND_1000),
        "scan min step","1,2,3,4,5,6,8,10,15,20,25,30,45,60",UNUSED},
    {F_T_INT,GS(ff_rewind_accel),INT(3),"scan accel",NULL,UNUSED},
#if CONFIG_CODEC == SWCODEC
    {F_T_INT,GS(buffer_margin),INT(0),"antiskip",
        "5s,15s,30s,1min,2min,3min,5min,10min",UNUSED},
#else
    {F_T_INT,GS(buffer_margin),INT(0),"antiskip",NULL,UNUSED},
#endif
    /* disk */
#ifndef HAVE_MMC
#ifdef HAVE_ATA_POWER_OFF
    OFFON_SETTING(0,disk_poweroff,false,"disk poweroff",NULL),
#endif
    {F_T_INT,GS(disk_spindown),INT(5),"disk spindown",NULL,UNUSED},
#endif /* HAVE_MMC */
    /* browser */
    {F_T_INT,GS(dirfilter),INT(SHOW_SUPPORTED),"show files",
        "all,supported,music,playlists"
#ifdef HAVE_TAGCACHE
        ",id3 database"
#endif
       ,UNUSED},
    OFFON_SETTING(0,sort_case,false,"sort case",NULL),
    OFFON_SETTING(0,browse_current,false,"follow playlist",NULL),
    OFFON_SETTING(0,playlist_viewer_icons,true,
        "playlist viewer icons",NULL),
    OFFON_SETTING(0,playlist_viewer_indices,true,
        "playlist viewer indices",NULL),
    {F_T_INT,GS(recursive_dir_insert),INT(RECURSE_OFF),
        "recursive directory insert",off_on_ask,UNUSED},
    /* bookmarks */
    {F_T_INT,GS(autocreatebookmark),INT(BOOKMARK_NO),"autocreate bookmarks",
        "off,on,ask,recent only - on,recent only - ask",UNUSED},
    {F_T_INT,GS(autoloadbookmark),INT(BOOKMARK_NO),
        "autoload bookmarks",off_on_ask,UNUSED},
    {F_T_INT,GS(usemrb),INT(BOOKMARK_NO),
        "use most-recent-bookmarks","off,on,unique only",UNUSED},
#ifdef HAVE_LCD_BITMAP
    /* peak meter */
    {F_T_INT,GS(peak_meter_clip_hold),INT(16),"peak meter clip hold",
        "on,1,2,3,4,5,6,7,8,9,10,15,20,25,30,45,60,90,2min"
        ",3min,5min,10min,20min,45min,90min",UNUSED},
    {F_T_INT,GS(peak_meter_hold),INT(3),"peak meter hold",
        "off,200ms,300ms,500ms,1,2,3,4,5,6,7,8,9,10,15,20,30,1min",UNUSED},
    {F_T_INT,GS(peak_meter_release),INT(8),"peak meter release",NULL,UNUSED},
    OFFON_SETTING(0,peak_meter_dbfs,true,"peak meter dbfs",NULL),
    {F_T_INT,GS(peak_meter_min),INT(60),"peak meter min",NULL,UNUSED},
    {F_T_INT,GS(peak_meter_max),INT(0),"peak meter max",NULL,UNUSED},
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    {F_T_INT,GS(mdb_strength),INT(0),"mdb strength",NULL,UNUSED},
    {F_T_INT,GS(mdb_harmonics),INT(0),"mdb harmonics",NULL,UNUSED},
    {F_T_INT,GS(mdb_center),INT(0),"mdb center",NULL,UNUSED},
    {F_T_INT,GS(mdb_shape),INT(0),"mdb shape",NULL,UNUSED},
    OFFON_SETTING(0,mdb_enable,false,"mdb enable",NULL),
#endif
#if CONFIG_CODEC == MAS3507D
    OFFON_SETTING(0,line_in,false,"line in",NULL),
#endif
    /* voice */
    {F_T_INT,GS(talk_dir),INT(0),"talk dir",off_number_spell_hover,UNUSED},
    {F_T_INT,GS(talk_file),INT(0),"talk file",off_number_spell_hover,UNUSED},
    OFFON_SETTING(0,talk_menu,true,"talk menu",NULL),

    {F_T_INT,GS(sort_file),INT(0),"sort files","alpha,oldest,newest,type",UNUSED},
    {F_T_INT,GS(sort_dir),INT(0),"sort dirs","alpha,oldest,newest",UNUSED},
    BOOL_SETTING(0,id3_v1_first,false,"id3 tag priority","v2-v1,v1-v2",
        LANG_ID3_V2_FIRST,LANG_ID3_V1_FIRST,NULL),

#ifdef HAVE_RECORDING
    /* recording */
    OFFON_SETTING(0,recscreen_on,false,"recscreen on",NULL),
    OFFON_SETTING(0,rec_startup,false,"rec screen on startup",NULL),
    {F_T_INT,GS(rec_timesplit),INT(0),"rec timesplit",
        "off,00:05,00:10,00:15,00:30,01:00,01:14,01:20,02:00,"
        "04:00,06:00,08:00,10:00,12:00,18:00,24:00",UNUSED},
    {F_T_INT,GS(rec_sizesplit),INT(0),"rec sizesplit",
        "off,5MB,10MB,15MB,32MB,64MB,75MB,100MB,128MB,"
        "256MB,512MB,650MB,700MB,1GB,1.5GB,1.75GB",UNUSED},
    {F_T_INT,GS(rec_channels),INT(0),"rec channels","stereo,mono",UNUSED},
    {F_T_INT,GS(rec_split_type),INT(0),"rec split type","Split, Stop",UNUSED},
    {F_T_INT,GS(rec_split_method),INT(0),"rec split method","Time,Filesize",UNUSED},
    {F_T_INT,GS(rec_source),INT(0),"rec source","mic,line"
#ifdef HAVE_SPDIF_IN
        ",spdif"
#endif
#ifdef HAVE_FMRADIO_IN
        ",fmradio"
#endif
    ,UNUSED},
    {F_T_INT,GS(rec_prerecord_time),INT(0),"prerecording time",NULL,UNUSED},
    {F_T_INT,GS(rec_directory),INT(0),"rec directory",REC_BASE_DIR ",current",UNUSED},
#ifdef CONFIG_BACKLIGHT
    {F_T_INT,GS(cliplight),INT(0),"cliplight","off,main,both,remote",UNUSED},
#endif
    {F_T_INT,GS(rec_mic_gain),INT(DEFAULT_REC_MIC_GAIN),
        "rec mic gain",NULL,UNUSED},
    {F_T_INT,GS(rec_left_gain),INT(DEFAULT_REC_LEFT_GAIN),
        "rec left gain",NULL,UNUSED},
    {F_T_INT,GS(rec_right_gain),INT(DEFAULT_REC_RIGHT_GAIN),
        "rec right gain",NULL,UNUSED},
#if CONFIG_CODEC == MAS3587F
    {F_T_INT,GS(rec_frequency),INT(0),"rec frequency","44,48,32,22,24,16",UNUSED},
    {F_T_INT,GS(rec_quality),INT(5),"rec quality",NULL,UNUSED},
    OFFON_SETTING(0,rec_editable,false,"editable recordings",NULL),
#endif /* CONFIG_CODEC == MAS3587F */
#if CONFIG_CODEC == SWCODEC
    {F_T_INT,GS(rec_frequency),INT(REC_FREQ_DEFAULT),
        "rec frequency",REC_FREQ_CFG_VAL_LIST,UNUSED},
    {F_T_INT,GS(rec_format),INT(REC_FORMAT_DEFAULT),
        "rec format",REC_FORMAT_CFG_VAL_LIST,UNUSED},
    /** Encoder settings start - keep these together **/
    /* aiff_enc */
    /* (no settings yet) */
    /* mp3_enc */
    {F_T_INT,GS(mp3_enc_config.bitrate),INT(MP3_ENC_BITRATE_CFG_DEFAULT),
        "mp3_enc bitrate",MP3_ENC_BITRATE_CFG_VALUE_LIST,UNUSED},
    /* wav_enc */
    /* (no settings yet) */
    /* wavpack_enc */
    /* (no settings yet) */
    /** Encoder settings end **/
#endif /* CONFIG_CODEC == SWCODEC */
    /* values for the trigger */
    {F_T_INT,GS(rec_start_thres),INT(-35),
        "trigger start threshold",NULL,UNUSED},
    {F_T_INT,GS(rec_stop_thres),INT(-45),
        "trigger stop threshold",NULL,UNUSED},
    {F_T_INT,GS(rec_start_duration),INT(0),
        "trigger start duration",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_stop_postrec),INT(2),
        "trigger stop postrec",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_stop_gap),INT(1),
        "trigger min gap",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_trigger_mode),INT(0),
        "trigger mode","off,once,repeat",UNUSED},
#endif /* HAVE_RECORDING */

#ifdef HAVE_SPDIF_POWER
    OFFON_SETTING(0,spdif_enable,false,"spdif enable",NULL),
#endif
    {F_T_INT,GS(next_folder),INT(FOLDER_ADVANCE_OFF),
        "folder navigation","off,on,random",UNUSED},
    OFFON_SETTING(0,runtimedb,false,"gather runtime data",NULL),

#if CONFIG_CODEC == SWCODEC
    OFFON_SETTING(0,replaygain,false,"replaygain",NULL),
    {F_T_INT,GS(replaygain_type),INT(REPLAYGAIN_ALBUM),
        "replaygain type","track,album,track shuffle",UNUSED},
    OFFON_SETTING(0,replaygain_noclip,false,"replaygain noclip",NULL),
    {F_T_INT,GS(replaygain_preamp),INT(0),"replaygain preamp",NULL,UNUSED},
    {F_T_INT,GS(beep),INT(0),"beep","off,weak,moderate,strong",UNUSED},
    {F_T_INT,GS(crossfade),INT(0),"crossfade",
        "off,shuffle,track skip,shuffle and track skip,always",UNUSED},
    {F_T_INT,GS(crossfade_fade_in_delay),INT(0),
        "crossfade fade in delay",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_delay),INT(0),
        "crossfade fade out delay",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_in_duration),INT(0),
        "crossfade fade in duration",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_duration),INT(0),
        "crossfade fade out duration",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_mixmode),INT(0),
        "crossfade fade out mode","crossfade,mix",UNUSED},
    {F_T_INT,GS(crossfade),INT(0),"crossfade",
        "off,shuffle,track skip,shuffle and track skip,always",UNUSED},
    OFFON_SETTING(0,crossfeed,false,"crossfeed",NULL),
    {F_T_INT,GS(crossfeed_direct_gain),INT(15),
        "crossfeed direct gain",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_cross_gain),INT(60),
        "crossfeed cross gain",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_hf_attenuation),INT(160),
        "crossfeed hf attenuation",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_hf_cutoff),INT(700),
        "crossfeed hf cutoff",NULL,UNUSED},

    /* equalizer */
    OFFON_SETTING(0,eq_enabled,false,"eq enabled",NULL),
    {F_T_INT,GS(eq_precut),INT(0),
        "eq precut",NULL,UNUSED},
    /* 0..32768 Hz */
    {F_T_INT,GS(eq_band0_cutoff),INT(0),
        "eq band 0 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_cutoff),INT(200),
        "eq band 1 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_cutoff),INT(800),
        "eq band 2 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_cutoff),INT(4000),
        "eq band 3 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_cutoff),INT(12000),
        "eq band 4 cutoff",NULL,UNUSED},
    /* 0..64 (or 0.0 to 6.4) */
    {F_T_INT,GS(eq_band0_q),INT(7),
        "eq band 0 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_q),INT(10),
        "eq band 1 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_q),INT(10),
        "eq band 2 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_q),INT(10),
        "eq band 3 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_q),INT(7),
        "eq band 4 q",NULL,UNUSED},
    /* -240..240 (or -24db to +24db) */
    {F_T_INT,GS(eq_band0_gain),INT(0),
        "eq band 0 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_gain),INT(0),
        "eq band 1 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_gain),INT(0),
        "eq band 2 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_gain),INT(0),
        "eq band 3 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_gain),INT(0),
        "eq band 4 gain",NULL,UNUSED},

    /* dithering */
    OFFON_SETTING(0,dithering_enabled,false,"dithering enabled",NULL),
#endif
#ifdef HAVE_DIRCACHE
    OFFON_SETTING(0,dircache,false,"dircache",NULL),
    SYSTEM_SETTING(NVRAM(4),dircache_size,0),
#endif

#ifdef HAVE_TAGCACHE
#ifdef HAVE_TC_RAMCACHE
    OFFON_SETTING(0,tagcache_ram,false,"tagcache_ram",NULL),
#endif
    OFFON_SETTING(0,tagcache_autoupdate,false,"tagcache_autoupdate",NULL),
#endif

    {F_T_INT,GS(default_codepage),INT(0),"default codepage",
        "iso8859-1,iso8859-7,iso8859-8,cp1251,iso8859-11,cp1256,"
        "iso8859-9,iso8859-2,sjis,gb2312,ksx1001,big5,utf-8,cp1256",UNUSED},

    OFFON_SETTING(0,warnon_erase_dynplaylist,false,
        "warn when erasing dynamic playlist",NULL),

#ifdef CONFIG_BACKLIGHT
#ifdef HAS_BUTTON_HOLD
    {F_T_INT,GS(backlight_on_button_hold),INT(0),
        "backlight on button hold","normal,off,on",UNUSED},
#endif

#ifdef HAVE_LCD_SLEEP
    {F_T_INT,GS(lcd_sleep_after_backlight_off),INT(3),
        "lcd sleep after backlight off",
        "always,never,5,10,15,20,30,45,60,90",UNUSED},
#endif
#endif /* CONFIG_BACKLIGHT */

#ifdef HAVE_WM8758
    OFFON_SETTING(0,eq_hw_enabled,false,
        "eq hardware enabled",NULL),

    {F_T_INT,GS(eq_hw_band0_cutoff),INT(1),
        "eq hardware band 0 cutoff",
        "80Hz,105Hz,135Hz,175Hz",UNUSED},
    {F_T_INT,GS(eq_hw_band0_gain),INT(0),
        "eq hardware band 0 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band1_center),INT(1),
        "eq hardware band 1 center",
        "230Hz,300Hz,385Hz,500Hz",UNUSED},
    {F_T_INT,GS(eq_hw_band1_bandwidth),INT(0),
        "eq hardware band 1 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band1_gain),INT(0),
        "eq hardware band 1 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band2_center),INT(1),
        "eq hardware band 2 center",
        "650Hz,850Hz,1.1kHz,1.4kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band2_bandwidth),INT(0),
        "eq hardware band 2 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band2_gain),INT(0),
        "eq hardware band 1 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band3_center),INT(1),
        "eq hardware band 3 center",
        "1.8kHz,2.4kHz,3.2kHz,4.1kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band3_bandwidth),INT(0),
        "eq hardware band 3 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band3_gain),INT(0),
        "eq hardware band 3 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band4_cutoff),INT(1),
        "eq hardware band 4 cutoff",
        "5.3kHz,6.9kHz,9kHz,11.7kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band4_gain),INT(0),
        "eq hardware band 0 gain",NULL,UNUSED},
#endif

    OFFON_SETTING(0,hold_lr_for_scroll_in_list,true,
        "hold_lr_for_scroll_in_list",NULL),
    {F_T_INT,GS(show_path_in_browser),INT(SHOW_PATH_OFF),
        "show path in browser","off,current directory,full path",UNUSED},

#ifdef HAVE_AGC
    {F_T_INT,GS(rec_agc_preset_mic),INT(1),"agc mic preset",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_preset_line),INT(1),"agc line preset",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_maxgain_mic),INT(104),
        "agc maximum mic gain",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_maxgain_line),INT(96),
        "agc maximum line gain",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_cliptime),INT(1),
        "agc cliptime","0.2s,0.4s,0.6s,0.8,1s",UNUSED},
#endif

#ifdef HAVE_REMOTE_LCD
#ifdef HAS_REMOTE_BUTTON_HOLD
    {F_T_INT,GS(remote_backlight_on_button_hold),INT(0),
        "remote backlight on button hold","normal,off,on",UNUSED},
#endif
#endif
#ifdef HAVE_HEADPHONE_DETECTION
    {F_T_INT,GS(unplug_mode),INT(0),"pause on headphone unplug",NULL,UNUSED},
    {F_T_INT,GS(unplug_rw),INT(0),"rewind duration on pause",NULL,UNUSED},
    OFFON_SETTING(0,unplug_autoresume,false,
        "disable autoresume if phones not present",NULL),
#endif
#ifdef CONFIG_TUNER
    {F_T_INT,GS(fm_region),INT(0),"fm_region","eu,us,jp,kr",UNUSED},
#endif

    OFFON_SETTING(0,audioscrobbler,false,"Last.fm Logging",NULL),

#ifdef HAVE_RECORDING
    {F_T_INT,GS(rec_trigger_type),INT(0),"trigger type","stop,pause,nf stp",UNUSED},
#endif

    /** settings not in the old config blocks **/
#ifdef CONFIG_TUNER
    FILENAME_SETTING(0,fmr_file,"fmr","",FMPRESET_PATH "/",".fmr",MAX_FILENAME+1),
#endif
    FILENAME_SETTING(0,font_file,"font","",FONT_DIR "/",".fnt",MAX_FILENAME+1),
    FILENAME_SETTING(0,wps_file, "wps","",WPS_DIR "/",".wps",MAX_FILENAME+1),
    FILENAME_SETTING(0,lang_file,"lang","",LANG_DIR "/",".lng",MAX_FILENAME+1),
#ifdef HAVE_REMOTE_LCD
    FILENAME_SETTING(0,rwps_file,"rwps","",WPS_DIR "/",".rwps",MAX_FILENAME+1),
#endif
#if LCD_DEPTH > 1
    FILENAME_SETTING(0,backdrop_file,"backdrop","",BACKDROP_DIR "/",".bmp",MAX_FILENAME+1),
#endif
#ifdef HAVE_LCD_BITMAP
    FILENAME_SETTING(0,lang_file,"kbd","",ROCKBOX_DIR "/",".kbd",MAX_FILENAME+1),
#endif

};

const int nb_settings = sizeof(settings)/sizeof(*settings);
