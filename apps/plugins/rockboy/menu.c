/*********************************************************************/
/* menu.c - user menu for rockboy                                    */
/*                                                                   */
/* Note: this file only exposes one function: do_user_menu().        */
/*********************************************************************/

#include "lib/helper.h"
#include "button.h"
#include "rockmacros.h"
#include "mem.h"
#include "save.h"
#include "rtc-gb.h"
#include "pcm.h"
#include "emu.h"
#include "loader.h"

#define SLOT_COUNT  50
#define DESC_SIZE   20

/* load/save state function declarations */
static void do_opt_menu(void);
static void do_slot_menu(bool is_load);
static void munge_name(char *buf, size_t bufsiz);

/* directory ROM save slots belong in */
#define STATE_DIR ROCKBOX_DIR "/rockboy"

static int getbutton(char *text)
{
    int fw, fh;
    int button;
    rb->lcd_clear_display();
    rb->font_getstringsize(text, &fw, &fh,0);
    rb->lcd_putsxy(LCD_WIDTH/2-fw/2, LCD_HEIGHT/2-fh/2, text);
    rb->lcd_update();
    rb->sleep(30);

    while (rb->button_get(false) != BUTTON_NONE)
        rb->yield();

    while(true)
    {
        button = rb->button_get(true);
        button = button&(BUTTON_MAIN|BUTTON_REMOTE);

        return button;
    }
}

static void setupkeys(void)
{
    options.UP      = getbutton("Press Up");
    options.DOWN    = getbutton("Press Down");
    options.LEFT    = getbutton("Press Left");
    options.RIGHT   = getbutton("Press Right");

    options.A       = getbutton("Press A");
    options.B       = getbutton("Press B");

    options.START   = getbutton("Press Start");
    options.SELECT  = getbutton("Press Select");

    options.MENU    = getbutton("Press Menu");
}

/*
 * do_user_menu - create the user menu on the screen.
 *
 * Returns USER_MENU_QUIT if the user selected "quit", otherwise 
 * returns zero.
 */
int do_user_menu(void) {
    bool done=false;
    int selected=0, ret=0;
    int result;
    int time = 0;
    
#if CONFIG_RTC    
    time = rb->mktime(rb->get_time());
#endif

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

    backlight_use_settings();

    /* Clean out the button Queue */
    while (rb->button_get(false) != BUTTON_NONE) 
        rb->yield();

    MENUITEM_STRINGLIST(menu, "Rockboy Menu", NULL,
                        "Load Game", "Save Game",
                        "Options", "Reset", "Quit");

    rockboy_pcm_init();

    while(!done)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
            case 0: /* Load Game */
                do_slot_menu(true);
                break;
            case 1: /* Save Game */
                do_slot_menu(false);
                break;
            case 2: /* Options */
                do_opt_menu();
                break;
            case 3: /* Reset */
                emu_reset();
                done=true;
                break;
            case 4: /* Quit */
                ret = USER_MENU_QUIT;
                if(options.autosave) sn_save();
                done=true;
                break;
            default:
                done=true;
                break;
        }
    }

    rb->lcd_setfont(0); /* Reset the font */
    rb->lcd_clear_display(); /* Clear display for screen size changes */
    
    /* Keep the RTC in sync */
#if CONFIG_RTC
    time = (rb->mktime(rb->get_time()) - time) * 60;
#endif
    while (time-- > 0) rtc_tick();

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    /* ignore backlight time out */
    backlight_ignore_timeout();

    return ret;
}

/*
 * munge_name - munge a string into a filesystem-safe name
 */
static void munge_name(char *buf, const size_t bufsiz) {
    unsigned int i, max;

    /* check strlen */
    max = strlen(buf);
    max = (max < bufsiz) ? max : bufsiz;
  
    /* iterate over characters and munge them (if necessary) */
    for (i = 0; i < max; i++)
        if (!isalnum(buf[i]))
            buf[i] = '_';
}

/*
 * build_slot_path - build a path to an slot state file for this rom
 *
 * Note: uses rom.name.  Is there a safer way of doing this?  Like a ROM
 * checksum or something like that?
 */
static void build_slot_path(char *buf, size_t bufsiz, int slot_id) {
    char name_buf[17];

    /* munge state file name */
    strlcpy(name_buf, rom.name, sizeof(name_buf));
    munge_name(name_buf, strlen(name_buf));

    /* glom the whole mess together */
    snprintf(buf, bufsiz, "%s/%s-%d.rbs", STATE_DIR, name_buf, slot_id + 1);
}

