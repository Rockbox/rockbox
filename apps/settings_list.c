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
#define FUNCTYPE(a) {.func = a}
#define NODEFAULT INT(0)

#define SOUND_SETTING(flags,var,lang_id,setting) \
        {flags|F_T_INT|F_T_SOUND, GS(var),lang_id, NODEFAULT,#var,NULL,\
            {.sound_setting=(struct sound_setting[]){{setting}}} }

#define BOOL_SETTING(flags,var,lang_id,default,name,cfgvals,yes,no,opt_cb) \
        {flags|F_T_BOOL, GS(var),lang_id, BOOL(default),name,cfgvals,  \
            {.bool_setting=(struct bool_setting[]){{opt_cb,yes,no}}} }

#define OFFON_SETTING(flags,var,lang_id,default,name,cb) \
        {flags|F_T_BOOL, GS(var),lang_id, BOOL(default),name,off_on,  \
            {.bool_setting=(struct bool_setting[])             \
            {{cb,LANG_SET_BOOL_YES,LANG_SET_BOOL_NO}}} }

#define SYSTEM_SETTING(flags,var,default) \
        {flags|F_T_INT, &global_status.var,-1, INT(default), NULL, NULL, UNUSED}
        
#define FILENAME_SETTING(flags,var,name,default,prefix,suffix,len) \
        {flags|F_T_UCHARPTR, GS(var),-1, CHARPTR(default),name,NULL,\
            {.filename_setting=(struct filename_setting[]){{prefix,suffix,len}}} }
