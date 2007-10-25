#include "plugin.h"
#include "lib/configfile.h"
#include "lib/oldmenuapi.h"

#include "mpeg_settings.h"

extern struct plugin_api* rb;

struct mpeg_settings settings;

ssize_t seek_PTS(int in_file, int startTime, int accept_button);
void display_thumb(int in_file);

#ifndef HAVE_LCD_COLOR
void gray_show(bool enable);
#endif

#define SETTINGS_VERSION 2
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "mpegplayer.cfg"

enum slider_state_t {state0, state1, state2, 
                     state3, state4, state5} slider_state;
                     
volatile long thumbDelayTimer;

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define MPEG_SELECT      BUTTON_ON
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_SCROLL_DOWN BUTTON_UP
#define MPEG_SCROLL_UP   BUTTON_DOWN
#define MPEG_EXIT        BUTTON_OFF

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define MPEG_SELECT      BUTTON_PLAY
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_SCROLL_DOWN BUTTON_UP
#define MPEG_SCROLL_UP   BUTTON_DOWN
#define MPEG_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_SCROLL_DOWN BUTTON_SCROLL_FWD
#define MPEG_SCROLL_UP   BUTTON_SCROLL_BACK
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
#define MPEG_SCROLL_UP   BUTTON_SCROLL_UP
#define MPEG_SCROLL_DOWN BUTTON_SCROLL_DOWN
#define MPEG_LEFT        BUTTON_LEFT
#define MPEG_RIGHT       BUTTON_RIGHT
#define MPEG_EXIT        BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define MPEG_SELECT      BUTTON_SELECT
#define MPEG_SCROLL_UP   BUTTON_SCROLL_UP
#define MPEG_SCROLL_DOWN BUTTON_SCROLL_DOWN
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
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
        [MPEG_OPTION_DITHERING] =
            { "Dithering", NULL },
#endif /* #ifdef TOSHIBA_GIGABEAT_F */
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
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
            case MPEG_OPTION_DITHERING:
                result = (settings.displayoptions & LCD_YUV_DITHER) ? 1 : 0;
                rb->set_option("Dithering", &result, INT, noyes, 2, NULL);
                settings.displayoptions = (settings.displayoptions & ~LCD_YUV_DITHER)
                  | ((result != 0) ? LCD_YUV_DITHER : 0);
                rb->lcd_yuv_set_options(settings.displayoptions);
                break;
#endif /* #ifdef TOSHIBA_GIGABEAT_F */
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

void draw_slider(int slider_ypos, int max_val, int current_val)
{
    int slider_margin = LCD_WIDTH*12/100; /* 12% */
    int slider_width = LCD_WIDTH-(slider_margin*2);
    char resume_str[32];

    /* max_val and current_val are in half minutes
       determine value .0 or .5 to display */
    int max_hol = max_val/2;
    int max_rem = (max_val-(max_hol*2))*5;
    int current_hol = current_val/2;
    int current_rem = (current_val-(current_hol*2))*5;

    rb->snprintf(resume_str, sizeof(resume_str), "0.0");
    rb->lcd_putsxy(slider_margin, slider_ypos, resume_str);

    rb->snprintf(resume_str, sizeof(resume_str), "%u.%u", max_hol, max_rem);
    rb->lcd_putsxy(LCD_WIDTH-slider_margin-25, slider_ypos, resume_str);
    
    rb->lcd_drawrect(slider_margin, slider_ypos+17, slider_width, 8);
    rb->lcd_fillrect(slider_margin, slider_ypos+17,
                     current_val*slider_width/max_val, 8);

    rb->snprintf(resume_str, sizeof(resume_str), "%u.%u", current_hol,
                 current_rem);
    rb->lcd_putsxy(slider_margin+(current_val*slider_width/max_val)-16,
                   slider_ypos+29, resume_str);

    rb->lcd_update_rect(0, slider_ypos, LCD_WIDTH, LCD_HEIGHT-slider_ypos);
}

