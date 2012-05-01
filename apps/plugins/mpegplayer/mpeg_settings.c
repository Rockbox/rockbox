#include "plugin.h"
#include "lib/helper.h"
#include "lib/configfile.h"

#include "mpegplayer.h"
#include "mpeg_settings.h"

struct mpeg_settings settings;

#define THUMB_DELAY (75*HZ/100)

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_ON
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_OFF

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_SCROLL_FWD
#define MPEG_START_TIME_DOWN        BUTTON_SCROLL_BACK
#define MPEG_START_TIME_EXIT        BUTTON_MENU

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#define MPEG_START_TIME_RC_SELECT   (BUTTON_RC_PLAY | BUTTON_REL)
#define MPEG_START_TIME_RC_LEFT     BUTTON_RC_REW
#define MPEG_START_TIME_RC_RIGHT    BUTTON_RC_FF
#define MPEG_START_TIME_RC_UP       BUTTON_RC_VOL_UP
#define MPEG_START_TIME_RC_DOWN     BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_RC_EXIT     (BUTTON_RC_PLAY | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#define MPEG_START_TIME_RC_SELECT   (BUTTON_RC_PLAY | BUTTON_REL)
#define MPEG_START_TIME_RC_LEFT     BUTTON_RC_REW
#define MPEG_START_TIME_RC_RIGHT    BUTTON_RC_FF
#define MPEG_START_TIME_RC_UP       BUTTON_RC_VOL_UP
#define MPEG_START_TIME_RC_DOWN     BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_RC_EXIT     (BUTTON_RC_PLAY | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_SCROLL_UP
#define MPEG_START_TIME_DOWN        BUTTON_SCROLL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_SCROLL_BACK
#define MPEG_START_TIME_RIGHT2      BUTTON_SCROLL_FWD
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_SCROLL_BACK
#define MPEG_START_TIME_RIGHT2      BUTTON_SCROLL_FWD
#define MPEG_START_TIME_EXIT        (BUTTON_HOME|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_START_TIME_SELECT      BUTTON_RC_HEART
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_RC_PLAY
#define MPEG_START_TIME_DOWN        BUTTON_RC_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_RC_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE100_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_PLAY
#define MPEG_START_TIME_RIGHT2      BUTTON_MENU
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MPEG_START_TIME_SELECT      BUTTON_RC_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_RC_REW
#define MPEG_START_TIME_RIGHT       BUTTON_RC_FF
#define MPEG_START_TIME_UP          BUTTON_RC_VOL_UP
#define MPEG_START_TIME_DOWN        BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_RC_REC

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define MPEG_START_TIME_SELECT      BUTTON_MENU
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_STOP
#define MPEG_START_TIME_DOWN        BUTTON_PLAY
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_PLAY
#define MPEG_START_TIME_RIGHT2      BUTTON_MENU
#define MPEG_START_TIME_EXIT        BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_PREV
#define MPEG_START_TIME_RIGHT       BUTTON_NEXT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_VOL_UP
#define MPEG_START_TIME_RIGHT2      BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_REW
#define MPEG_START_TIME_RIGHT2      BUTTON_FFWD
#define MPEG_START_TIME_EXIT        BUTTON_REC

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_PREV
#define MPEG_START_TIME_RIGHT       BUTTON_NEXT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_LEFT2       BUTTON_OK
#define MPEG_START_TIME_RIGHT2      BUTTON_CANCEL
#define MPEG_START_TIME_EXIT        BUTTON_REC

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define MPEG_START_TIME_SELECT      BUTTON_FUNC
#define MPEG_START_TIME_LEFT        BUTTON_REW
#define MPEG_START_TIME_RIGHT       BUTTON_FF
#define MPEG_START_TIME_UP          BUTTON_VOL_UP
#define MPEG_START_TIME_DOWN        BUTTON_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_REC

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define MPEG_START_TIME_SELECT      BUTTON_ENTER
#define MPEG_START_TIME_LEFT        BUTTON_REW
#define MPEG_START_TIME_RIGHT       BUTTON_FF
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_REC

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YPR0_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_BACK

