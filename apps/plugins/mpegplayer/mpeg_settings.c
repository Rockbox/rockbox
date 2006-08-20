#include "plugin.h"
#include "lib/configfile.h"

#include "mpeg_settings.h"

extern struct plugin_api* rb;

struct mpeg_settings settings;
static struct mpeg_settings old_settings;

#define SETTINGS_VERSION 1
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "mpegplayer.cfg"

static char* showfps_options[] = {"No", "Yes"};
static char* limitfps_options[] = {"No", "Yes"};
static char* skipframes_options[] = {"No", "Yes"};

static struct configdata config[] =
{
   {TYPE_ENUM, 0, 2, &settings.showfps, "Show FPS", showfps_options, NULL},
   {TYPE_ENUM, 0, 2, &settings.limitfps, "Limit FPS", limitfps_options, NULL},
   {TYPE_ENUM, 0, 2, &settings.skipframes, "Skip frames", skipframes_options, NULL},
};

bool mpeg_menu(void)
{
    int m;
    int result;
    int menu_quit=0;

    static const struct opt_items noyes[2] = {
        { "No", -1 },
        { "Yes", -1 },
    };

    static const struct menu_item items[] = {
        { "Display FPS", NULL },
        { "Limit FPS", NULL },
        { "Skip frames", NULL },
        { "Quit mpegplayer", NULL },
    };
    
    m = rb->menu_init(items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    rb->button_clear_queue();

    while (!menu_quit) {
        result=rb->menu_show(m);

        switch(result)
        {
            case 0: /* Show FPS */
                rb->set_option("Display FPS",&settings.showfps,INT, 
                               noyes, 2, NULL);
                break;
            case 1: /* Limit FPS */
                rb->set_option("Limit FPS",&settings.limitfps,INT, 
                               noyes, 2, NULL);
                break;
            case 2: /* Skip frames */
                rb->set_option("Skip frames",&settings.skipframes,INT, 
                               noyes, 2, NULL);
                break;
            default:
                menu_quit=1;
                break;
        }
    }

    rb->menu_exit(m);

    rb->lcd_clear_display();
    rb->lcd_update();

    return (result==3);
}


void init_settings(void)
{
    /* Set the default settings */
    settings.showfps = 0;     /* Do not show FPS */
    settings.limitfps = 0;    /* Do not limit FPS */
    settings.skipframes = 0;  /* Do not skip frames */

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