const struct settings_list settings[] = {
    /* sound settings */
    SOUND_SETTING(0,volume, LANG_VOLUME, SOUND_VOLUME),
    SOUND_SETTING(0,balance, LANG_BALANCE, SOUND_BALANCE),
    SOUND_SETTING(0,bass, LANG_BASS, SOUND_BASS),
    SOUND_SETTING(0,treble, LANG_TREBLE, SOUND_TREBLE),
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    { F_T_INT, GS(loudness), LANG_LOUDNESS, INT(0), "loudness", NULL, UNUSED },
    { F_T_INT, GS(avc), LANG_AUTOVOL, INT(0), "auto volume",
         "off,20ms,2,4,8", UNUSED },
    OFFON_SETTING(0,superbass,LANG_SUPERBASS,false,"superbass",NULL),
#endif
    { F_T_INT, GS(channel_config), LANG_CHANNEL, INT(0), "channels",
         "stereo,mono,custom,mono left,mono right,karaoke", UNUSED },
    { F_T_INT, GS(stereo_width),LANG_STEREO_WIDTH,
         INT(100), "stereo width", NULL, UNUSED },
    /* playback */
    OFFON_SETTING(0, resume, LANG_RESUME, false, "resume", NULL),
    OFFON_SETTING(0, playlist_shuffle, LANG_SHUFFLE, false, "shuffle", NULL),
    SYSTEM_SETTING(NVRAM(4),resume_index,-1),
    SYSTEM_SETTING(NVRAM(4),resume_first_index,0),
    SYSTEM_SETTING(NVRAM(4),resume_offset,-1),
    SYSTEM_SETTING(NVRAM(4),resume_seed,-1),
    {F_T_INT, GS(repeat_mode), LANG_REPEAT, INT(REPEAT_ALL), "repeat",
         "off,all,one,shuffle,ab" , UNUSED},
    /* LCD */
#ifdef HAVE_LCD_CONTRAST
    {F_T_INT|F_DEF_ISFUNC, GS(contrast), LANG_CONTRAST,
         FUNCTYPE(lcd_default_contrast),
         "contrast", NULL , UNUSED},
#endif
#ifdef CONFIG_BACKLIGHT
    {F_T_INT, GS(backlight_timeout), LANG_BACKLIGHT, INT(6),
        "backlight timeout",backlight_times_conf , UNUSED},
#ifdef CONFIG_CHARGING
    {F_T_INT, GS(backlight_timeout_plugged), LANG_BACKLIGHT_ON_WHEN_CHARGING,
         INT(11), "backlight timeout plugged",backlight_times_conf , UNUSED},
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_LCD_BITMAP
    OFFON_SETTING(0,invert, LANG_INVERT, false,"invert", NULL),
    OFFON_SETTING(0,flip_display, LANG_FLIP_DISPLAY, false,"flip display", NULL),
    /* display */
    OFFON_SETTING(0,invert_cursor, LANG_INVERT_CURSOR,
        true,"invert cursor", NULL),
    OFFON_SETTING(F_THEMESETTING,statusbar, LANG_STATUS_BAR, true,"statusbar", NULL),
    OFFON_SETTING(0,scrollbar, LANG_SCROLL_BAR, true,"scrollbar", NULL),
#if CONFIG_KEYPAD == RECORDER_PAD
    OFFON_SETTING(0,buttonbar, LANG_BUTTON_BAR ,true,"buttonbar", NULL),
#endif
    {F_T_INT,GS(volume_type),LANG_VOLUME_DISPLAY, INT(0),
        "volume display",graphic_numeric,UNUSED},
    {F_T_INT,GS(battery_display), LANG_BATTERY_DISPLAY, INT(0),
        "battery display",graphic_numeric,UNUSED},
    {F_T_INT,GS(timeformat), LANG_TIMEFORMAT, INT(0),
        "time format","24hour,12hour",UNUSED},
#endif /* HAVE_LCD_BITMAP */
    OFFON_SETTING(0,show_icons, LANG_SHOW_ICONS ,true,"show icons", NULL),
    /* system */
    {F_T_INT,GS(poweroff),LANG_POWEROFF_IDLE, INT(10),"idle poweroff",
        "off,1,2,3,4,5,6,7,8,9,10,15,30,45,60",UNUSED},
    SYSTEM_SETTING(NVRAM(4),runtime,0),
    SYSTEM_SETTING(NVRAM(4),topruntime,0),
#if MEM > 1
    {F_T_INT,GS(max_files_in_playlist),LANG_MAX_FILES_IN_PLAYLIST, 
        INT(10000),"max files in playlist",NULL,UNUSED},
    {F_T_INT,GS(max_files_in_dir),LANG_MAX_FILES_IN_DIR,
        INT(400),"max files in dir",NULL,UNUSED},
#else
    {F_T_INT,GS(max_files_in_playlist),LANG_MAX_FILES_IN_PLAYLIST, 
        INT(1000),"max files in playlist",NULL,UNUSED},
    {F_T_INT,GS(max_files_in_dir),LANG_MAX_FILES_IN_DIR, 
        INT(200),"max files in dir",NULL,UNUSED},
#endif
    {F_T_INT,GS(battery_capacity),LANG_BATTERY_CAPACITY, 
        INT(BATTERY_CAPACITY_DEFAULT),
        "battery capacity",NULL,UNUSED},
#ifdef CONFIG_CHARGING
    OFFON_SETTING(NVRAM(1), car_adapter_mode,
        LANG_CAR_ADAPTER_MODE,false,"car adapter mode", NULL),
#endif
    /* tuner */
#ifdef CONFIG_TUNER
    OFFON_SETTING(0,fm_force_mono, LANG_FM_MONO_MODE,
        false,"force fm mono", NULL),
    SYSTEM_SETTING(NVRAM(4),last_frequency,0),
#endif

#if BATTERY_TYPES_COUNT > 1
    {F_T_INT,GS(battery_type), LANG_BATTERY_TYPE, INT(0),
        "battery type","alkaline,nimh",UNUSED},
#endif
#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    {F_T_INT,GS(remote_contrast), LANG_CONTRAST, 
        INT(DEFAULT_REMOTE_CONTRAST_SETTING),
        "remote contrast",NULL,UNUSED},
    OFFON_SETTING(0,remote_invert, LANG_INVERT,
        false,"remote invert", NULL),
    OFFON_SETTING(0,remote_flip_display, LANG_FLIP_DISPLAY,
        false,"remote flip display", NULL),
    {F_T_INT,GS(remote_backlight_timeout), LANG_BACKLIGHT, INT(6),
        "remote backlight timeout",backlight_times_conf,UNUSED},
#ifdef CONFIG_CHARGING
    {F_T_INT,GS(remote_backlight_timeout_plugged),
        LANG_BACKLIGHT_ON_WHEN_CHARGING, INT(11),
        "remote backlight timeout plugged",backlight_times_conf,UNUSED},
#endif
#ifdef HAVE_REMOTE_LCD_TICKING
    OFFON_SETTING(0,remote_reduce_ticking, LANG_REDUCE_TICKING,
        false,"remote reduce ticking", NULL),
#endif
#endif

#ifdef CONFIG_BACKLIGHT
    OFFON_SETTING(0,bl_filter_first_keypress,
        LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS, false,
        "backlight filters first keypress", NULL),
#ifdef HAVE_REMOTE_LCD
    OFFON_SETTING(0,remote_bl_filter_first_keypress,
        LANG_BACKLIGHT_FILTER_FIRST_KEYPRESS, false,
        "backlight filters first remote keypress", NULL),
#endif
#endif /* CONFIG_BACKLIGHT */

/** End of old RTC config block **/

#ifdef CONFIG_BACKLIGHT
    OFFON_SETTING(0,caption_backlight, LANG_CAPTION_BACKLIGHT, 
        false,"caption backlight",NULL),