#elif (CONFIG_KEYPAD == HM60X_PAD) || (CONFIG_KEYPAD == HM801_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MPEG_START_TIME_SELECT
#define MPEG_START_TIME_SELECT      BUTTON_CENTER
#endif
#ifndef MPEG_START_TIME_LEFT
#define MPEG_START_TIME_LEFT        BUTTON_MIDLEFT
#endif
#ifndef MPEG_START_TIME_RIGHT
#define MPEG_START_TIME_RIGHT       BUTTON_MIDRIGHT
#endif
#ifndef MPEG_START_TIME_UP
#define MPEG_START_TIME_UP          BUTTON_TOPMIDDLE
#endif
#ifndef MPEG_START_TIME_DOWN
#define MPEG_START_TIME_DOWN        BUTTON_BOTTOMMIDDLE
#endif
#ifndef MPEG_START_TIME_LEFT2
#define MPEG_START_TIME_LEFT2       BUTTON_TOPRIGHT
#endif
#ifndef MPEG_START_TIME_RIGHT2
#define MPEG_START_TIME_RIGHT2      BUTTON_TOPLEFT
#endif
#ifndef MPEG_START_TIME_EXIT
#define MPEG_START_TIME_EXIT        BUTTON_TOPLEFT
#endif
#endif

static struct configdata config[] =
{
    {TYPE_INT, 0, 2, { .int_p = &settings.showfps }, "Show FPS", NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.limitfps }, "Limit FPS", NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.skipframes }, "Skip frames", NULL},
    {TYPE_INT, 0, INT_MAX, { .int_p = &settings.resume_count }, "Resume count",
     NULL},
    {TYPE_INT, 0, MPEG_RESUME_NUM_OPTIONS,
     { .int_p = &settings.resume_options }, "Resume options", NULL},
#if MPEG_OPTION_DITHERING_ENABLED
    {TYPE_INT, 0, INT_MAX, { .int_p = &settings.displayoptions },
     "Display options", NULL},
#endif
    {TYPE_INT, 0, 2, { .int_p = &settings.tone_controls }, "Tone controls",
     NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.channel_modes }, "Channel modes",
     NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.crossfeed }, "Crossfeed", NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.equalizer }, "Equalizer", NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.dithering }, "Dithering", NULL},
    {TYPE_INT, 0, 2, { .int_p = &settings.play_mode }, "Play mode", NULL},
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    {TYPE_INT, -1, INT_MAX, { .int_p = &settings.backlight_brightness },
     "Backlight brightness", NULL},
#endif
};

static const struct opt_items noyes[2] = {
    { "No", -1 },
    { "Yes", -1 },
};

static const struct opt_items singleall[2] = {
    { "Single", -1 },
    { "All", -1 },
};

static const struct opt_items enabledisable[2] = {
    { "Disable", -1 },
    { "Enable", -1 },
};

static const struct opt_items globaloff[2] = {
    { "Force off", -1 },
    { "Use sound setting", -1 },
};

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
#define BACKLIGHT_OPTION_DEFAULT "Use setting"
#endif

