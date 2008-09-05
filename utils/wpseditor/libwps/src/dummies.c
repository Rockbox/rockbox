#include <string.h>
#include <stdio.h>
#include "dummies.h"
#include "proxy.h"

struct user_settings global_settings;

struct wps_state wps_state;
struct gui_wps gui_wps[NB_SCREENS];
struct wps_data wps_datas[NB_SCREENS];
struct cuesheet *curr_cue;
struct cuesheet *temp_cue;
struct system_status global_status;
struct gui_syncstatusbar statusbars;
struct playlist_info current_playlist;
struct font sysfont;
int battery_percent = 100;
struct mp3entry current_song, next_song;
int _audio_status;

charger_input_state_type charger_input_state;
#if CONFIG_CHARGING >= CHARGING_MONITOR
charge_state_type charge_state;
#endif

#if defined(CPU_PP) && defined(BOOTLOADER)
/* We don't enable interrupts in the iPod bootloader, so we need to fake
the current_tick variable */
#define current_tick (signed)(USEC_TIMER/10000)
#else
volatile long current_tick;
#endif


void dummies_init(){
    sysfont.height = 9;
    sysfont.maxwidth = 6;
    global_settings.statusbar=true;
}

int playlist_amount_ex(const struct playlist_info* playlist);
void sound_set_volume(int value)
{
    DEBUGF3("sound_set_volume(int value=%d)",value);
    global_settings.volume = value;
}
int sound_get_pitch(void)
{
    return 0;
}
int sound_min(int setting)
{
    DEBUGF3("sound_min(int setting=%d)",setting);
    return -78; //audiohw_settings[setting].minval;
}

void sleep(int hz)
{
}

void audio_init(void){}
void audio_wait_for_init(void){}
void audio_play(long offset){}
void audio_stop(void){}
void audio_pause(void){}
void audio_resume(void){}
void audio_next(void){}
void audio_prev(void){}
int audio_status(void)
{
    return _audio_status;
}

#if CONFIG_CODEC == SWCODEC
int audio_track_count(void){return 0;} /* SWCODEC only */
long audio_filebufused(void){return 0;} /* SWCODEC only */
void audio_pre_ff_rewind(void){} /* SWCODEC only */
#endif /* CONFIG_CODEC == SWCODEC */
void audio_ff_rewind(long newtime){}
void audio_flush_and_reload_tracks(void){}
#ifdef HAVE_ALBUMART
int audio_current_aa_hid(void){return -1;}
#endif
struct mp3entry* audio_current_track(void){return 0;}
struct mp3entry* audio_next_track(void){return 0;}
bool audio_has_changed_track(void)
{
    return false;
}

int get_sleep_timer(void){return 0;}


int battery_level(void){return battery_percent;} /* percent */
int battery_time(void){return 0;} /* minutes */
unsigned int battery_adc_voltage(void){return 0;} /* voltage from ADC in millivolts */
unsigned int battery_voltage(void){return 0;} /* filtered batt. voltage in millivolts */
int  get_radio_status(void){return 0;}


/* returns full path of playlist (minus extension) */
char *playlist_name(const struct playlist_info* playlist, char *buf,
int buf_size)
{
    char *sep;

    if (!playlist)
    return "no";

    snprintf(buf, buf_size, "%s", playlist->filename+playlist->dirlen);

    if (!buf[0])
    return NULL;

    /* Remove extension */
    sep = strrchr(buf, '.');
    if(sep)
        *sep = 0;

    return buf;
}
int playlist_get_display_index(void)
{
    return 1;
}

void gui_syncsplash(int ticks, const unsigned char *fmt, ...)
{

}

void splash(int ticks, const unsigned char *fmt, ...)
{

}

void gui_statusbar_draw(struct gui_statusbar * bar, bool force_redraw){
    DEBUGF3("gui_statusbar_draw");
}

void yield(void){}


/* returns true if cuesheet support is initialised */
bool cuesheet_is_enabled(void){return false;}

/* allocates the cuesheet buffer */
void cuesheet_init(void){}

/* looks if there is a cuesheet file that has a name matching "trackpath" */
bool look_for_cuesheet_file(const char *trackpath, char *found_cue_path){return false;}

/* parse cuesheet "file" and store the information in "cue" */
bool parse_cuesheet(char *file, struct cuesheet *cue){return false;}

/* reads a cuesheet to find the audio track associated to it */
bool get_trackname_from_cuesheet(char *filename, char *buf){return false;}

/* display a cuesheet struct */
void browse_cuesheet(struct cuesheet *cue){}

/* display a cuesheet file after parsing and loading it to the plugin buffer */
bool display_cuesheet_content(char* filename){return false;}

/* finds the index of the current track played within a cuesheet */
int cue_find_current_track(struct cuesheet *cue, unsigned long curpos){return 0;}

/* update the id3 info to that of the currently playing track in the cuesheet */
void cue_spoof_id3(struct cuesheet *cue, struct mp3entry *id3){}

/* skip to next track in the cuesheet towards "direction" (which is 1 or -1) */
bool curr_cuesheet_skip(int direction, unsigned long curr_pos){return false;}