#ifdef HAVE_REMOTE_LCD
    OFFON_SETTING(0,remote_caption_backlight, LANG_CAPTION_BACKLIGHT, 
        false,"remote caption backlight",NULL),
#endif
#endif /* CONFIG_BACKLIGHT */
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    {F_T_INT,GS(brightness),LANG_BRIGHTNESS,
        INT(DEFAULT_BRIGHTNESS_SETTING), "brightness", NULL ,UNUSED},
#endif
#ifdef HAVE_BACKLIGHT_PWM_FADING
    /* backlight fading */
    {F_T_INT,GS(backlight_fade_in), LANG_BACKLIGHT_FADE_IN, INT(1),
        "backlight fade in","off,500ms,1s,2s",UNUSED},
    {F_T_INT,GS(backlight_fade_out), LANG_BACKLIGHT_FADE_OUT, INT(1),
        "backlight fade out","off,500ms,1s,2s,3s,4s,5s,10s",UNUSED},
#endif
    {F_T_INT,GS(scroll_speed), LANG_SCROLL_SPEED ,
        INT(9),"scroll speed",NULL,UNUSED},
    {F_T_INT,GS(scroll_delay), LANG_SCROLL_DELAY, 
        INT(100),"scroll delay",NULL,UNUSED},
    {F_T_INT,GS(bidir_limit), LANG_BIDIR_SCROLL, 
        INT(50),"bidir limit",NULL,UNUSED},
#ifdef HAVE_REMOTE_LCD
    {F_T_INT,GS(remote_scroll_speed),LANG_SCROLL_SPEED,INT(9),
        "remote scroll speed",NULL,UNUSED},
    {F_T_INT,GS(remote_scroll_step),LANG_SCROLL_STEP,INT(6),
        "remote scroll step",NULL,UNUSED},
    {F_T_INT,GS(remote_scroll_delay),LANG_SCROLL_DELAY,INT(100),
        "remote scroll delay",NULL,UNUSED},
    {F_T_INT,GS(remote_bidir_limit),LANG_BIDIR_SCROLL,INT(50),
        "remote bidir limit",NULL,UNUSED},
#endif
#ifdef HAVE_LCD_BITMAP
    OFFON_SETTING(0,offset_out_of_view,LANG_SCREEN_SCROLL_VIEW,
        false,"Screen Scrolls Out Of View",NULL),
    {F_T_INT,GS(scroll_step),LANG_SCROLL_STEP,INT(6),"scroll step",NULL,UNUSED},
    {F_T_INT,GS(screen_scroll_step),LANG_SCREEN_SCROLL_STEP,
        INT(16),"screen scroll step",NULL,UNUSED},
#endif /* HAVE_LCD_BITMAP */
#ifdef HAVE_LCD_CHARCELLS
    {F_T_INT,GS(jump_scroll),LANG_JUMP_SCROLL,INT(0),"jump scroll",NULL,UNUSED},
    {F_T_INT,GS(jump_scroll_delay),LANG_JUMP_SCROLL_DELAY,
        INT(50),"jump scroll delay",NULL,UNUSED},
#endif
    OFFON_SETTING(0,scroll_paginated,LANG_SCROLL_PAGINATED,
        false,"scroll paginated",NULL),
#ifdef HAVE_LCD_COLOR
    {F_T_INT|F_RGB|F_THEMESETTING ,GS(fg_color),-1,INT(LCD_DEFAULT_FG),
        "foreground color",NULL,UNUSED},
    {F_T_INT|F_RGB|F_THEMESETTING ,GS(bg_color),-1,INT(LCD_DEFAULT_BG),
        "background color",NULL,UNUSED},
#endif
    /* more playback */
    OFFON_SETTING(0,play_selected,LANG_PLAY_SELECTED,true,"play selected",NULL),
    OFFON_SETTING(0,party_mode,LANG_PARTY_MODE,false,"party mode",NULL),
    OFFON_SETTING(0,fade_on_stop,LANG_FADE_ON_STOP,true,"volume fade",NULL),
    {F_T_INT,GS(ff_rewind_min_step),LANG_FFRW_STEP,INT(FF_REWIND_1000),
        "scan min step","1,2,3,4,5,6,8,10,15,20,25,30,45,60",UNUSED},
    {F_T_INT,GS(ff_rewind_accel),LANG_FFRW_ACCEL,INT(3),
        "scan accel",NULL,UNUSED},
#if CONFIG_CODEC == SWCODEC
    {F_T_INT,GS(buffer_margin),LANG_MP3BUFFER_MARGIN,INT(0),"antiskip",
        "5s,15s,30s,1min,2min,3min,5min,10min",UNUSED},
#else
    {F_T_INT,GS(buffer_margin),LANG_MP3BUFFER_MARGIN,INT(0),
        "antiskip",NULL,UNUSED},
#endif
    /* disk */