static void mpeg_settings(void);
static bool mpeg_set_option(const char* string,
                            void* variable,
                            enum optiontype type,
                            const struct opt_items* options,
                            int numoptions,
                            void (*function)(int))
{
    mpeg_sysevent_clear();

    /* This eats SYS_POWEROFF - :\ */
    bool usb = rb->set_option(string, variable, type, options, numoptions,
                              function);

    if (usb)
        mpeg_sysevent_set();

    return usb;
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS /* Only used for this atm */
static bool mpeg_set_int(const char *string, const char *unit,
                         int voice_unit, const int *variable,
                         void (*function)(int), int step,
                         int min,
                         int max,
                         const char* (*formatter)(char*, size_t, int, const char*))
{
    mpeg_sysevent_clear();

    bool usb = rb->set_int(string, unit, voice_unit, variable, function,
                           step, min, max, formatter);

    if (usb)
        mpeg_sysevent_set();

    return usb;
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void mpeg_backlight_update_brightness(int value)
{
    if (value >= 0)
    {
        value += MIN_BRIGHTNESS_SETTING;
        backlight_brightness_set(value);
    }
    else
    {
        backlight_brightness_use_setting();
    }
}

static void backlight_brightness_function(int value)
{
    mpeg_backlight_update_brightness(value);
}

static const char* backlight_brightness_formatter(char *buf, size_t length,
                                                  int value, const char *input)
{
    (void)input;

    if (value < 0)
        return BACKLIGHT_OPTION_DEFAULT;
    else
        rb->snprintf(buf, length, "%d", value + MIN_BRIGHTNESS_SETTING);
    return buf;
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

/* Sync a particular audio setting to global or mpegplayer forced off */
static void sync_audio_setting(int setting, bool global)
{
    switch (setting)
    {
    case MPEG_AUDIO_TONE_CONTROLS:
    #ifdef AUDIOHW_HAVE_BASS
        rb->sound_set(SOUND_BASS, (global || settings.tone_controls)
            ? rb->global_settings->bass
            : rb->sound_default(SOUND_BASS));
    #endif
    #ifdef AUDIOHW_HAVE_TREBLE
        rb->sound_set(SOUND_TREBLE, (global || settings.tone_controls)
            ? rb->global_settings->treble
            : rb->sound_default(SOUND_TREBLE));
    #endif

    #ifdef AUDIOHW_HAVE_EQ
        for (int band = 0;; band++)
        {
            int setting = rb->sound_enum_hw_eq_band_setting(band, AUDIOHW_EQ_GAIN);

            if (setting == -1)
                break;

            rb->sound_set(setting, (global || settings.tone_controls)
                    ? rb->global_settings->hw_eq_bands[band].gain
                    : rb->sound_default(setting));
        }
    #endif /* AUDIOHW_HAVE_EQ */
        break;

    case MPEG_AUDIO_CHANNEL_MODES:
        rb->sound_set(SOUND_CHANNELS, (global || settings.channel_modes)
                ? rb->global_settings->channel_config
                : SOUND_CHAN_STEREO);
        break;

    case MPEG_AUDIO_CROSSFEED:
        rb->dsp_set_crossfeed_type((global || settings.crossfeed) ?
                                   rb->global_settings->crossfeed :
                                   CROSSFEED_TYPE_NONE);
        break;

    case MPEG_AUDIO_EQUALIZER:
        rb->dsp_eq_enable((global || settings.equalizer) ?
                          rb->global_settings->eq_enabled : false);
        break;

    case MPEG_AUDIO_DITHERING:
        rb->dsp_dither_enable((global || settings.dithering) ?
                              rb->global_settings->dithering_enabled : false);
       break;
    }
}

/* Sync all audio settings to global or mpegplayer forced off */
static void sync_audio_settings(bool global)
{
    static const int setting_index[] =
    {
        MPEG_AUDIO_TONE_CONTROLS,
        MPEG_AUDIO_CHANNEL_MODES,
        MPEG_AUDIO_CROSSFEED,
        MPEG_AUDIO_EQUALIZER,
        MPEG_AUDIO_DITHERING,
    };
    unsigned i;

    for (i = 0; i < ARRAYLEN(setting_index); i++)
    {
        sync_audio_setting(setting_index[i], global);
    }
}

#ifndef HAVE_LCD_COLOR
/* Cheapo splash implementation for the grey surface */
static void grey_splash(int ticks, const unsigned char *fmt, ...)
{
    unsigned char buffer[256];
    int x, y, w, h;
    int oldfg, oldmode;

    va_list ap;
    va_start(ap, fmt);

    rb->vsnprintf(buffer, sizeof (buffer), fmt, ap);

    va_end(ap);

    grey_getstringsize(buffer, &w, &h);

    oldfg = grey_get_foreground();
    oldmode = grey_get_drawmode();

    grey_set_drawmode(DRMODE_FG);
    grey_set_foreground(GREY_LIGHTGRAY);

    x = (LCD_WIDTH - w) / 2;
    y = (LCD_HEIGHT - h) / 2;

    grey_fillrect(x - 1, y - 1, w + 2, h + 2);

    grey_set_foreground(GREY_BLACK);

    grey_putsxy(x, y, buffer);
    grey_drawrect(x - 2, y - 2, w + 4, h + 4);

    grey_set_foreground(oldfg);
    grey_set_drawmode(oldmode);

    grey_update();

    if (ticks > 0)
        rb->sleep(ticks);
}
#endif /* !HAVE_LCD_COLOR */

static void show_loading(struct vo_rect *rc)
{
    int oldmode = mylcd_get_drawmode();
    mylcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
    mylcd_fillrect(rc->l-1, rc->t-1, rc->r - rc->l + 2, rc->b - rc->t + 2);
    mylcd_set_drawmode(oldmode);
    mylcd_splash(0, "Loading...");
}

static void draw_slider(uint32_t range, uint32_t pos, struct vo_rect *rc)
{
    #define SLIDER_WIDTH   (LCD_WIDTH-SLIDER_LMARGIN-SLIDER_RMARGIN)
    #define SLIDER_X       SLIDER_LMARGIN
    #define SLIDER_Y       (LCD_HEIGHT-SLIDER_HEIGHT-SLIDER_BMARGIN)
    #define SLIDER_HEIGHT  8
    #define SLIDER_TEXTMARGIN 1
    #define SLIDER_LMARGIN 1
    #define SLIDER_RMARGIN 1
    #define SLIDER_TMARGIN 1
    #define SLIDER_BMARGIN 1
    #define SCREEN_MARGIN  1

    struct hms hms;
    char str[32];
    int text_w, text_h, text_y;

    /* Put positition on left */
    ts_to_hms(pos, &hms);
    hms_format(str, sizeof(str), &hms);
    mylcd_getstringsize(str, NULL, &text_h);
    text_y = SLIDER_Y - SLIDER_TEXTMARGIN - text_h;

    if (rc == NULL)
    {
        int oldmode = mylcd_get_drawmode();
        mylcd_set_drawmode(DRMODE_BG | DRMODE_INVERSEVID);
        mylcd_fillrect(SLIDER_X, text_y, SLIDER_WIDTH,
                       LCD_HEIGHT - SLIDER_BMARGIN - text_y
                       - SLIDER_TMARGIN);
        mylcd_set_drawmode(oldmode);

        mylcd_putsxy(SLIDER_X, text_y, str);

        /* Put duration on right */
        ts_to_hms(range, &hms);
        hms_format(str, sizeof(str), &hms);
        mylcd_getstringsize(str, &text_w, NULL);

        mylcd_putsxy(SLIDER_X + SLIDER_WIDTH - text_w, text_y, str);

        /* Draw slider */
        mylcd_drawrect(SLIDER_X, SLIDER_Y, SLIDER_WIDTH, SLIDER_HEIGHT);
        mylcd_fillrect(SLIDER_X, SLIDER_Y,
                      muldiv_uint32(pos, SLIDER_WIDTH, range),
                      SLIDER_HEIGHT);

        /* Update screen */
        mylcd_update_rect(SLIDER_X, text_y - SLIDER_TMARGIN, SLIDER_WIDTH,
                         LCD_HEIGHT - SLIDER_BMARGIN - text_y + SLIDER_TEXTMARGIN);
    }
    else
    {
        /* Just return slider rectangle */
        rc->l = SLIDER_X;
        rc->t = text_y - SLIDER_TMARGIN;
        rc->r = rc->l + SLIDER_WIDTH;
        rc->b = rc->t + LCD_HEIGHT - SLIDER_BMARGIN - text_y;
    }
}

static bool display_thumb_image(const struct vo_rect *rc)
{
    bool retval = true;
    unsigned ltgray = MYLCD_LIGHTGRAY;
    unsigned dkgray = MYLCD_DARKGRAY;

    int oldcolor = mylcd_get_foreground();

    if (!stream_display_thumb(rc))
    {
        /* Display "No Frame" and erase any border */
        const char * const str = "No Frame";
        int x, y, w, h;

        mylcd_getstringsize(str, &w, &h);
        x = (rc->r + rc->l - w) / 2;
        y = (rc->b + rc->t - h) / 2;
        mylcd_putsxy(x, y, str);

        mylcd_update_rect(x, y, w, h);

        ltgray = dkgray = mylcd_get_background();
        retval = false;
    }

    /* Draw a raised border around the frame (or erase if no frame) */

    mylcd_set_foreground(ltgray);

    mylcd_hline(rc->l-1, rc->r-1, rc->t-1);
    mylcd_vline(rc->l-1, rc->t, rc->b-1);

    mylcd_set_foreground(dkgray);

    mylcd_hline(rc->l-1, rc->r, rc->b);
    mylcd_vline(rc->r, rc->t-1, rc->b);

    mylcd_set_foreground(oldcolor);

    mylcd_update_rect(rc->l-1, rc->t-1, rc->r - rc->l + 2, 1);
    mylcd_update_rect(rc->l-1, rc->t, 1, rc->b - rc->t);
    mylcd_update_rect(rc->l-1, rc->b, rc->r - rc->l + 2, 1);
    mylcd_update_rect(rc->r, rc->t, 1, rc->b - rc->t);

    return retval;
}

/* Add an amount to the specified time - with saturation */
static uint32_t increment_time(uint32_t val, int32_t amount, uint32_t range)
{
    if (amount < 0)
    {
        uint32_t off = -amount;
        if (range > off && val >= off)
            val -= off;
        else
            val = 0;
    }
    else if (amount > 0)
    {
        uint32_t off = amount;
        if (range > off && val <= range - off)
            val += off;
        else
            val = range;
    }

    return val;
}

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
static void get_start_time_lcd_enable_hook(void *param)
{
    (void)param;
    rb->queue_post(rb->button_queue, LCD_ENABLE_EVENT_0, 0);
}
#endif /* HAVE_LCD_ENABLE */

static int get_start_time(uint32_t duration)
{
    int button = 0;
    int tmo = TIMEOUT_NOBLOCK;
    uint32_t resume_time = settings.resume_time;
    struct vo_rect rc_vid, rc_bound;
    uint32_t aspect_vid, aspect_bound;

    enum state_enum slider_state = STATE0;

    mylcd_clear_display();
    mylcd_update();

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    rb->add_event(LCD_EVENT_ACTIVATION, false, get_start_time_lcd_enable_hook);
#endif

    draw_slider(0, 100, &rc_bound);
    rc_bound.b = rc_bound.t - SLIDER_TMARGIN;
    rc_bound.t = SCREEN_MARGIN;

    DEBUGF("rc_bound: %d, %d, %d, %d\n", rc_bound.l, rc_bound.t,
           rc_bound.r, rc_bound.b);

    rc_vid.l = rc_vid.t = 0;
    if (!stream_vo_get_size((struct vo_ext *)&rc_vid.r))
    {
        /* Can't get size - fill whole thing */
        rc_vid.r = rc_bound.r - rc_bound.l;
        rc_vid.b = rc_bound.b - rc_bound.t;
    }

    /* Get aspect ratio of bounding rectangle and video in u16.16 */
    aspect_bound = ((rc_bound.r - rc_bound.l) << 16) /
                    (rc_bound.b - rc_bound.t);

    DEBUGF("aspect_bound: %u.%02u\n", (unsigned)(aspect_bound >> 16),
           (unsigned)(100*(aspect_bound & 0xffff) >> 16));

    aspect_vid = (rc_vid.r << 16) / rc_vid.b;

    DEBUGF("aspect_vid: %u.%02u\n", (unsigned)(aspect_vid >> 16),
           (unsigned)(100*(aspect_vid & 0xffff) >> 16));

    if (aspect_vid >= aspect_bound)
    {
        /* Video proportionally wider than or same as bounding rectangle */
        if (rc_vid.r > rc_bound.r - rc_bound.l)
        {
            rc_vid.r = rc_bound.r - rc_bound.l;
            rc_vid.b = (rc_vid.r << 16) / aspect_vid;
        }
        /* else already fits */
    }
    else
    {
        /* Video proportionally narrower than bounding rectangle */
        if (rc_vid.b > rc_bound.b - rc_bound.t)
        {
            rc_vid.b = rc_bound.b - rc_bound.t;
            rc_vid.r = (aspect_vid * rc_vid.b) >> 16;
        }
        /* else already fits */
    }

    /* Even width and height >= 2 */
    rc_vid.r = (rc_vid.r < 2) ? 2 : (rc_vid.r & ~1);
    rc_vid.b = (rc_vid.b < 2) ? 2 : (rc_vid.b & ~1);

    /* Center display in bounding rectangle */
    rc_vid.l = ((rc_bound.l + rc_bound.r) - rc_vid.r) / 2;
    rc_vid.r += rc_vid.l;

    rc_vid.t = ((rc_bound.t + rc_bound.b) - rc_vid.b) / 2;
    rc_vid.b += rc_vid.t;

    DEBUGF("rc_vid: %d, %d, %d, %d\n", rc_vid.l, rc_vid.t,
           rc_vid.r, rc_vid.b);

#ifndef HAVE_LCD_COLOR
    stream_gray_show(true);
#endif

    while (slider_state < STATE9)
    {
        button = mpeg_button_get(tmo);

        switch (button)
        {
        case BUTTON_NONE:
            break;

        /* Coarse (1 minute) control */
        case MPEG_START_TIME_DOWN:
        case MPEG_START_TIME_DOWN | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_DOWN
        case MPEG_START_TIME_RC_DOWN:
        case MPEG_START_TIME_RC_DOWN | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, -60*TS_SECOND, duration);
            slider_state = STATE0;
            break;

        case MPEG_START_TIME_UP:
        case MPEG_START_TIME_UP | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_UP
        case MPEG_START_TIME_RC_UP:
        case MPEG_START_TIME_RC_UP | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, 60*TS_SECOND, duration);
            slider_state = STATE0;
            break;

        /* Fine (1 second) control */
        case MPEG_START_TIME_LEFT:
        case MPEG_START_TIME_LEFT | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_LEFT
        case MPEG_START_TIME_RC_LEFT:
        case MPEG_START_TIME_RC_LEFT | BUTTON_REPEAT:
#endif
#ifdef MPEG_START_TIME_LEFT2
        case MPEG_START_TIME_LEFT2:
        case MPEG_START_TIME_LEFT2 | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, -TS_SECOND, duration);
            slider_state = STATE0;
            break;

        case MPEG_START_TIME_RIGHT:
        case MPEG_START_TIME_RIGHT | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_RIGHT
        case MPEG_START_TIME_RC_RIGHT:
        case MPEG_START_TIME_RC_RIGHT | BUTTON_REPEAT:
#endif
#ifdef MPEG_START_TIME_RIGHT2
        case MPEG_START_TIME_RIGHT2:
        case MPEG_START_TIME_RIGHT2 | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, TS_SECOND, duration);
            slider_state = STATE0;
            break;

        case MPEG_START_TIME_SELECT:
#ifdef MPEG_START_TIME_RC_SELECT
        case MPEG_START_TIME_RC_SELECT:
#endif
            settings.resume_time = resume_time;
            button = MPEG_START_SEEK;
            slider_state = STATE9;
            break;

        case MPEG_START_TIME_EXIT:
#ifdef MPEG_START_TIME_RC_EXIT
        case MPEG_START_TIME_RC_EXIT:
#endif
            button = MPEG_START_EXIT;
            slider_state = STATE9;
            break;

        case ACTION_STD_CANCEL:
            button = MPEG_START_QUIT;
            slider_state = STATE9;
            break;

#ifdef HAVE_LCD_ENABLE
        case LCD_ENABLE_EVENT_0:
            if (slider_state == STATE2)
                display_thumb_image(&rc_vid);
            continue;
#endif

        default:
            rb->default_event_handler(button);
            rb->yield();
            continue;
        }

        switch (slider_state)
        {
        case STATE0:
            trigger_cpu_boost();
            stream_seek(resume_time, SEEK_SET);
            show_loading(&rc_bound);
            draw_slider(duration, resume_time, NULL);
            slider_state = STATE1;
            tmo = THUMB_DELAY;
            break;
        case STATE1:
            display_thumb_image(&rc_vid);
            slider_state = STATE2;
        case STATE2:
            cancel_cpu_boost();
            tmo = TIMEOUT_BLOCK;
        default:
            break;
        }

        rb->yield();
    }

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    rb->remove_event(LCD_EVENT_ACTIVATION, get_start_time_lcd_enable_hook);
#endif
#ifndef HAVE_LCD_COLOR
    stream_gray_show(false);
    grey_clear_display();
    grey_update();
