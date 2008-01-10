#include "plugin.h"
#include "lib/configfile.h"
#include "lib/oldmenuapi.h"

#include "mpegplayer.h"
#include "mpeg_settings.h"

struct mpeg_settings settings;

#define SETTINGS_VERSION 2
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "mpegplayer.cfg"

#define THUMB_DELAY (75*HZ/100)

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_SELECT      BUTTON_ON
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_UP          BUTTON_UP
#define MPEG_DOWN        BUTTON_DOWN
#define MPEG_EXIT        BUTTON_OFF

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MPEG_SELECT      BUTTON_PLAY
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_UP          BUTTON_UP
#define MPEG_DOWN        BUTTON_DOWN
#define MPEG_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_UP          BUTTON_SCROLL_FWD
#define MPEG_DOWN        BUTTON_SCROLL_BACK
#define MPEG_EXIT        BUTTON_MENU

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_UP          BUTTON_UP
#define MPEG_DOWN        BUTTON_DOWN
#define MPEG_SCROLL_DOWN BUTTON_VOL_DOWN
#define MPEG_SCROLL_UP   BUTTON_VOL_UP
#define MPEG_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define MPEG_SELECT      BUTTON_PLAY
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_UP          BUTTON_SCROLL_UP
#define MPEG_DOWN        BUTTON_SCROLL_DOWN
#define MPEG_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_SCROLL_UP   BUTTON_SCROLL_BACK
#define MPEG_SCROLL_DOWN BUTTON_SCROLL_FWD
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_UP          BUTTON_UP
#define MPEG_DOWN        BUTTON_DOWN
#define MPEG_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD)
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_SCROLL_UP   BUTTON_VOL_UP
#define MPEG_SCROLL_DOWN BUTTON_VOL_DOWN
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_UP          BUTTON_UP
#define MPEG_DOWN        BUTTON_DOWN
#define MPEG_EXIT        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define MPEG_SELECT      BUTTON_RC_HEART
#define MPEG_SCROLL_UP   BUTTON_RC_VOL_UP
#define MPEG_SCROLL_DOWN BUTTON_RC_VOL_DOWN
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_UP          BUTTON_RC_PLAY
#define MPEG_DOWN        BUTTON_RC_DOWN
#define MPEG_EXIT        BUTTON_POWER

#else
#error MPEGPLAYER: Unsupported keypad
#endif

static struct configdata config[] =
{
    {TYPE_INT, 0, 2, &settings.showfps, "Show FPS", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.limitfps, "Limit FPS", NULL, NULL},
    {TYPE_INT, 0, 2, &settings.skipframes, "Skip frames", NULL, NULL},
    {TYPE_INT, 0, INT_MAX, &settings.resume_count, "Resume count",
     NULL, NULL},
    {TYPE_INT, 0, 2, &settings.enable_start_menu, "Enable start menu",
     NULL, NULL},
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
    {TYPE_INT, 0, INT_MAX, &settings.displayoptions, "Display options",
     NULL, NULL},
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

static void display_options(void)
{
    int result;
    int menu_id;
    int options_quit = 0;

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
    };