#ifndef HAVE_MMC
    {F_T_INT,GS(disk_spindown),LANG_SPINDOWN,INT(5),"disk spindown",NULL,UNUSED},
#endif /* HAVE_MMC */
    /* browser */
    {F_T_INT,GS(dirfilter),LANG_FILTER,INT(SHOW_SUPPORTED),"show files",
        "all,supported,music,playlists"
#ifdef HAVE_TAGCACHE
        ",id3 database"
#endif
       ,UNUSED},
    OFFON_SETTING(0,sort_case,LANG_SORT_CASE,false,"sort case",NULL),
    OFFON_SETTING(0,browse_current,LANG_FOLLOW,false,"follow playlist",NULL),
    OFFON_SETTING(0,playlist_viewer_icons,LANG_SHOW_ICONS,true,
        "playlist viewer icons",NULL),
    OFFON_SETTING(0,playlist_viewer_indices,LANG_SHOW_INDICES,true,
        "playlist viewer indices",NULL),
    {F_T_INT,GS(playlist_viewer_track_display),LANG_TRACK_DISPLAY,
        INT(0),"playlist viewer track display","track name,full path",UNUSED},
    {F_T_INT,GS(recursive_dir_insert),LANG_RECURSE_DIRECTORY, INT(RECURSE_OFF),
        "recursive directory insert",off_on_ask,UNUSED},
    /* bookmarks */
    {F_T_INT,GS(autocreatebookmark),LANG_BOOKMARK_SETTINGS_AUTOCREATE,
        INT(BOOKMARK_NO),"autocreate bookmarks",
        "off,on,ask,recent only - on,recent only - ask",UNUSED},
    {F_T_INT,GS(autoloadbookmark),LANG_BOOKMARK_SETTINGS_AUTOLOAD,
        INT(BOOKMARK_NO), "autoload bookmarks",off_on_ask,UNUSED},
    {F_T_INT,GS(usemrb),LANG_BOOKMARK_SETTINGS_MAINTAIN_RECENT_BOOKMARKS,
        INT(BOOKMARK_NO),
        "use most-recent-bookmarks","off,on,unique only",UNUSED},
#ifdef HAVE_LCD_BITMAP
    /* peak meter */
    {F_T_INT, GS(peak_meter_clip_hold), LANG_PM_CLIP_HOLD,
        INT(16), "peak meter clip hold",
        "on,1,2,3,4,5,6,7,8,9,10,15,20,25,30,45,60,90,2min"
        ",3min,5min,10min,20min,45min,90min", UNUSED},
    {F_T_INT,GS(peak_meter_hold), LANG_PM_PEAK_HOLD, 
        INT(3),"peak meter hold",
        "off,200ms,300ms,500ms,1,2,3,4,5,6,7,8,9,10,15,20,30,1min",UNUSED},
    {F_T_INT,GS(peak_meter_release),LANG_PM_RELEASE, 
        INT(8),"peak meter release",NULL,UNUSED},
    OFFON_SETTING(0,peak_meter_dbfs,LANG_PM_DBFS,true,"peak meter dbfs",NULL),
    {F_T_INT,GS(peak_meter_min),LANG_PM_MIN,INT(60),"peak meter min",NULL,UNUSED},
    {F_T_INT,GS(peak_meter_max),LANG_PM_MAX,INT(0),"peak meter max",NULL,UNUSED},
#endif
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    {F_T_INT,GS(mdb_strength),LANG_MDB_STRENGTH,INT(0),
        "mdb strength",NULL,UNUSED},
    {F_T_INT,GS(mdb_harmonics),LANG_MDB_HARMONICS,INT(0),
        "mdb harmonics",NULL,UNUSED},
    {F_T_INT,GS(mdb_center),LANG_MDB_CENTER,INT(0),"mdb center",NULL,UNUSED},
    {F_T_INT,GS(mdb_shape),LANG_MDB_SHAPE,INT(0),"mdb shape",NULL,UNUSED},
    OFFON_SETTING(0,mdb_enable,LANG_MDB_ENABLE,false,"mdb enable",NULL),
#endif
#if CONFIG_CODEC == MAS3507D
    OFFON_SETTING(0,line_in,LANG_LINE_IN,false,"line in",NULL),
#endif
    /* voice */
    {F_T_INT,GS(talk_dir),LANG_VOICE_DIR,INT(0),
        "talk dir",off_number_spell_hover,UNUSED},
    {F_T_INT,GS(talk_file),LANG_VOICE_FILE,INT(0),
        "talk file",off_number_spell_hover,UNUSED},
    OFFON_SETTING(0,talk_menu,LANG_VOICE_MENU,true,"talk menu",NULL),

    /* file sorting */
    {F_T_INT,GS(sort_file),LANG_SORT_FILE,INT(0),
        "sort files","alpha,oldest,newest,type",UNUSED},
    {F_T_INT,GS(sort_dir),LANG_SORT_DIR,INT(0),
        "sort dirs","alpha,oldest,newest",UNUSED},
    BOOL_SETTING(0,id3_v1_first,LANG_ID3_ORDER,false,
        "id3 tag priority","v2-v1,v1-v2",
        LANG_ID3_V2_FIRST,LANG_ID3_V1_FIRST,NULL),