#endif

    cancel_cpu_boost();

    return button;
}

static int show_start_menu(uint32_t duration)
{
    int selected = 0;
    int result = 0;
    bool menu_quit = false;

    /* add the resume time to the menu display */
    static char resume_str[32];
    char hms_str[32];
    struct hms hms;

    MENUITEM_STRINGLIST(menu, "Mpegplayer Menu", mpeg_sysevent_callback,
                        "Play from beginning", resume_str, "Set start time",
                        "Settings", "Quit mpegplayer");

    ts_to_hms(settings.resume_time, &hms);
    hms_format(hms_str, sizeof(hms_str), &hms);
    rb->snprintf(resume_str, sizeof (resume_str),
                     "Resume at: %s", hms_str);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_sysevent_clear();
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
        case MPEG_START_RESTART:
            settings.resume_time = 0;
            menu_quit = true;
            break;

        case MPEG_START_RESUME:
            menu_quit = true;
            break;

        case MPEG_START_SEEK:
            if (!stream_can_seek())
            {
                rb->splash(HZ, "Unavailable");
                break;
            }

            result = get_start_time(duration);

            if (result != MPEG_START_EXIT)
                menu_quit = true;
            break;

        case MPEG_START_SETTINGS:
            mpeg_settings();
            break;

        default:
            result = MPEG_START_QUIT;
            menu_quit = true;
            break;
        }

        if (mpeg_sysevent() != 0)
        {
            result = MPEG_START_QUIT;
            menu_quit = true;
        }
    }

    return result;
}