/*
 * do_file - load or save game data in the given file
 *
 * Returns true on success and false on failure.
 *
 * @desc is a brief user-provided description of the state.
 * If no description is provided, set @desc to NULL.
 *
 */
static bool do_file(char *path, char *desc, bool is_load) {
    char desc_buf[DESC_SIZE];
    int fd, file_mode;
    
    /* set file mode */
    file_mode = is_load ? O_RDONLY : (O_WRONLY | O_CREAT);
  
    /* attempt to open file descriptor here */
    if ((fd = open(path, file_mode, 0666)) < 0)
        return false;

    /* load/save state */
    if (is_load)
    {
        /* load description */
        read(fd, desc_buf, sizeof(desc_buf));
    
        /* load state */
        loadstate(fd);
    
        /* print out a status message so the user knows the state loaded */
        rb->splashf(HZ * 1, "Loaded state from \"%s\"", path);
    }
    else
    {
        /* build description buffer */
        memset(desc_buf, 0, sizeof(desc_buf));
        if (desc)
            strlcpy(desc_buf, desc, sizeof(desc_buf));

        /* save state */
        write(fd, desc_buf, sizeof(desc_buf));
        savestate(fd);
    }
    
    /* close file descriptor */
    close(fd);

    /* return true (for success) */
    return true;
}

/* 
 * get information on the given slot
 */
static void slot_info(char *info_buf, size_t info_bufsiz, int slot_id,
                        bool number) {
    char buf[256];
    int fd;

    /* get slot file path */
    build_slot_path(buf, sizeof(buf), slot_id);

    /* attempt to open slot */
    if ((fd = open(buf, O_RDONLY)) >= 0)
    {
        /* this slot has a some data in it, read it */
        if (read(fd, buf, DESC_SIZE) == DESC_SIZE)
        {
            buf[DESC_SIZE] = '\0';
            strlcpy(info_buf, buf, info_bufsiz);
        }
        else if(number)
            strlcpy(info_buf, "ERROR", info_bufsiz);

        close(fd);
    }
    else if(number)
    {
        /* if we couldn't open the file, then the slot is empty */
        strlcpy(info_buf, "<Empty>", info_bufsiz);
    }
}

/*
 * do_slot - load or save game data in the given slot
 *
 * Returns true on success and false on failure.
 */
static bool do_slot(int slot_id, bool is_load) {
    char path_buf[256], desc_buf[DESC_SIZE];
  
    /* build slot filename, clear desc buf */
    build_slot_path(path_buf, sizeof(path_buf), slot_id);
    memset(desc_buf, 0, sizeof(desc_buf));

    /* if we're saving to a slot, then get a brief description */
    if (!is_load)
    {
        slot_info(desc_buf, sizeof(desc_buf), slot_id, false);
        if ( rb->kbd_input(desc_buf, sizeof(desc_buf)) < 0 )
            return false;
        if ( !strlen(desc_buf) )
            strlcpy(desc_buf, "Untitled", sizeof(desc_buf));
    }

    /* load/save file */
    return do_file(path_buf, desc_buf, is_load);
}

/* 
 * slot_get_name
 */
static const char* slot_get_name(int selected_item, void * data,
                                 char * buffer, size_t buffer_len)
{
    const char (*items)[DESC_SIZE] = data;
    snprintf(buffer, buffer_len, "%d. %s",
                selected_item + 1, items[selected_item]);
    return buffer;
}

/*
 * list_action_callback
 */