#ifdef HAVE_RECORDING
    /* recording */
    OFFON_SETTING(0,recscreen_on,-1,false,"recscreen on",NULL),
    OFFON_SETTING(0,rec_startup,LANG_RECORD_STARTUP,false,
        "rec screen on startup",NULL),
    {F_T_INT,GS(rec_timesplit), LANG_SPLIT_TIME, INT(0),"rec timesplit",
        "off,00:05,00:10,00:15,00:30,01:00,01:14,01:20,02:00,"
        "04:00,06:00,08:00,10:00,12:00,18:00,24:00",UNUSED},
    {F_T_INT,GS(rec_sizesplit),LANG_SPLIT_SIZE,INT(0),"rec sizesplit",
        "off,5MB,10MB,15MB,32MB,64MB,75MB,100MB,128MB,"
        "256MB,512MB,650MB,700MB,1GB,1.5GB,1.75GB",UNUSED},
    {F_T_INT,GS(rec_channels),LANG_RECORDING_CHANNELS,INT(0),
        "rec channels","stereo,mono",UNUSED},
    {F_T_INT,GS(rec_split_type),LANG_RECORDING_CHANNELS,INT(0),
        "rec split type","Split, Stop",UNUSED},
    {F_T_INT,GS(rec_split_method),LANG_SPLIT_MEASURE,INT(0),
        "rec split method","Time,Filesize",UNUSED},
    {F_T_INT,GS(rec_source),LANG_RECORDING_SOURCE,INT(0),
        "rec source","mic,line"
#ifdef HAVE_SPDIF_IN
        ",spdif"
#endif
#ifdef HAVE_FMRADIO_IN
        ",fmradio"
#endif
    ,UNUSED},
    {F_T_INT,GS(rec_prerecord_time),LANG_RECORD_PRERECORD_TIME,
        INT(0),"prerecording time",NULL,UNUSED},
    {F_T_INT,GS(rec_directory),LANG_RECORD_DIRECTORY,
        INT(0),"rec directory",REC_BASE_DIR ",current",UNUSED},
#ifdef CONFIG_BACKLIGHT
    {F_T_INT,GS(cliplight),LANG_CLIP_LIGHT,INT(0),
        "cliplight","off,main,both,remote",UNUSED},
#endif
    {F_T_INT,GS(rec_mic_gain),LANG_RECORDING_GAIN,INT(DEFAULT_REC_MIC_GAIN),
        "rec mic gain",NULL,UNUSED},
    {F_T_INT,GS(rec_left_gain),LANG_RECORDING_LEFT,INT(DEFAULT_REC_LEFT_GAIN),
        "rec left gain",NULL,UNUSED},
    {F_T_INT,GS(rec_right_gain),LANG_RECORDING_RIGHT,
        INT(DEFAULT_REC_RIGHT_GAIN),
        "rec right gain",NULL,UNUSED},
#if CONFIG_CODEC == MAS3587F
    {F_T_INT,GS(rec_frequency),LANG_RECORDING_FREQUENCY,
        INT(0),"rec frequency","44,48,32,22,24,16",UNUSED},
    {F_T_INT,GS(rec_quality),LANG_RECORDING_QUALITY,INT(5),
        "rec quality",NULL,UNUSED},
    OFFON_SETTING(0,rec_editable,LANG_RECORDING_EDITABLE,
        false,"editable recordings",NULL),
#endif /* CONFIG_CODEC == MAS3587F */
#if CONFIG_CODEC == SWCODEC
    {F_T_INT,GS(rec_frequency),LANG_RECORDING_FREQUENCY,INT(REC_FREQ_DEFAULT),
        "rec frequency",REC_FREQ_CFG_VAL_LIST,UNUSED},
    {F_T_INT,GS(rec_format),LANG_RECORDING_FORMAT,INT(REC_FORMAT_DEFAULT),
        "rec format",REC_FORMAT_CFG_VAL_LIST,UNUSED},
    /** Encoder settings start - keep these together **/
    /* aiff_enc */
    /* (no settings yet) */
    /* mp3_enc */
    {F_T_INT,GS(mp3_enc_config.bitrate),-1,INT(MP3_ENC_BITRATE_CFG_DEFAULT),
        "mp3_enc bitrate",MP3_ENC_BITRATE_CFG_VALUE_LIST,UNUSED},
    /* wav_enc */
    /* (no settings yet) */
    /* wavpack_enc */
    /* (no settings yet) */
    /** Encoder settings end **/