/* Return the desired resume action */
int mpeg_start_menu(uint32_t duration)
{
    mpeg_sysevent_clear();

    switch (settings.resume_options)
    {
    case MPEG_RESUME_MENU_IF_INCOMPLETE:
        if (!stream_can_seek() || settings.resume_time == 0)
        {
    case MPEG_RESUME_RESTART:
            settings.resume_time = 0;
            return MPEG_START_RESTART;
        }
    default:
    case MPEG_RESUME_MENU_ALWAYS:
        return show_start_menu(duration);
    case MPEG_RESUME_ALWAYS:
        return MPEG_START_SEEK;
    }
}

int mpeg_menu(void)
{
    int result;

    MENUITEM_STRINGLIST(menu, "Mpegplayer Menu", mpeg_sysevent_callback,
                        "Settings", "Resume playback", "Quit mpegplayer");

    rb->button_clear_queue();

    mpeg_sysevent_clear();

    result = rb->do_menu(&menu, NULL, NULL, false);

    switch (result)
    {
    case MPEG_MENU_SETTINGS:
        mpeg_settings();
        break;

    case MPEG_MENU_RESUME:
        break;

    case MPEG_MENU_QUIT:
        break;

    default:
        break;
    }

    if (mpeg_sysevent() != 0)
        result = MPEG_MENU_QUIT;

    return result;
}