    menu_id = menu_init(rb, items, ARRAYLEN(items),
                            NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while(options_quit == 0)
    {
        result = menu_show(menu_id);

        switch (result)
        {
#if MPEG_OPTION_DITHERING_ENABLED
            case MPEG_OPTION_DITHERING:
                result = (settings.displayoptions & LCD_YUV_DITHER) ? 1 : 0;
                rb->set_option("Dithering", &result, INT, noyes, 2, NULL);
                settings.displayoptions = (settings.displayoptions & ~LCD_YUV_DITHER)
                  | ((result != 0) ? LCD_YUV_DITHER : 0);
                rb->lcd_yuv_set_options(settings.displayoptions);
                break;
#endif /* MPEG_OPTION_DITHERING_ENABLED */
            case MPEG_OPTION_DISPLAY_FPS:
                rb->set_option("Display FPS",&settings.showfps,INT,
                               noyes, 2, NULL);
                break;
            case MPEG_OPTION_LIMIT_FPS:
                rb->set_option("Limit FPS",&settings.limitfps,INT,
                               noyes, 2, NULL);
                break;
            case MPEG_OPTION_SKIP_FRAMES:
                rb->set_option("Skip frames",&settings.skipframes,INT,
                               noyes, 2, NULL);
                break;
            default:
                options_quit=1;
                break;
        }
    }

    menu_exit(menu_id);
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

void draw_slider(uint32_t range, uint32_t pos, struct vo_rect *rc)
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

bool display_thumb_image(const struct vo_rect *rc)
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
uint32_t increment_time(uint32_t val, int32_t amount, uint32_t range)
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

int get_start_time(uint32_t duration)
{
    int button = 0;
    int tmo = TIMEOUT_NOBLOCK;
    uint32_t resume_time = settings.resume_time;
    struct vo_rect rc_vid, rc_bound;
    uint32_t aspect_vid, aspect_bound;

    enum state_enum slider_state = state0;

    lcd_(clear_display)();
    lcd_(update)();

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
    /* Restore gray overlay dimensions */
    stream_gray_show(true);
#endif

    while(slider_state < state9)
    {
        button = tmo == TIMEOUT_BLOCK ?
            rb->button_get(true) : rb->button_get_w_tmo(tmo);

        switch (button)
        {
        case BUTTON_NONE:
            break;

        /* Coarse (1 minute) control */
        case MPEG_DOWN:
        case MPEG_DOWN | BUTTON_REPEAT:
            resume_time = increment_time(resume_time, -60*TS_SECOND, duration);
            slider_state = state0;
            break;

        case MPEG_UP:
        case MPEG_UP | BUTTON_REPEAT:
            resume_time = increment_time(resume_time, 60*TS_SECOND, duration);
            slider_state = state0;
            break;

        /* Fine (1 second) control */
        case MPEG_LEFT:
        case MPEG_LEFT | BUTTON_REPEAT:
#ifdef MPEG_SCROLL_UP
        case MPEG_SCROLL_UP:
        case MPEG_SCROLL_UP | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, -TS_SECOND, duration);
            slider_state = state0;
            break;

        case MPEG_RIGHT:
        case MPEG_RIGHT | BUTTON_REPEAT:
#ifdef MPEG_SCROLL_DOWN
        case MPEG_SCROLL_DOWN:
        case MPEG_SCROLL_DOWN | BUTTON_REPEAT:
#endif
            resume_time = increment_time(resume_time, TS_SECOND, duration);
            slider_state = state0;
            break;

        case MPEG_SELECT:
            settings.resume_time = resume_time;
        case MPEG_EXIT:
            slider_state = state9;
            break;

        case SYS_USB_CONNECTED:
            slider_state = state9;
#ifndef HAVE_LCD_COLOR
            stream_gray_show(false);
#endif
            cancel_cpu_boost();
        default:
            rb->default_event_handler(button);
            rb->yield();
            continue;
        }

        switch(slider_state)
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
        case state9:
            cancel_cpu_boost();
            tmo = TIMEOUT_BLOCK;
        default:
            break;
        }
    }

#ifndef HAVE_LCD_COLOR
    stream_gray_show(false);
    grey_clear_display();
    grey_update();
#endif

    cancel_cpu_boost();

    return button;
}

enum mpeg_start_id  mpeg_start_menu(uint32_t duration)
{
    int menu_id;
    int result = 0;
    int menu_quit = 0;

    /* add the resume time to the menu display */
    char resume_str[32];
    char hms_str[32];
    struct hms hms;

    ts_to_hms(settings.resume_time, &hms);
    hms_format(hms_str, sizeof(hms_str), &hms);

    if (settings.enable_start_menu == 0)
    {
        rb->snprintf(resume_str, sizeof(resume_str), "Yes: %s", hms_str);

        struct opt_items resume_no_yes[2] =
        {
            { "No", -1 },
            { resume_str, -1 },
        };
        if (settings.resume_time == 0)
            return MPEG_START_RESTART;

        rb->set_option("Resume", &result, INT,
                       resume_no_yes, 2, NULL);

        if (result == 0)
        {
            settings.resume_time = 0;
            return MPEG_START_RESTART;
        }
        else
            return MPEG_START_RESUME;
    }

    rb->snprintf(resume_str, sizeof(resume_str), "Resume at: %s", hms_str);