#endif /* CONFIG_CODEC == SWCODEC */
    /* values for the trigger */
    {F_T_INT,GS(rec_start_thres),LANG_RECORD_START_THRESHOLD,INT(-35),
        "trigger start threshold",NULL,UNUSED},
    {F_T_INT,GS(rec_stop_thres),LANG_RECORD_STOP_THRESHOLD,INT(-45),
        "trigger stop threshold",NULL,UNUSED},
    {F_T_INT,GS(rec_start_duration),LANG_RECORD_MIN_DURATION,INT(0),
        "trigger start duration",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_stop_postrec),LANG_RECORD_STOP_POSTREC,INT(2),
        "trigger stop postrec",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_stop_gap),LANG_RECORD_STOP_GAP,INT(1),
        "trigger min gap",trig_durations_conf,UNUSED},
    {F_T_INT,GS(rec_trigger_mode),LANG_RECORD_TRIGGER_MODE,INT(0),
        "trigger mode","off,once,repeat",UNUSED},
#endif /* HAVE_RECORDING */

#ifdef HAVE_SPDIF_POWER
    OFFON_SETTING(0,spdif_enable,LANG_SPDIF_ENABLE,false,"spdif enable",NULL),
#endif
    {F_T_INT,GS(next_folder),LANG_NEXT_FOLDER,INT(FOLDER_ADVANCE_OFF),
        "folder navigation","off,on,random",UNUSED},
    OFFON_SETTING(0,runtimedb,LANG_RUNTIMEDB_ACTIVE,false,"gather runtime data",NULL),