static void display_options(void)
{
    int selected = 0;
    int result;
    bool menu_quit = false;

    MENUITEM_STRINGLIST(menu, "Display Options", mpeg_sysevent_callback,
#if MPEG_OPTION_DITHERING_ENABLED
                        "Dithering",
#endif
                        "Display FPS", "Limit FPS", "Skip frames",
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
                        "Backlight brightness",
#endif
                        );

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_sysevent_clear();
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
#if MPEG_OPTION_DITHERING_ENABLED
        case MPEG_OPTION_DITHERING:
            result = (settings.displayoptions & LCD_YUV_DITHER) ? 1 : 0;
            mpeg_set_option("Dithering", &result, INT, noyes, 2, NULL);
            settings.displayoptions =
                (settings.displayoptions & ~LCD_YUV_DITHER)
                      | ((result != 0) ? LCD_YUV_DITHER : 0);
            rb->lcd_yuv_set_options(settings.displayoptions);
            break;
#endif /* MPEG_OPTION_DITHERING_ENABLED */

        case MPEG_OPTION_DISPLAY_FPS:
            mpeg_set_option("Display FPS", &settings.showfps, INT,
                            noyes, 2, NULL);
            break;

        case MPEG_OPTION_LIMIT_FPS:
            mpeg_set_option("Limit FPS", &settings.limitfps, INT,
                            noyes, 2, NULL);
            break;

        case MPEG_OPTION_SKIP_FRAMES:
            mpeg_set_option("Skip frames", &settings.skipframes, INT,
                            noyes, 2, NULL);
            break;

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
        case MPEG_OPTION_BACKLIGHT_BRIGHTNESS:
            result = settings.backlight_brightness;
            mpeg_backlight_update_brightness(result);
            mpeg_set_int("Backlight brightness", NULL, -1, &result,
                         backlight_brightness_function, 1, -1,
                         MAX_BRIGHTNESS_SETTING - MIN_BRIGHTNESS_SETTING,
                         backlight_brightness_formatter);
            settings.backlight_brightness = result;
            mpeg_backlight_update_brightness(-1);
            break;
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

        default:
            menu_quit = true;
            break;
        }

        if (mpeg_sysevent() != 0)
            menu_quit = true;
    }
}