    struct menu_item items[] =
    {
        [MPEG_START_RESTART] =
            { "Play from beginning", NULL },
        [MPEG_START_RESUME] =
            { resume_str, NULL },
        [MPEG_START_SEEK] =
            { "Set start time", NULL },
        [MPEG_START_QUIT] =
            { "Quit mpegplayer", NULL },
    };


    menu_id = menu_init(rb, items, sizeof(items) / sizeof(*items),
                        NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while(menu_quit == 0)
    {
        result = menu_show(menu_id);

        switch (result)
        {
            case MPEG_START_RESTART:
                settings.resume_time = 0;
                menu_quit = 1;
                break;
            case MPEG_START_RESUME:
                menu_quit = 1;
                break;
            case MPEG_START_SEEK:
            {
                if (!stream_can_seek())
                {
                    rb->splash(HZ, "Unavailable");
                    break;
                }

                bool vis = stream_show_vo(false);
                if (get_start_time(duration) == MPEG_SELECT)
                    menu_quit = 1;
                stream_show_vo(vis);
                break;
                }
            case MPEG_START_QUIT:
                menu_quit = 1;
                break;
            default:
                result = MPEG_START_QUIT;
                menu_quit = 1;
                break;
        }
    }

    menu_exit(menu_id);

    rb->lcd_clear_display();
    rb->lcd_update();

    return result;
}

void clear_resume_count(void)
{
    configfile_save(SETTINGS_FILENAME, config, ARRAYLEN(config),
                    SETTINGS_VERSION);

    settings.resume_count = 0;

    /* add this place holder so the count is above resume entries */
    configfile_update_entry(SETTINGS_FILENAME, "Resume count", 0);
}

enum mpeg_menu_id mpeg_menu(void)
{
    int menu_id;
    int result;
    int menu_quit=0;

    /* add the clear resume option to the menu display */
    char clear_str[32];
    rb->snprintf(clear_str, sizeof(clear_str),
                 "Clear all resumes: %u", settings.resume_count);

    struct menu_item items[] = {
        [MPEG_MENU_DISPLAY_SETTINGS] =
            { "Display Options", NULL },
        [MPEG_MENU_ENABLE_START_MENU] =
            { "Start menu", NULL },
        [MPEG_MENU_CLEAR_RESUMES] =
            { clear_str, NULL },
        [MPEG_MENU_QUIT] =
            { "Quit mpegplayer", NULL },
    };

    menu_id = menu_init(rb, items, ARRAYLEN(items),
                        NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (menu_quit == 0)
    {
        result=menu_show(menu_id);

        switch(result)
        {
            case MPEG_MENU_DISPLAY_SETTINGS:
                display_options();
                break;
            case MPEG_MENU_ENABLE_START_MENU:
                rb->set_option("Start menu",
                               &settings.enable_start_menu,
                               INT, enabledisable, 2, NULL);
                break;
            case MPEG_MENU_CLEAR_RESUMES:
                clear_resume_count();
                rb->snprintf(clear_str, sizeof(clear_str),
                           "Clear all resumes: %u", 0);
                break;
            case MPEG_MENU_QUIT:
            default:
                menu_quit=1;
                if (result == MENU_ATTACHED_USB)
                    result = MPEG_MENU_QUIT;
                break;
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
    settings.enable_start_menu = 1; /* Enable start menu */
    settings.resume_count = -1;
#if MPEG_OPTION_DITHERING_ENABLED
    settings.displayoptions = 0; /* No visual effects */
#endif

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
}

void save_settings(void)
{
    configfile_update_entry(SETTINGS_FILENAME, "Show FPS",
                            settings.showfps);
    configfile_update_entry(SETTINGS_FILENAME, "Limit FPS",
                            settings.limitfps);
    configfile_update_entry(SETTINGS_FILENAME, "Skip frames",
                            settings.skipframes);
    configfile_update_entry(SETTINGS_FILENAME, "Enable start menu",
                            settings.enable_start_menu);

    /* If this was a new resume entry then update the total resume count */
    if (configfile_update_entry(SETTINGS_FILENAME, settings.resume_filename,
                                settings.resume_time) == 0)
    {
        configfile_update_entry(SETTINGS_FILENAME, "Resume count",
                                ++settings.resume_count);
    }

#if MPEG_OPTION_DITHERING_ENABLED
    configfile_update_entry(SETTINGS_FILENAME, "Display options",
                            settings.displayoptions);
#endif
}