#if CONFIG_CODEC == SWCODEC
    /* replay gain */
    OFFON_SETTING(0,replaygain,LANG_REPLAYGAIN,false,"replaygain",NULL),
    {F_T_INT,GS(replaygain_type),LANG_REPLAYGAIN_MODE,INT(REPLAYGAIN_ALBUM),
        "replaygain type","track,album,track shuffle",UNUSED},
    OFFON_SETTING(0,replaygain_noclip,LANG_REPLAYGAIN_NOCLIP,
        false,"replaygain noclip",NULL),
    {F_T_INT,GS(replaygain_preamp),LANG_REPLAYGAIN_PREAMP,
        INT(0),"replaygain preamp",NULL,UNUSED},

    {F_T_INT,GS(beep),LANG_BEEP,INT(0),"beep","off,weak,moderate,strong",UNUSED},

    /* crossfade */
    {F_T_INT,GS(crossfade),LANG_CROSSFADE_ENABLE,INT(0),"crossfade",
        "off,shuffle,track skip,shuffle and track skip,always",UNUSED},
    {F_T_INT,GS(crossfade_fade_in_delay),LANG_CROSSFADE_FADE_IN_DELAY,INT(0),
        "crossfade fade in delay",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_delay),
        LANG_CROSSFADE_FADE_OUT_DELAY,INT(0),
        "crossfade fade out delay",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_in_duration),
        LANG_CROSSFADE_FADE_IN_DURATION,INT(0),
        "crossfade fade in duration",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_duration),
        LANG_CROSSFADE_FADE_OUT_DURATION,INT(0),
        "crossfade fade out duration",NULL,UNUSED},
    {F_T_INT,GS(crossfade_fade_out_mixmode),
        LANG_CROSSFADE_FADE_OUT_MODE,INT(0),
        "crossfade fade out mode","crossfade,mix",UNUSED},

    /* crossfeed */
    OFFON_SETTING(0,crossfeed,LANG_CROSSFEED,false,"crossfeed",NULL),
    {F_T_INT,GS(crossfeed_direct_gain),LANG_CROSSFEED_DIRECT_GAIN,INT(15),
        "crossfeed direct gain",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_cross_gain),LANG_CROSSFEED_CROSS_GAIN,INT(60),
        "crossfeed cross gain",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_hf_attenuation),LANG_CROSSFEED_HF_ATTENUATION,INT(160),
        "crossfeed hf attenuation",NULL,UNUSED},
    {F_T_INT,GS(crossfeed_hf_cutoff),LANG_CROSSFEED_HF_CUTOFF,INT(700),
        "crossfeed hf cutoff",NULL,UNUSED},

    /* equalizer */
    OFFON_SETTING(0,eq_enabled,LANG_EQUALIZER_ENABLED,false,"eq enabled",NULL),
    {F_T_INT,GS(eq_precut),LANG_EQUALIZER_PRECUT,INT(0),
        "eq precut",NULL,UNUSED},
    /* 0..32768 Hz */
    {F_T_INT,GS(eq_band0_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(60),
        "eq band 0 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(200),
        "eq band 1 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(800),
        "eq band 2 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(4000),
        "eq band 3 cutoff",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(12000),
        "eq band 4 cutoff",NULL,UNUSED},
    /* 0..64 (or 0.0 to 6.4) */
    {F_T_INT,GS(eq_band0_q),LANG_EQUALIZER_BAND_Q,INT(7),
        "eq band 0 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_q),LANG_EQUALIZER_BAND_Q,INT(10),
        "eq band 1 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_q),LANG_EQUALIZER_BAND_Q,INT(10),
        "eq band 2 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_q),LANG_EQUALIZER_BAND_Q,INT(10),
        "eq band 3 q",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_q),LANG_EQUALIZER_BAND_Q,INT(7),
        "eq band 4 q",NULL,UNUSED},
    /* -240..240 (or -24db to +24db) */
    {F_T_INT,GS(eq_band0_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq band 0 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band1_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq band 1 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band2_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq band 2 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band3_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq band 3 gain",NULL,UNUSED},
    {F_T_INT,GS(eq_band4_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq band 4 gain",NULL,UNUSED},

    /* dithering */
    OFFON_SETTING(0,dithering_enabled,LANG_DITHERING,
        false,"dithering enabled",NULL),
#endif
#ifdef HAVE_DIRCACHE
    OFFON_SETTING(0,dircache,LANG_DIRCACHE_ENABLE,false,"dircache",NULL),
    SYSTEM_SETTING(NVRAM(4),dircache_size,0),
#endif

#ifdef HAVE_TAGCACHE
#ifdef HAVE_TC_RAMCACHE
    OFFON_SETTING(0,tagcache_ram,LANG_TAGCACHE_RAM,false,"tagcache_ram",NULL),
#endif
    OFFON_SETTING(0,tagcache_autoupdate,
        LANG_TAGCACHE_AUTOUPDATE,false,"tagcache_autoupdate",NULL),
#endif

    {F_T_INT,GS(default_codepage),LANG_DEFAULT_CODEPAGE,
        INT(0),"default codepage",
        "iso8859-1,iso8859-7,iso8859-8,cp1251,iso8859-11,cp1256,"
        "iso8859-9,iso8859-2,sjis,gb2312,ksx1001,big5,utf-8,cp1256",UNUSED},

    OFFON_SETTING(0,warnon_erase_dynplaylist,
        LANG_WARN_ERASEDYNPLAYLIST_MENU,false,
        "warn when erasing dynamic playlist",NULL),

#ifdef CONFIG_BACKLIGHT
#ifdef HAS_BUTTON_HOLD
    {F_T_INT,GS(backlight_on_button_hold),LANG_BACKLIGHT_ON_BUTTON_HOLD,INT(0),
        "backlight on button hold","normal,off,on",UNUSED},
#endif

#ifdef HAVE_LCD_SLEEP
    {F_T_INT,GS(lcd_sleep_after_backlight_off),
        LANG_LCD_SLEEP_AFTER_BACKLIGHT_OFF,INT(3),
        "lcd sleep after backlight off",
        "always,never,5,10,15,20,30,45,60,90",UNUSED},
#endif
#endif /* CONFIG_BACKLIGHT */

#ifdef HAVE_WM8758
    OFFON_SETTING(0,eq_hw_enabled,LANG_EQUALIZER_HARDWARE_ENABLED,false,
        "eq hardware enabled",NULL),

    {F_T_INT,GS(eq_hw_band0_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(1),
        "eq hardware band 0 cutoff",
        "80Hz,105Hz,135Hz,175Hz",UNUSED},
    {F_T_INT,GS(eq_hw_band0_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq hardware band 0 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band1_center),LANG_EQUALIZER_BAND_CENTER,INT(1),
        "eq hardware band 1 center",
        "230Hz,300Hz,385Hz,500Hz",UNUSED},
    {F_T_INT,GS(eq_hw_band1_bandwidth),LANG_EQUALIZER_BANDWIDTH,INT(0),
        "eq hardware band 1 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band1_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq hardware band 1 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band2_center),LANG_EQUALIZER_BAND_CENTER,INT(1),
        "eq hardware band 2 center",
        "650Hz,850Hz,1.1kHz,1.4kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band2_bandwidth),LANG_EQUALIZER_BANDWIDTH,INT(0),
        "eq hardware band 2 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band2_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq hardware band 2 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band3_center),LANG_EQUALIZER_BAND_CENTER,INT(1),
        "eq hardware band 3 center",
        "1.8kHz,2.4kHz,3.2kHz,4.1kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band3_bandwidth),LANG_EQUALIZER_BANDWIDTH,INT(0),
        "eq hardware band 3 bandwidth","narrow,wide",UNUSED},
    {F_T_INT,GS(eq_hw_band3_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq hardware band 3 gain",NULL,UNUSED},

    {F_T_INT,GS(eq_hw_band4_cutoff),LANG_EQUALIZER_BAND_CUTOFF,INT(1),
        "eq hardware band 4 cutoff",
        "5.3kHz,6.9kHz,9kHz,11.7kHz",UNUSED},
    {F_T_INT,GS(eq_hw_band4_gain),LANG_EQUALIZER_BAND_GAIN,INT(0),
        "eq hardware band 4 gain",NULL,UNUSED},
#endif

    OFFON_SETTING(0,hold_lr_for_scroll_in_list,-1,true,
        "hold_lr_for_scroll_in_list",NULL),
    {F_T_INT,GS(show_path_in_browser),LANG_SHOW_PATH,INT(SHOW_PATH_OFF),
        "show path in browser","off,current directory,full path",UNUSED},

#ifdef HAVE_AGC
    {F_T_INT,GS(rec_agc_preset_mic),LANG_RECORD_AGC_PRESET,INT(1),
        "agc mic preset",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_preset_line),LANG_RECORD_AGC_PRESET,INT(1),
        "agc line preset",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_maxgain_mic),-1,INT(104),
        "agc maximum mic gain",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_maxgain_line),-1,INT(96),
        "agc maximum line gain",NULL,UNUSED},
    {F_T_INT,GS(rec_agc_cliptime),LANG_RECORD_AGC_CLIPTIME,INT(1),
        "agc cliptime","0.2s,0.4s,0.6s,0.8,1s",UNUSED},
#endif

#ifdef HAVE_REMOTE_LCD
#ifdef HAS_REMOTE_BUTTON_HOLD
    {F_T_INT,GS(remote_backlight_on_button_hold),
        LANG_BACKLIGHT_ON_BUTTON_HOLD,INT(0),
        "remote backlight on button hold","normal,off,on",UNUSED},
#endif
#endif
#ifdef HAVE_HEADPHONE_DETECTION
    {F_T_INT,GS(unplug_mode),LANG_UNPLUG,INT(0),
        "pause on headphone unplug",NULL,UNUSED},
    {F_T_INT,GS(unplug_rw),LANG_UNPLUG_RW,INT(0),
        "rewind duration on pause",NULL,UNUSED},
    OFFON_SETTING(0,unplug_autoresume,LANG_UNPLUG_DISABLE_AUTORESUME,false,
        "disable autoresume if phones not present",NULL),
#endif
#ifdef CONFIG_TUNER
    {F_T_INT,GS(fm_region),LANG_FM_REGION,INT(0),
        "fm_region","eu,us,jp,kr",UNUSED},
#endif

    OFFON_SETTING(0,audioscrobbler,LANG_AUDIOSCROBBLER,
        false,"Last.fm Logging",NULL),

#ifdef HAVE_RECORDING
    {F_T_INT,GS(rec_trigger_type),LANG_RECORD_TRIGGER_TYPE,
        INT(0),"trigger type","stop,pause,nf stp",UNUSED},
#endif

    /** settings not in the old config blocks **/
#ifdef CONFIG_TUNER
    FILENAME_SETTING(0, fmr_file, "fmr",
		"", FMPRESET_PATH "/", ".fmr", MAX_FILENAME+1),
#endif
    FILENAME_SETTING(F_THEMESETTING, font_file, "font",
		"", FONT_DIR "/", ".fnt", MAX_FILENAME+1),
    FILENAME_SETTING(F_THEMESETTING,wps_file, "wps",
		"", WPS_DIR "/", ".wps", MAX_FILENAME+1),
    FILENAME_SETTING(0,lang_file,"lang","",LANG_DIR "/",".lng",MAX_FILENAME+1),
#ifdef HAVE_REMOTE_LCD
    FILENAME_SETTING(F_THEMESETTING,rwps_file,"rwps",
		"", WPS_DIR "/", ".rwps", MAX_FILENAME+1),
#endif
#if LCD_DEPTH > 1
    FILENAME_SETTING(F_THEMESETTING,backdrop_file,"backdrop",
		"", BACKDROP_DIR "/", ".bmp", MAX_FILENAME+1),
#endif
#ifdef HAVE_LCD_BITMAP
    FILENAME_SETTING(0,kbd_file,"kbd","",ROCKBOX_DIR "/",".kbd",MAX_FILENAME+1),
#endif
#ifdef HAVE_USB_POWER
#ifdef CONFIG_CHARGING
    OFFON_SETTING(0,usb_charging,LANG_USB_CHARGING,false,"usb charging",NULL),
#endif
#endif
};

const int nb_settings = sizeof(settings)/sizeof(*settings);