static void audio_options(void)
{
    int selected = 0;
    int result;
    bool menu_quit = false;

    MENUITEM_STRINGLIST(menu, "Audio Options", mpeg_sysevent_callback,
                        "Tone Controls", "Channel Modes", "Crossfeed",
                        "Equalizer", "Dithering");

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_sysevent_clear();
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
        case MPEG_AUDIO_TONE_CONTROLS:
            mpeg_set_option("Tone Controls", &settings.tone_controls, INT,
                            globaloff, 2, NULL);
            sync_audio_setting(result, false);
            break;

        case MPEG_AUDIO_CHANNEL_MODES:
            mpeg_set_option("Channel Modes", &settings.channel_modes,
                            INT, globaloff, 2, NULL);
            sync_audio_setting(result, false);
            break;

        case MPEG_AUDIO_CROSSFEED:
            mpeg_set_option("Crossfeed", &settings.crossfeed, INT,
                            globaloff, 2, NULL);
            sync_audio_setting(result, false);
            break;

        case MPEG_AUDIO_EQUALIZER:
            mpeg_set_option("Equalizer", &settings.equalizer, INT,
                            globaloff, 2, NULL);
            sync_audio_setting(result, false);
            break;

        case MPEG_AUDIO_DITHERING:
            mpeg_set_option("Dithering", &settings.dithering, INT,
                            globaloff, 2, NULL);
            sync_audio_setting(result, false);
            break;

        default:
            menu_quit = true;
            break;
        }

        if (mpeg_sysevent() != 0)
            menu_quit = true;
    }
}

