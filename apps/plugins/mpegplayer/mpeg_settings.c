#include "plugin.h"
#include "lib/helper.h"
#include "lib/configfile.h"
#include "lib/oldmenuapi.h"

#include "mpegplayer.h"
#include "mpeg_settings.h"

struct mpeg_settings settings;

#define THUMB_DELAY (75*HZ/100)

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_ON
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_OFF

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_UP          BUTTON_SCROLL_FWD
#define MPEG_START_TIME_DOWN        BUTTON_SCROLL_BACK
#define MPEG_START_TIME_EXIT        BUTTON_MENU

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_VOL_DOWN
#define MPEG_START_TIME_SCROLL_UP   BUTTON_VOL_UP
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#define MPEG_START_TIME_RC_SELECT      (BUTTON_RC_PLAY | BUTTON_REL)
#define MPEG_START_TIME_RC_LEFT        BUTTON_RC_REW
#define MPEG_START_TIME_RC_RIGHT       BUTTON_RC_FF
#define MPEG_START_TIME_RC_UP          BUTTON_RC_VOL_UP
#define MPEG_START_TIME_RC_DOWN        BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_RC_EXIT        (BUTTON_RC_PLAY | BUTTON_REPEAT)

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_VOL_DOWN
#define MPEG_START_TIME_SCROLL_UP   BUTTON_VOL_UP
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_START_TIME_SELECT      BUTTON_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_SCROLL_UP
#define MPEG_START_TIME_DOWN        BUTTON_SCROLL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_SCROLL_UP   BUTTON_SCROLL_BACK
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_SCROLL_FWD
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_SCROLL_UP   BUTTON_VOL_UP
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_VOL_DOWN
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_START_TIME_SELECT      BUTTON_RC_HEART
#define MPEG_START_TIME_SCROLL_UP   BUTTON_RC_VOL_UP
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_RC_PLAY
#define MPEG_START_TIME_DOWN        BUTTON_RC_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE100_PAD
#define MPEG_START_TIME_SELECT      BUTTON_SELECT
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_UP
#define MPEG_START_TIME_DOWN        BUTTON_DOWN
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_MENU
#define MPEG_START_TIME_SCROLL_UP   BUTTON_PLAY
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define MPEG_START_TIME_SELECT      BUTTON_RC_PLAY
#define MPEG_START_TIME_LEFT        BUTTON_RC_REW
#define MPEG_START_TIME_RIGHT       BUTTON_RC_FF
#define MPEG_START_TIME_UP          BUTTON_RC_VOL_UP
#define MPEG_START_TIME_DOWN        BUTTON_RC_VOL_DOWN
#define MPEG_START_TIME_EXIT        BUTTON_RC_REC

#elif CONFIG_KEYPAD == COWOND2_PAD
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define MPEG_START_TIME_SELECT      BUTTON_MENU
#define MPEG_START_TIME_LEFT        BUTTON_LEFT
#define MPEG_START_TIME_RIGHT       BUTTON_RIGHT
#define MPEG_START_TIME_UP          BUTTON_STOP
#define MPEG_START_TIME_DOWN        BUTTON_PLAY
#define MPEG_START_TIME_EXIT        BUTTON_POWER

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef MPEG_START_TIME_SELECT
#define MPEG_START_TIME_SELECT      BUTTON_CENTER
#endif
#ifndef MPEG_START_TIME_SCROLL_UP
#define MPEG_START_TIME_SCROLL_UP   BUTTON_TOPRIGHT
#endif
#ifndef MPEG_START_TIME_SCROLL_DOWN
#define MPEG_START_TIME_SCROLL_DOWN BUTTON_TOPLEFT
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
#ifndef MPEG_START_TIME_EXIT
#define MPEG_START_TIME_EXIT        BUTTON_TOPLEFT
#endif
#endif