#ifdef HAVE_LCD_BITMAP
/* draw track markers on the progressbar */
void cue_draw_markers(struct screen *screen, unsigned long tracklen,
int x1, int x2, int y, int h){}
#endif

#ifdef HAVE_ALBUMART
void draw_album_art(struct gui_wps *gwps, int handle_id, bool clear)
{
    if (!gwps || !gwps->data || !gwps->display || handle_id < 0)
    return;

    struct wps_data *data = gwps->data;

#ifdef HAVE_REMOTE_LCD
    /* No album art on RWPS */
    if (data->remote_wps)
    return;
#endif

    struct bitmap *bmp;
    /* if (bufgetdata(handle_id, 0, (void *)&bmp) <= 0)
        return;*/

    short x = data->albumart_x;
    short y = data->albumart_y;
    short width = bmp->width;
    short height = bmp->height;

    if (data->albumart_max_width > 0)
    {
        /* Crop if the bitmap is too wide */
        width = MIN(bmp->width, data->albumart_max_width);

        /* Align */
        if (data->albumart_xalign & WPS_ALBUMART_ALIGN_RIGHT)
        x += data->albumart_max_width - width;
        else if (data->albumart_xalign & WPS_ALBUMART_ALIGN_CENTER)
        x += (data->albumart_max_width - width) / 2;
    }

    if (data->albumart_max_height > 0)
    {
        /* Crop if the bitmap is too high */
        height = MIN(bmp->height, data->albumart_max_height);

        /* Align */
        if (data->albumart_yalign & WPS_ALBUMART_ALIGN_BOTTOM)
        y += data->albumart_max_height - height;
        else if (data->albumart_yalign & WPS_ALBUMART_ALIGN_CENTER)
        y += (data->albumart_max_height - height) / 2;
    }

    if (!clear)
    {
        /* Draw the bitmap */
        gwps->display->set_drawmode(DRMODE_FG);
        gwps->display->bitmap_part((fb_data*)bmp->data, 0, 0, bmp->width,
        x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
    else
    {
        /* Clear the bitmap */
        gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        gwps->display->fillrect(x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
}
#endif
/* Update the "data" pointer to make the handle's data available to the caller.
Return the length of the available linear data or < 0 for failure (handle
not found).
The caller is blocked until the requested amount of data is available.
size is the amount of linear data requested. it can be 0 to get as
much as possible.
The guard buffer may be used to provide the requested size. This means it's
unsafe to request more than the size of the guard buffer.
*/
size_t bufgetdata(int handle_id, size_t size, void **data)
{


    return size;
}


void gui_syncstatusbar_draw(struct gui_syncstatusbar * bars,
bool force_redraw)
{
#ifdef HAVE_LCD_BITMAP
    if(!global_settings.statusbar)
    return;
#endif /* HAVE_LCD_BITMAP */
    int i;
    FOR_NB_SCREENS(i) {
        gui_statusbar_draw( &(bars->statusbars[i]), force_redraw );
    }
}
void unload_wps_backdrop(void)
{

}
void unload_remote_wps_backdrop(void)
{

}

#if CONFIG_CODEC == SWCODEC
int get_replaygain_mode(bool have_track_gain, bool have_album_gain)
{
    int type;

    bool track = ((global_settings.replaygain_type == REPLAYGAIN_TRACK)
    || ((global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
    && global_settings.playlist_shuffle));

    type = (!track && have_album_gain) ? REPLAYGAIN_ALBUM
    : have_track_gain ? REPLAYGAIN_TRACK : -1;

    return type;
}
#endif

/* Common functions for all targets */
void rtc_init(void){}
int rtc_read_datetime(unsigned char* buf){return 0;}
int rtc_write_datetime(unsigned char* buf){return 0;}

void backlight_on(void){}
void backlight_off(void){}

void remote_backlight_on(void){}
void remote_backlight_off(void){}

void debugf(const char *fmt, ...)
{}
void panicf( const char *fmt, ...)
{
}

off_t filesize(int fd){return 0;}

int playlist_amount(void)
{
    return playlist_amount_ex(NULL);
}
int playlist_amount_ex(const struct playlist_info* playlist)
{
    if (!playlist)
        playlist = &current_playlist;

    return playlist->amount;
}

int get_action(int context, int timeout)
{
    return 0;
}

void lcd_mono_bitmap(const unsigned char *src, int x, int y, int width,
                            int height){}

void pcm_calculate_rec_peaks(int *left, int *right)
{
}
void pcm_calculate_peaks(int *left, int *right)
{
}
bool led_read(int delayticks) /* read by status bar update */
{
    return false;
}

#ifndef HAS_BUTTON_HOLD
bool is_keys_locked(void)
{
    return false;
}
#endif

long default_event_handler_ex(long event, void (*callback)(void *), void *parameter)
{
     return 0;
}

long default_event_handler(long event)
{
    return default_event_handler_ex(event, NULL, NULL);
}

void ab_draw_markers(struct screen * screen, int capacity,
                     int x0, int x1, int y, int h)
                     {
                     }
void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude){}