static void resume_options(void)
{
    static const struct opt_items items[MPEG_RESUME_NUM_OPTIONS] = {
        [MPEG_RESUME_MENU_ALWAYS] =
            { "Start menu", -1 },
        [MPEG_RESUME_MENU_IF_INCOMPLETE] =
            { "Start menu if not completed", -1 },
        [MPEG_RESUME_ALWAYS] =
            { "Resume automatically", -1 },
        [MPEG_RESUME_RESTART] =
            { "Play from beginning", -1 },
    };

    mpeg_set_option("Resume Options", &settings.resume_options,
                    INT, items, MPEG_RESUME_NUM_OPTIONS, NULL);
}

static void clear_resume_count(void)
{
    settings.resume_count = 0;
    configfile_save(SETTINGS_FILENAME, config, ARRAYLEN(config),
                    SETTINGS_VERSION);
}

static void mpeg_settings(void)
{
    int selected = 0;
    int result;
    bool menu_quit = false;
    static char clear_str[32];

    MENUITEM_STRINGLIST(menu, "Settings", mpeg_sysevent_callback,
                        "Display Options", "Audio Options",
                        "Resume Options", "Play Mode", clear_str);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_sysevent_clear();

        /* Format and add resume option to the menu display */
        rb->snprintf(clear_str, sizeof(clear_str),
                     "Clear all resumes: %u", settings.resume_count);

        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
        case MPEG_SETTING_DISPLAY_SETTINGS:
            display_options();
            break;

        case MPEG_SETTING_AUDIO_SETTINGS:
            audio_options();
            break;

        case MPEG_SETTING_ENABLE_START_MENU:
            resume_options();
            break;

        case MPEG_SETTING_PLAY_MODE:
            mpeg_set_option("Play mode", &settings.play_mode,
                            INT, singleall, 2, NULL);
            break;

        case MPEG_SETTING_CLEAR_RESUMES:
            clear_resume_count();
            break;

        default:
            menu_quit = true;
            break;
        }

        if (mpeg_sysevent() != 0)
            menu_quit = true;
    }
}

void init_settings(const char* filename)
{
    /* Set the default settings */
    settings.showfps = 0;     /* Do not show FPS */
    settings.limitfps = 1;    /* Limit FPS */
    settings.skipframes = 1;  /* Skip frames */
    settings.play_mode = 0;   /* Play single video */
    settings.resume_options = MPEG_RESUME_MENU_ALWAYS; /* Enable start menu */
    settings.resume_count = 0;
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    settings.backlight_brightness = -1; /* Use default setting */
#endif
#if MPEG_OPTION_DITHERING_ENABLED
    settings.displayoptions = 0; /* No visual effects */
#endif
    settings.tone_controls = false;
    settings.channel_modes = false;
    settings.crossfeed = false;
    settings.equalizer = false;
    settings.dithering = false;

    if (configfile_load(SETTINGS_FILENAME, config, ARRAYLEN(config),
                        SETTINGS_MIN_VERSION) < 0)
    {
        /* Generate a new config file with default values */
        configfile_save(SETTINGS_FILENAME, config, ARRAYLEN(config),
                        SETTINGS_VERSION);
    }

    rb->strlcpy(settings.resume_filename, filename, MAX_PATH);

    /* get the resume time for the current mpeg if it exists */
    if ((settings.resume_time = configfile_get_value
         (SETTINGS_FILENAME, filename)) < 0)
    {
        settings.resume_time = 0;
    }

#if MPEG_OPTION_DITHERING_ENABLED
    rb->lcd_yuv_set_options(settings.displayoptions);
#endif

    /* Set our audio options */
    sync_audio_settings(false);
}

void save_settings(void)
{
    unsigned i;
    for (i = 0; i < ARRAYLEN(config); i++)
    {
        configfile_update_entry(SETTINGS_FILENAME, config[i].name,
                                *(config[i].int_p));
    }

    /* If this was a new resume entry then update the total resume count */
    if (configfile_update_entry(SETTINGS_FILENAME, settings.resume_filename,
                                settings.resume_time) == 0)
    {
        configfile_update_entry(SETTINGS_FILENAME, "Resume count",
                                ++settings.resume_count);
    }

    /* Restore audio options */
    sync_audio_settings(true);
}