int get_start_time(int play_time, int in_file)
{
    int seek_quit = 0;
    int button = 0;
    int resume_time = settings.resume_time;
    int slider_ypos = LCD_HEIGHT-45;
    int seek_return;
    
    slider_state = state0;
    thumbDelayTimer = *(rb->current_tick);
    rb->lcd_clear_display();
    rb->lcd_update();
    
    while(seek_quit == 0)
    {
        button = rb->button_get(false);
        switch (button)
        {
#if (CONFIG_KEYPAD == GIGABEAT_PAD)   || \
    (CONFIG_KEYPAD == SANSA_E200_PAD) || \
    (CONFIG_KEYPAD == SANSA_C200_PAD)
            case MPEG_DOWN:
            case MPEG_DOWN | BUTTON_REPEAT:
                if ((resume_time -= 20) < 0)
                    resume_time = 0;
                slider_state = state0;
                thumbDelayTimer = *(rb->current_tick);
                break;
            case MPEG_UP:
            case MPEG_UP | BUTTON_REPEAT:
                if ((resume_time += 20) > play_time)
                    resume_time = play_time;
                slider_state = state0;
                thumbDelayTimer = *(rb->current_tick);
                break;
#endif
            case MPEG_LEFT:
            case MPEG_LEFT | BUTTON_REPEAT:
            case MPEG_SCROLL_UP:
            case MPEG_SCROLL_UP | BUTTON_REPEAT:
                if (--resume_time < 0)
                    resume_time = 0;
                slider_state = state0;
                thumbDelayTimer = *(rb->current_tick);
                break;
            case MPEG_RIGHT:
            case MPEG_RIGHT | BUTTON_REPEAT:
            case MPEG_SCROLL_DOWN:
            case MPEG_SCROLL_DOWN | BUTTON_REPEAT:
                if (++resume_time > play_time)
                    resume_time = play_time;
                slider_state = state0;
                thumbDelayTimer = *(rb->current_tick);
                break;
            case MPEG_SELECT:
                settings.resume_time = resume_time;
            case MPEG_EXIT:
                seek_quit = 1;
                break;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    seek_quit = 1;
                break;
        }
        
        rb->yield();
        
        switch(slider_state)
        {
            case state0:
                rb->lcd_clear_display();
                rb->lcd_update();
#ifdef HAVE_LCD_COLOR
                if (resume_time > 0)
                    rb->splash(0, "Loading...");
#endif
                slider_state = state1;
                break;
            case state1:
                if (*(rb->current_tick) - thumbDelayTimer > 75)
                    slider_state = state2;
                if (resume_time == 0)
                {
                    seek_return = 0;
                    slider_state = state5;
                }
                draw_slider(slider_ypos, play_time, resume_time);
                break;
            case state2:
                if ( (seek_return = seek_PTS(in_file, resume_time, 1)) >= 0)
                    slider_state = state3;
                else if (seek_return == -101)
                {
                    slider_state = state0;
                    thumbDelayTimer = *(rb->current_tick);
                } 
                else 
                    slider_state = state4;
                break;
            case state3:
                display_thumb(in_file);
                draw_slider(slider_ypos, play_time, resume_time);
                slider_state = state4;
                break;
            case state4:
                draw_slider(slider_ypos, play_time, resume_time);
                slider_state = state5;
                break;
            case state5:
                break;
        }
    }

    return button;
}

enum mpeg_start_id  mpeg_start_menu(int play_time, int in_file)
{
    int menu_id;
    int result = 0;
    int menu_quit = 0;
    
    /* add the resume time to the menu display */
    char resume_str[32];
    int time_hol  = (int)(settings.resume_time/2);
    int time_rem = ((settings.resume_time%2)==0) ? 0 : 5;

    if (settings.enable_start_menu == 0)
    {
        rb->snprintf(resume_str, sizeof(resume_str),
                     "Yes (min): %d.%d", time_hol, time_rem);

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

    rb->snprintf(resume_str, sizeof(resume_str),
                 "Resume time (min): %d.%d", time_hol, time_rem);

    struct menu_item items[] =
    {
        [MPEG_START_RESTART] =
            { "Play from beginning", NULL },
        [MPEG_START_RESUME] =
            { resume_str, NULL },
        [MPEG_START_SEEK] =
            { "Set start time (min)", NULL },
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
#ifndef HAVE_LCD_COLOR
                gray_show(true);
#endif
                if (get_start_time(play_time, in_file) == MPEG_SELECT)
                    menu_quit = 1;
#ifndef HAVE_LCD_COLOR
                gray_show(false);
#endif
                break;
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

    return result;
}

void clear_resume_count(void)
{
    configfile_save(SETTINGS_FILENAME, config,
                    sizeof(config)/sizeof(*config),
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
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
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

#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
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

    rb->strcpy(settings.resume_filename, filename);

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

#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200) || defined(SANSA_C200)
    configfile_update_entry(SETTINGS_FILENAME, "Display options", 
                            settings.displayoptions);
#endif
}