static int list_action_callback(int action, struct gui_synclist *lists)
{
    (void) lists;
    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

/*
 * do_slot_menu - prompt the user for a load/save memory slot
 */
static void do_slot_menu(bool is_load) {
    bool done=false;
    char items[SLOT_COUNT][DESC_SIZE];
    int result;
    int i;
    struct simplelist_info info;

    /* create menu items */
    for (i = 0; i < SLOT_COUNT; i++)
        slot_info(items[i], sizeof(*items), i, true);

    rb->simplelist_info_init(&info, NULL, SLOT_COUNT, (void *)items);
    info.get_name = slot_get_name;
    info.action_callback = list_action_callback;

    while(!done)
    {
        if(rb->simplelist_show_list(&info))
            break;

        result = info.selection;
        if (result<SLOT_COUNT && result >= 0 )
            done = do_slot(result, is_load);
        else
            done = true;
    }
}

static void do_opt_menu(void)
{
    bool done=false;
    int selected=0;
    int result;

    static const struct opt_items onoff[2] = {
        { "Off", -1 },
        { "On" , -1 },
    };

    static const struct opt_items stats[3] = {
        { "Off", -1 },
        { "Short" , -1 },
        { "Full" , -1 },
    };

    static const struct opt_items frameskip[]= {
        { "0 Max", -1 },
        { "1 Max", -1 },
        { "2 Max", -1 },
        { "3 Max", -1 },
        { "4 Max", -1 },
        { "5 Max", -1 },
        { "6 Max", -1 },
        { "7 Max", -1 },
        { "8 Max", -1 },
        { "9 Max", -1 },
        { "10 Max", -1 },
        { "11 Max", -1 },
        { "12 Max", -1 },
        { "13 Max", -1 },
        { "14 Max", -1 },
        { "15 Max", -1 },
        { "16 Max", -1 },
        { "17 Max", -1 },
        { "18 Max", -1 },
        { "19 Max", -1 },
        { "20 Max", -1 },
    };
    
#ifdef HAVE_LCD_COLOR
    static const struct opt_items rotate[] = {
        { "No rotation", -1 },
        { "Rotate Right" , -1 },
        { "Rotate Left" , -1 },
    };

    static const struct opt_items scaling[]= {
        { "Scaled", -1 },
        { "Scaled - Maintain Ratio", -1 },
#if (LCD_WIDTH>=160) && (LCD_HEIGHT>=144)
        { "Unscaled", -1 },
#endif
    };

    static const struct opt_items palette[]= {
        { "Brown (Default)", -1 },
        { "Gray", -1 },
        { "Light Gray", -1 },
        { "Multi-Color 1", -1 },
        { "Multi-Color 2", -1 },
        { "Adventure Island", -1 },
        { "Adventure Island 2", -1 },
        { "Balloon Kid", -1 },
        { "Batman", -1 },
        { "Batman: Return of Joker", -1 },
        { "Bionic Commando", -1 },
        { "Castlvania Adventure", -1 },
        { "Donkey Kong Land", -1 },
        { "Dr. Mario", -1 },
        { "Kirby", -1 },
        { "Metroid", -1 },
        { "Zelda", -1 },
    };
#endif

    MENUITEM_STRINGLIST(menu, "Options", NULL,
                        "Max Frameskip", "Autosave", "Sound", "Volume", 
                        "Stats", "Set Keys (Buggy)",
#ifdef HAVE_LCD_COLOR
                        "Screen Size", "Screen Rotate", "Set Palette",
#endif
                        );

    options.dirty=1; /* Assume that the settings have been changed */

    struct viewport *parentvp = NULL;
    const struct settings_list* vol = rb->find_setting(&rb->global_settings->volume, NULL);

    while(!done)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);

        switch (result)
        {
            case 0: /* Frameskip */
                rb->set_option("Max Frameskip", &options.maxskip, INT, frameskip, 
                    sizeof(frameskip)/sizeof(*frameskip), NULL );
                break;
            case 1: /* Autosave */
                rb->set_option("Autosave", &options.autosave, INT, onoff, 2, NULL );
                break;
            case 2: /* Sound */
                if(options.sound>1) options.sound=1;
                rb->set_option("Sound", &options.sound, INT, onoff, 2, NULL );
                if(options.sound) sound_dirty();
                break;
            case 3: /* Volume */
                rb->option_screen((struct settings_list*)vol, parentvp, false, "Volume");
                break;
            case 4: /* Stats */
                rb->set_option("Stats", &options.showstats, INT, stats, 3, NULL );
                break;
            case 5: /* Keys */
                setupkeys();
                break;
#ifdef HAVE_LCD_COLOR
            case 6: /* Screen Size */
                rb->set_option("Screen Size", &options.scaling, INT, scaling,
                    sizeof(scaling)/sizeof(*scaling), NULL );
                setvidmode();
                break;
            case 7: /* Screen rotate */
                rb->set_option("Screen Rotate", &options.rotate, INT, rotate,
                    sizeof(rotate)/sizeof(*rotate), NULL );
                setvidmode();
                break;
            case 8: /* Palette */
                rb->set_option("Set Palette", &options.pal, INT, palette, 17, NULL );
                set_pal();
                break;
#endif
            default:
                done=true;
                break;
        }
    }
}
