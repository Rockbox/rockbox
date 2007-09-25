#include "plugin.h"
#include "lib/configfile.h"
#include "lib/oldmenuapi.h"

#include "mpeg_settings.h"

extern struct plugin_api* rb;

struct mpeg_settings settings;
static struct mpeg_settings old_settings;

#define SETTINGS_VERSION 2
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "mpegplayer.cfg"

static struct configdata config[] =
{
   {TYPE_ENUM, 0, 2, &settings.showfps, "Show FPS",
        (char *[]){ "No", "Yes" }, NULL},
   {TYPE_ENUM, 0, 2, &settings.limitfps, "Limit FPS",
        (char *[]){ "No", "Yes" }, NULL},
   {TYPE_ENUM, 0, 2, &settings.skipframes, "Skip frames",
        (char *[]){ "No", "Yes" }, NULL},
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
   {TYPE_INT, 0, INT_MAX, &settings.displayoptions, "Display options",
        NULL, NULL},
#endif
};

enum mpeg_menu_ids
{
    __MPEG_OPTION_START = -1,
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    MPEG_OPTION_DISPLAY_SETTINGS,
#endif
    MPEG_OPTION_DISPLAY_FPS,
    MPEG_OPTION_LIMIT_FPS,
    MPEG_OPTION_SKIP_FRAMES,
    MPEG_OPTION_QUIT,
};

static const struct opt_items noyes[2] = {
    { "No", -1 },
    { "Yes", -1 },
};

#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
static bool set_option_dithering(void)
{
    int val = (settings.displayoptions & LCD_YUV_DITHER) ? 1 : 0;
    rb->set_option("Dithering", &val, INT, noyes, 2, NULL);
    settings.displayoptions = (settings.displayoptions & ~LCD_YUV_DITHER)
        | ((val != 0) ? LCD_YUV_DITHER : 0);
    rb->lcd_yuv_set_options(settings.displayoptions);
    return false;
}

static void display_options(void)
{
    static const struct menu_item items[] = {
        { "Dithering", set_option_dithering },
    };

    int m = menu_init(rb, items, ARRAYLEN(items),
                          NULL, NULL, NULL, NULL);
    menu_run(m);
    menu_exit(m);
}
#endif /* #ifdef TOSHIBA_GIGABEAT_F */

bool mpeg_menu(void)
{
    int m;
    int result;
    int menu_quit=0;

    static const struct menu_item items[] = {
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
        [MPEG_OPTION_DISPLAY_SETTINGS] =
            { "Display Options", NULL },
#endif
        [MPEG_OPTION_DISPLAY_FPS] =
            { "Display FPS", NULL },
        [MPEG_OPTION_LIMIT_FPS] =
            { "Limit FPS", NULL },
        [MPEG_OPTION_SKIP_FRAMES] =
            { "Skip frames", NULL },
        [MPEG_OPTION_QUIT] =
            { "Quit mpegplayer", NULL },
    };

    m = menu_init(rb, items, ARRAYLEN(items), NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=menu_show(m);

        switch(result)
        {
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
            case MPEG_OPTION_DISPLAY_SETTINGS:
                display_options();
                break;
#endif
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
            case MPEG_OPTION_QUIT:
            default:
                menu_quit=1;
                if (result == MENU_ATTACHED_USB)
                    result = MPEG_OPTION_QUIT;
                break;
        }
    }

    menu_exit(m);

    rb->lcd_clear_display();
    rb->lcd_update();

    return (result==MPEG_OPTION_QUIT);
}


void init_settings(void)
{
    /* Set the default settings */
    settings.showfps = 0;     /* Do not show FPS */
    settings.limitfps = 1;    /* Limit FPS */
    settings.skipframes = 1;  /* Skip frames */
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    settings.displayoptions = 0; /* No visual effects */
#endif

    configfile_init(rb);

    if (configfile_load(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION
                       ) < 0)
    {
        /* If the loading failed, save a new config file (as the disk is
           already spinning) */
        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }

    /* Keep a copy of the saved version of the settings - so we can check if
       the settings have changed when we quit */
    old_settings = settings;
#if defined(TOSHIBA_GIGABEAT_F) || defined(SANSA_E200)
    rb->lcd_yuv_set_options(settings.displayoptions);
#endif
}

void save_settings(void)
{
    /* Save the user settings if they have changed */
    if (rb->memcmp(&settings,&old_settings,sizeof(settings))!=0) {
        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);

        /* Store the settings in old_settings - to check for future changes */
        old_settings = settings;
    }
}