static struct configdata config[] =
{
    {TYPE_INT, 0, 2, &settings.showfps, "Show FPS", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.limitfps, "Limit FPS", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.skipframes, "Skip frames", NULL, NULL},
    {TYPE_INT, 0, INT_MAX, &settings.resume_count, "Resume count",
     NULL, NULL},
    {TYPE_INT, 0, MPEG_RESUME_NUM_OPTIONS, &settings.resume_options,
     "Resume options", NULL, NULL},
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
    {TYPE_INT, 0, INT_MAX, &settings.displayoptions, "Display options",
     NULL, NULL},
#endif
    {TYPE_INT, 0, 2, &settings.tone_controls, "Tone controls", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.channel_modes, "Channel modes", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.crossfeed, "Crossfeed", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.equalizer, "Equalizer", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.dithering, "Dithering", NULL, NULL},
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    {TYPE_INT, -1, INT_MAX, &settings.backlight_brightness,
     "Backlight brightness", NULL, NULL},
#endif
};

static const struct opt_items noyes[2] = {
    { "No", -1 },
    { "Yes", -1 },
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

static long mpeg_menu_sysevent_id;

void mpeg_menu_sysevent_clear(void)
{
    mpeg_menu_sysevent_id = 0;
}

int mpeg_menu_sysevent_callback(int btn, int menu)
{
    switch (btn)
    {
    case SYS_USB_CONNECTED:
    case SYS_POWEROFF:
        mpeg_menu_sysevent_id = btn;
        return ACTION_STD_CANCEL;
    }

    return btn;
    (void)menu;
}

long mpeg_menu_sysevent(void)
{
    return mpeg_menu_sysevent_id;
}

void mpeg_menu_sysevent_handle(void)
{
    long id = mpeg_menu_sysevent();
    if (id != 0)
        rb->default_event_handler(id);
}

static void format_menu_item(struct menu_item *item, int bufsize,
                             const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    rb->vsnprintf(item->desc, bufsize, fmt, ap);

    va_end(ap);
}

static bool mpeg_set_option(const char* string,
                            void* variable,
                            enum optiontype type,
                            const struct opt_items* options,
                            int numoptions,
                            void (*function)(int))
{
    mpeg_menu_sysevent_clear();

    /* This eats SYS_POWEROFF - :\ */
    bool usb = rb->set_option(string, variable, type, options, numoptions,
                              function);

    if (usb)
        mpeg_menu_sysevent_id = ACTION_STD_CANCEL;

    return usb;
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS /* Only used for this atm */
static bool mpeg_set_int(const char *string, const char *unit,
                         int voice_unit, const int *variable,
                         void (*function)(int), int step,
                         int min,
                         int max,
                         void (*formatter)(char*, size_t, int, const char*))
{
    mpeg_menu_sysevent_clear();

    bool usb = rb->set_int(string, unit, voice_unit, variable, function,
                           step, min, max, formatter);

    if (usb)
        mpeg_menu_sysevent_id = ACTION_STD_CANCEL;

    return usb;
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
void mpeg_backlight_update_brightness(int value)
{
    if (value >= 0)
    {
        value += MIN_BRIGHTNESS_SETTING;
        backlight_brightness_set(rb, value);
    }
    else
    {
        backlight_brightness_use_setting(rb);
    }
}

static void backlight_brightness_function(int value)
{
    mpeg_backlight_update_brightness(value);
}

static void backlight_brightness_formatter(char *buf, size_t length,
                                           int value, const char *input)
{
    if (value < 0)
        rb->strncpy(buf, BACKLIGHT_OPTION_DEFAULT, length);
    else
        rb->snprintf(buf, length, "%d", value + MIN_BRIGHTNESS_SETTING);

    (void)input;
}
#endif /* HAVE_BACKLIGHT_BRIGHTNESS */

/* Sync a particular audio setting to global or mpegplayer forced off */
static void sync_audio_setting(int setting, bool global)
{
    int val0, val1;

    switch (setting)
    {
    case MPEG_AUDIO_TONE_CONTROLS:
        if (global || settings.tone_controls)
        {
            val0 = rb->global_settings->bass;
            val1 = rb->global_settings->treble;
        }
        else
        {
            val0 = rb->sound_default(SOUND_BASS);
            val1 = rb->sound_default(SOUND_TREBLE);
        }
        rb->sound_set(SOUND_BASS, val0);
        rb->sound_set(SOUND_TREBLE, val1);
        break;

    case MPEG_AUDIO_CHANNEL_MODES:
        val0 = (global || settings.channel_modes) ?
                rb->global_settings->channel_config :
                SOUND_CHAN_STEREO;
        rb->sound_set(SOUND_CHANNELS, val0);
        break;

    case MPEG_AUDIO_CROSSFEED:
        rb->dsp_set_crossfeed((global || settings.crossfeed) ?
                              rb->global_settings->crossfeed : false);
        break;

    case MPEG_AUDIO_EQUALIZER:
        rb->dsp_set_eq((global || settings.equalizer) ?
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
    int oldmode = lcd_(get_drawmode)();
    lcd_(set_drawmode)(DRMODE_SOLID | DRMODE_INVERSEVID);
    lcd_(fillrect)(rc->l-1, rc->t-1, rc->r - rc->l + 2, rc->b - rc->t + 2);
    lcd_(set_drawmode)(oldmode);
    lcd_(splash)(0, "Loading...");
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
    lcd_(getstringsize)(str, NULL, &text_h);
    text_y = SLIDER_Y - SLIDER_TEXTMARGIN - text_h;

    if (rc == NULL)
    {
        int oldmode = lcd_(get_drawmode)();
        lcd_(set_drawmode)(DRMODE_BG | DRMODE_INVERSEVID);
        lcd_(fillrect)(SLIDER_X, text_y, SLIDER_WIDTH,
                       LCD_HEIGHT - SLIDER_BMARGIN - text_y
                       - SLIDER_TMARGIN);
        lcd_(set_drawmode)(oldmode);

        lcd_(putsxy)(SLIDER_X, text_y, str);

        /* Put duration on right */
        ts_to_hms(range, &hms);
        hms_format(str, sizeof(str), &hms);
        lcd_(getstringsize)(str, &text_w, NULL);

        lcd_(putsxy)(SLIDER_X + SLIDER_WIDTH - text_w, text_y, str);

        /* Draw slider */
        lcd_(drawrect)(SLIDER_X, SLIDER_Y, SLIDER_WIDTH, SLIDER_HEIGHT);
        lcd_(fillrect)(SLIDER_X, SLIDER_Y,
                       muldiv_uint32(pos, SLIDER_WIDTH, range),
                       SLIDER_HEIGHT);

        /* Update screen */
        lcd_(update_rect)(SLIDER_X, text_y - SLIDER_TMARGIN, SLIDER_WIDTH,
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
    if (!stream_display_thumb(rc))
    {
        lcd_(splash)(0, "Frame not available");
        return false;
    }

    /* Draw a raised border around the frame */
    int oldcolor = lcd_(get_foreground)();
    lcd_(set_foreground)(DRAW_LIGHTGRAY);

    lcd_(hline)(rc->l-1, rc->r-1, rc->t-1);
    lcd_(vline)(rc->l-1, rc->t, rc->b-1);

    lcd_(set_foreground)(DRAW_DARKGRAY);

    lcd_(hline)(rc->l-1, rc->r, rc->b);
    lcd_(vline)(rc->r, rc->t-1, rc->b);

    lcd_(set_foreground)(oldcolor);

    lcd_(update_rect)(rc->l-1, rc->t-1, rc->r - rc->l + 2, 1);
    lcd_(update_rect)(rc->l-1, rc->t, 1, rc->b - rc->t);
    lcd_(update_rect)(rc->l-1, rc->b, rc->r - rc->l + 2, 1);
    lcd_(update_rect)(rc->r, rc->t, 1, rc->b - rc->t);

    return true;
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

#ifdef HAVE_LCD_ENABLE
static void get_start_time_lcd_enable_hook(void)
{
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

    enum state_enum slider_state = state0;

    lcd_(clear_display)();
    lcd_(update)();

#if defined(HAVE_LCD_ENABLE) && defined(HAVE_LCD_COLOR)
    rb->lcd_set_enable_hook(get_start_time_lcd_enable_hook);
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

    while (slider_state < state9)
    {
        mpeg_menu_sysevent_clear();
        button = tmo == TIMEOUT_BLOCK ?
            rb->button_get(true) : rb->button_get_w_tmo(tmo);

        button = mpeg_menu_sysevent_callback(button, -1);

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
            slider_state = state0;
            break;

        case MPEG_START_TIME_UP:
        case MPEG_START_TIME_UP | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_UP
        case MPEG_START_TIME_RC_UP:
        case MPEG_START_TIME_RC_UP | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, 60*TS_SECOND, duration);
            slider_state = state0;
            break;

        /* Fine (1 second) control */
        case MPEG_START_TIME_LEFT:
        case MPEG_START_TIME_LEFT | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_LEFT
        case MPEG_START_TIME_RC_LEFT:
        case MPEG_START_TIME_RC_LEFT | BUTTON_REPEAT:
#endif
#ifdef MPEG_START_TIME_SCROLL_UP
        case MPEG_START_TIME_SCROLL_UP:
        case MPEG_START_TIME_SCROLL_UP | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, -TS_SECOND, duration);
            slider_state = state0;
            break;

        case MPEG_START_TIME_RIGHT:
        case MPEG_START_TIME_RIGHT | BUTTON_REPEAT:
#ifdef MPEG_START_TIME_RC_RIGHT
        case MPEG_START_TIME_RC_RIGHT:
        case MPEG_START_TIME_RC_RIGHT | BUTTON_REPEAT:
#endif
#ifdef MPEG_START_TIME_SCROLL_DOWN
        case MPEG_START_TIME_SCROLL_DOWN:
        case MPEG_START_TIME_SCROLL_DOWN | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, TS_SECOND, duration);
            slider_state = state0;
            break;

        case MPEG_START_TIME_SELECT:
#ifdef MPEG_START_TIME_RC_SELECT
        case MPEG_START_TIME_RC_SELECT:
#endif
            settings.resume_time = resume_time;
            button = MPEG_START_SEEK;
            slider_state = state9;
            break;

        case MPEG_START_TIME_EXIT:
#ifdef MPEG_START_TIME_RC_EXIT
        case MPEG_START_TIME_RC_EXIT:
#endif
            button = MPEG_START_EXIT;
            slider_state = state9;
            break;

        case ACTION_STD_CANCEL:
            button = MPEG_START_QUIT;
            slider_state = state9;
            break;

#ifdef HAVE_LCD_ENABLE
        case LCD_ENABLE_EVENT_0:
            if (slider_state == state2)
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
        case state0:
            trigger_cpu_boost();
            stream_seek(resume_time, SEEK_SET);
            show_loading(&rc_bound);
            draw_slider(duration, resume_time, NULL);
            slider_state = state1;
            tmo = THUMB_DELAY;
            break;
        case state1:
            display_thumb_image(&rc_vid);
            slider_state = state2;
        case state2:
            cancel_cpu_boost();
            tmo = TIMEOUT_BLOCK;
        default:
            break;
        }

        rb->yield();
    }

#ifdef HAVE_LCD_COLOR
#ifdef HAVE_LCD_ENABLE
    rb->lcd_set_enable_hook(NULL);
#endif
#else
    stream_gray_show(false);
    grey_clear_display();
    grey_update();
#endif

    cancel_cpu_boost();

    return button;
}

static int show_start_menu(uint32_t duration)
{
    int menu_id;
    int result = 0;
    bool menu_quit = false;

    /* add the resume time to the menu display */
    char resume_str[32];
    char hms_str[32];
    struct hms hms;

    struct menu_item items[] =
    {
        [MPEG_START_RESTART] =
            { "Play from beginning", NULL },
        [MPEG_START_RESUME] =
            { resume_str, NULL },
        [MPEG_START_SEEK] =
            { "Set start time", NULL },
        [MPEG_START_SETTINGS] =
            { "Settings", NULL },
        [MPEG_START_QUIT] =
            { "Quit mpegplayer", NULL },
    };

    ts_to_hms(settings.resume_time, &hms);
    hms_format(hms_str, sizeof(hms_str), &hms);
    format_menu_item(&items[MPEG_START_RESUME], sizeof (resume_str),
                     "Resume at: %s", hms_str);

    menu_id = menu_init(rb, items, ARRAYLEN(items),
                        mpeg_menu_sysevent_callback, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_menu_sysevent_clear();
        result = menu_show(menu_id);

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
            if (mpeg_menu(MPEG_MENU_HIDE_QUIT_ITEM) != MPEG_MENU_QUIT)
                break;
            /* Fall-through */
        default:
            result = MPEG_START_QUIT;
            menu_quit = true;
            break;
        }

        if (mpeg_menu_sysevent() != 0)
        {
            result = MPEG_START_QUIT;
            menu_quit = true;
        }
    }

    menu_exit(menu_id);

    rb->lcd_clear_display();
    rb->lcd_update();

    return result;
}

/* Return the desired resume action */
int mpeg_start_menu(uint32_t duration)
{
    mpeg_menu_sysevent_clear();

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

/** MPEG Menu **/
static void display_options(void)
{
    int result;
    int menu_id;
    bool menu_quit = false;

    static const struct menu_item items[] = {
#if MPEG_OPTION_DITHERING_ENABLED
        [MPEG_OPTION_DITHERING] =
            { "Dithering", NULL },
#endif
        [MPEG_OPTION_DISPLAY_FPS] =
            { "Display FPS", NULL },
        [MPEG_OPTION_LIMIT_FPS] =
            { "Limit FPS", NULL },
        [MPEG_OPTION_SKIP_FRAMES] =
            { "Skip frames", NULL },
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
        [MPEG_OPTION_BACKLIGHT_BRIGHTNESS] =
            { "Backlight brightness", NULL },
#endif
    };

    menu_id = menu_init(rb, items, ARRAYLEN(items),
                        mpeg_menu_sysevent_callback, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_menu_sysevent_clear();
        result = menu_show(menu_id);

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

        if (mpeg_menu_sysevent() != 0)
            menu_quit = true;
    }

    menu_exit(menu_id);
}

static void audio_options(void)
{
    int result;
    int menu_id;
    bool menu_quit = false;

    static const struct menu_item items[] = {
        [MPEG_AUDIO_TONE_CONTROLS] =
            { "Tone Controls", NULL },
        [MPEG_AUDIO_CHANNEL_MODES] =
            { "Channel Modes", NULL },
        [MPEG_AUDIO_CROSSFEED] =
            { "Crossfeed", NULL },
        [MPEG_AUDIO_EQUALIZER] =
            { "Equalizer", NULL },
        [MPEG_AUDIO_DITHERING] =
            { "Dithering", NULL },
    };

    menu_id = menu_init(rb, items, ARRAYLEN(items),
                        mpeg_menu_sysevent_callback, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_menu_sysevent_clear();
        result = menu_show(menu_id);

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

        if (mpeg_menu_sysevent() != 0)
            menu_quit = true;
    }

    menu_exit(menu_id);
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
    configfile_save(SETTINGS_FILENAME, config, ARRAYLEN(config),
                    SETTINGS_VERSION);

    settings.resume_count = 0;

    /* add this place holder so the count is above resume entries */
    configfile_update_entry(SETTINGS_FILENAME, "Resume count", 0);
}

int mpeg_menu(unsigned flags)
{
    int menu_id;
    int result;
    bool menu_quit = false;
    int item_count;
    char clear_str[32];

    struct menu_item items[] = {
        [MPEG_MENU_DISPLAY_SETTINGS] =
            { "Display Options", NULL },
        [MPEG_MENU_AUDIO_SETTINGS] =
            { "Audio Options", NULL },
        [MPEG_MENU_ENABLE_START_MENU] =
            { "Resume Options", NULL },
        [MPEG_MENU_CLEAR_RESUMES] =
            { clear_str, NULL },
        [MPEG_MENU_QUIT] =
            { "Quit mpegplayer", NULL },
    };

    item_count = ARRAYLEN(items);

    if (flags & MPEG_MENU_HIDE_QUIT_ITEM)
        item_count--;

    menu_id = menu_init(rb, items, item_count,
                        mpeg_menu_sysevent_callback, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit)
    {
        mpeg_menu_sysevent_clear();

        /* Format and add resume option to the menu display */
        format_menu_item(&items[MPEG_MENU_CLEAR_RESUMES], sizeof(clear_str),
                         "Clear all resumes: %u", settings.resume_count);

        result = menu_show(menu_id);

        switch (result)
        {
        case MPEG_MENU_DISPLAY_SETTINGS:
            display_options();
            break;

        case MPEG_MENU_AUDIO_SETTINGS:
            audio_options();
            break;

        case MPEG_MENU_ENABLE_START_MENU:
            resume_options();
            break;

        case MPEG_MENU_CLEAR_RESUMES:
            clear_resume_count();
            break;

        case MPEG_MENU_QUIT:
        default:
            menu_quit = true;
            break;
        }

        if (mpeg_menu_sysevent() != 0)
        {
            result = MPEG_MENU_QUIT;
            menu_quit = true;
        }
    }

    menu_exit(menu_id);

    rb->lcd_clear_display();
    rb->lcd_update();

    return result;
}

void init_settings(const char* filename)
{
    /* Set the default settings */
    settings.showfps = 0;     /* Do not show FPS */
    settings.limitfps = 1;    /* Limit FPS */
    settings.skipframes = 1;  /* Skip frames */
    settings.resume_options = MPEG_RESUME_MENU_ALWAYS; /* Enable start menu */
    settings.resume_count = -1;
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

    configfile_init(rb);

    if (configfile_load(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION) < 0)
    {
        /* Generate a new config file with default values */
        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }

#if MPEG_OPTION_DITHERING_ENABLED
    if ((settings.displayoptions =
         configfile_get_value(SETTINGS_FILENAME, "Display options")) < 0)
    {
        configfile_update_entry(SETTINGS_FILENAME, "Display options",
                                (settings.displayoptions=0));
    }
    rb->lcd_yuv_set_options(settings.displayoptions);
#endif

    if (settings.resume_count < 0)
    {
        settings.resume_count = 0;
        configfile_update_entry(SETTINGS_FILENAME, "Resume count", 0);
    }

    rb->snprintf(settings.resume_filename, MAX_PATH, "%s", filename);

    /* get the resume time for the current mpeg if it exist */
    if ((settings.resume_time = configfile_get_value
         (SETTINGS_FILENAME, filename)) < 0)
    {
        settings.resume_time = 0;
    }

    /* Set our audio options */
    sync_audio_settings(false);
}

void save_settings(void)
{
    configfile_update_entry(SETTINGS_FILENAME, "Show FPS",
                            settings.showfps);
    configfile_update_entry(SETTINGS_FILENAME, "Limit FPS",
                            settings.limitfps);
    configfile_update_entry(SETTINGS_FILENAME, "Skip frames",
                            settings.skipframes);
    configfile_update_entry(SETTINGS_FILENAME, "Resume options",
                            settings.resume_options);

    /* If this was a new resume entry then update the total resume count */
    if (configfile_update_entry(SETTINGS_FILENAME, settings.resume_filename,
                                settings.resume_time) == 0)
    {
        configfile_update_entry(SETTINGS_FILENAME, "Resume count",
                                ++settings.resume_count);
    }

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    configfile_update_entry(SETTINGS_FILENAME, "Backlight brightness",
                            settings.backlight_brightness);
#endif

#if MPEG_OPTION_DITHERING_ENABLED
    configfile_update_entry(SETTINGS_FILENAME, "Display options",
                            settings.displayoptions);
#endif
    configfile_update_entry(SETTINGS_FILENAME, "Tone controls",
                            settings.tone_controls);
    configfile_update_entry(SETTINGS_FILENAME, "Channel modes",
                            settings.channel_modes);
    configfile_update_entry(SETTINGS_FILENAME, "Crossfeed",
                            settings.crossfeed);
    configfile_update_entry(SETTINGS_FILENAME, "Equalizer",
                            settings.equalizer);
    configfile_update_entry(SETTINGS_FILENAME, "Dithering",
                            settings.dithering);

    /* Restore audio options */
    sync_audio_settings(true);
}
