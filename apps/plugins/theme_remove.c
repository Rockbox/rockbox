/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "lib/configfile.h"

PLUGIN_HEADER

/* taken from apps/gui/wps_parser.c */
#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"

#define CONFIG_FILENAME "theme_remove.cfg"
#define LOG_FILENAME    "/theme_remove_log.txt"
#define RB_FONTS_CONFIG VIEWERS_DIR "/rockbox-fonts.config"

enum remove_option {
    ALWAYS_REMOVE,
    NEVER_REMOVE,
    REMOVE_IF_NOT_USED,
    ASK_FOR_REMOVAL,
    NUM_REMOVE_OPTION
};

struct remove_setting {
    const char *name;
    const char *prefix, *suffix;
    char value[MAX_PATH];
    int  option;
    int  (*func)(struct remove_setting *);
    bool used;
};

static int remove_wps(struct remove_setting *);
#ifdef HAVE_LCD_BITMAP
static int remove_icons(struct remove_setting *setting);
#endif

enum remove_settings {
#ifdef HAVE_LCD_BITMAP
    REMOVE_FONT,
#endif
    REMOVE_WPS,
#ifdef HAVE_LCD_BITMAP
    REMOVE_SBS,
#endif
#ifdef HAVE_REMOTE_LCD
    REMOVE_RWPS,
    REMOVE_RSBS,
#endif
#if LCD_DEPTH > 1
    REMOVE_BACKDROP,
#endif
#ifdef HAVE_LCD_BITMAP
    REMOVE_ICON,
    REMOVE_VICON,
#endif
#ifdef HAVE_REMOTE_LCD
    REMOVE_RICON,
    REMOVE_RVICON,
#endif
#ifdef HAVE_LCD_COLOR
    REMOVE_COLOURS,
#endif
    NUM_REMOVE_ITEMS
};

static bool create_log = true;
static struct remove_setting remove_list[NUM_REMOVE_ITEMS] = {
#ifdef HAVE_LCD_BITMAP
    [REMOVE_FONT] = { "font", FONT_DIR "/", ".fnt", "",
        ASK_FOR_REMOVAL, NULL, false },
#endif
    [REMOVE_WPS] = { "wps", WPS_DIR "/", ".wps", "",
        REMOVE_IF_NOT_USED, remove_wps, false },
#ifdef HAVE_LCD_BITMAP
    [REMOVE_SBS] = { "sbs", SBS_DIR "/", ".sbs", "",
        REMOVE_IF_NOT_USED, remove_wps, false },
#endif
#ifdef HAVE_REMOTE_LCD
    [REMOVE_RWPS] = { "rwps", WPS_DIR "/", ".rwps", "",
        REMOVE_IF_NOT_USED, remove_wps, false },
    [REMOVE_RSBS] = { "rsbs", SBS_DIR "/", ".rsbs", "",
        REMOVE_IF_NOT_USED, remove_wps, false },
#endif
#if LCD_DEPTH > 1
    [REMOVE_BACKDROP] = { "backdrop", BACKDROP_DIR "/", ".bmp", "",
        REMOVE_IF_NOT_USED, NULL, false },
#endif
#ifdef HAVE_LCD_BITMAP
    [REMOVE_ICON] = { "iconset", ICON_DIR "/", ".bmp", "",
        ASK_FOR_REMOVAL, NULL, false },
    [REMOVE_VICON] = { "viewers iconset", ICON_DIR "/", ".bmp", "",
        ASK_FOR_REMOVAL, remove_icons, false },
#endif
#ifdef HAVE_REMOTE_LCD
    [REMOVE_RICON] = { "remote iconset", ICON_DIR "/", ".bmp", "",
        ASK_FOR_REMOVAL, NULL, false },
    [REMOVE_RVICON] = { "remote viewers iconset", ICON_DIR "/", ".bmp", "",
        ASK_FOR_REMOVAL, NULL, false },
#endif
#ifdef HAVE_LCD_COLOR
    [REMOVE_COLOURS] = { "filetype colours", THEME_DIR "/", ".colours", "",
        ASK_FOR_REMOVAL, NULL, false },
#endif
};
static char *option_names[NUM_REMOVE_OPTION] = {
    "always", "never", "not used", "ask",
};
static struct configdata config[] = {
#ifdef HAVE_LCD_BITMAP
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_FONT].option },
        "remove font", option_names },
#endif
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_WPS].option },
        "remove wps", option_names },
#ifdef HAVE_LCD_BITMAP
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_SBS].option },
        "remove sbs", option_names },
#endif
#ifdef HAVE_REMOTE_LCD
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_RWPS].option },
        "remove rwps", option_names },
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_RSBS].option },
        "remove rsbs", option_names },
#endif
#if LCD_DEPTH > 1
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_BACKDROP].option },
        "remove backdrop", option_names },
#endif
#ifdef HAVE_LCD_BITMAP
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_ICON].option },
        "remove iconset", option_names },
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_VICON].option },
        "remove viconset", option_names },
#endif
#ifdef HAVE_REMOTE_LCD
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_RICON].option },
        "remove riconset", option_names },
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_RVICON].option },
        "remove rviconset", option_names },
#endif
#ifdef HAVE_LCD_COLOR
    { TYPE_INT, 0, NUM_REMOVE_OPTION,
        { .int_p = &remove_list[REMOVE_COLOURS].option },
        "remove colours", option_names },
#endif
    {TYPE_BOOL, 0, 1, { .bool_p = &create_log },
        "create log", NULL},
};
static const int nb_config = sizeof(config)/sizeof(*config);
static char themefile[MAX_PATH];
static int log_fd = -1;

static int show_mess(const char *text, const char *file)
{
    static char buf[MAX_PATH*2];

    if (file)
        rb->snprintf(buf, sizeof(buf), "%s: %s", text, file);
    else
        rb->snprintf(buf, sizeof(buf), "%s", text);

    DEBUGF("%s\n", buf);
    if (log_fd >= 0)
        rb->fdprintf(log_fd, "%s\n", buf);

    rb->splash(0, buf);
    rb->sleep(HZ/4);

    return 0;
}

/* set full path of file. */
static void set_file_name(char *buf, const char*file,
                          struct remove_setting *setting)
{
    int len1, len2;
    if (rb->strncasecmp(file, setting->prefix, rb->strlen(setting->prefix)))
        rb->snprintf(buf, MAX_PATH, "%s%s", setting->prefix, file);
    else
        rb->strlcpy(buf, file, MAX_PATH);
    len1 = rb->strlen(buf);
    len2 = rb->strlen(setting->suffix);
    if (rb->strcasecmp(buf+len1-len2, setting->suffix))
        rb->strlcpy(&buf[len1], setting->suffix, MAX_PATH-len1);
}

/* taken from apps/onplay.c */
/* helper function to remove a non-empty directory */
static int remove_dir(char* dirname, int len)
{
    int result = 0;
    DIR* dir;
    int dirlen = rb->strlen(dirname);

    dir = rb->opendir(dirname);
    if (!dir)
        return -1; /* open error */

    while (true)
    {
        struct dirent* entry;
        /* walk through the directory content */
        entry = rb->readdir(dir);
        if (!entry)
            break;

        dirname[dirlen] ='\0';

        /* append name to current directory */
        rb->snprintf(dirname+dirlen, len-dirlen, "/%s", entry->d_name);
        if (entry->attribute & ATTR_DIRECTORY)
        {
            /* remove a subdirectory */
            if (!rb->strcmp((char *)entry->d_name, ".") ||
                !rb->strcmp((char *)entry->d_name, ".."))
                continue; /* skip these */

            result = remove_dir(dirname, len); /* recursion */
            if (result)
                break;
        }
        else
        {
            /* remove a file */
            result = rb->remove(dirname);
        }
        if (ACTION_STD_CANCEL == rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK))
        {
            show_mess("Canceled", NULL);
            result = -1;
            break;
        }
    }
    rb->closedir(dir);

    if (!result)
    {   /* remove the now empty directory */
        dirname[dirlen] = '\0'; /* terminate to original length */

        result = rb->rmdir(dirname);
        show_mess("Removed", dirname);
    }

    return result;
}

static int remove_wps(struct remove_setting *setting)
{
    char bmpdir[MAX_PATH];
    char *p;
    rb->strcpy(bmpdir, setting->value);
    p = rb->strrchr(bmpdir, '.');
    if (p) *p = 0;
    if (!rb->dir_exists(bmpdir))
        return 0;
    return remove_dir(bmpdir, MAX_PATH);
}

#ifdef HAVE_LCD_BITMAP
static int remove_icons(struct remove_setting *setting)
{
    char path[MAX_PATH];
    char *p;
    rb->strcpy(path, setting->value);
    p = rb->strrchr(path, '.');
    rb->strlcpy(p, ".icons", path+MAX_PATH-p);

    if (!rb->file_exists(path))
    {
        return 0;
    }
    if (rb->remove(path))
    {
        show_mess("Failed", path);
        return 1;
    }
    show_mess("Removed", path);
    return 0;
}
#endif

#ifdef HAVE_LCD_BITMAP
static char font_file[MAX_PATH];
#endif
static bool is_deny_file(const char *file)
{
    const char *deny_files[] = {
        WPS_DEFAULTCFG,
        RWPS_DEFAULTCFG,
#ifdef HAVE_LCD_BITMAP
        font_file,
#endif
        NULL
    };
    const char **p = deny_files;
    while ( *p )
    {
        if (!rb->strcmp(file, *p))
            return true;
        p++;
    }
    return false;
}

static void check_whether_used_in_setting(void)
{
    const char *setting_files[] = {
#ifdef HAVE_LCD_BITMAP
        rb->global_settings->font_file,
#endif
        rb->global_settings->wps_file,
#ifdef HAVE_LCD_BITMAP
        rb->global_settings->sbs_file,
#endif
#ifdef HAVE_REMOTE_LCD
        rb->global_settings->rwps_file,
        rb->global_settings->rsbs_file,
#endif
#if LCD_DEPTH > 1
        rb->global_settings->backdrop_file,
#endif
#ifdef HAVE_LCD_BITMAP
        rb->global_settings->icon_file,
        rb->global_settings->viewers_icon_file,
#endif
#ifdef HAVE_REMOTE_LCD
        rb->global_settings->remote_icon_file,
        rb->global_settings->remote_viewers_icon_file,
#endif
#ifdef HAVE_LCD_COLOR
        rb->global_settings->colors_file,
#endif
    };
    char tempfile[MAX_PATH];
    int i;
    for (i=0; i<NUM_REMOVE_ITEMS; i++)
    {
        struct remove_setting *setting = &remove_list[i];
        if (setting->value[0])
        {
            set_file_name(tempfile, setting_files[i], setting);
            if (!rb->strcasecmp(tempfile, setting->value))
                setting->used = true;
        }
    }
}
static void check_whether_used_in_file(const char *cfgfile)
{
    char line[MAX_PATH];
    char settingfile[MAX_PATH];
    char *p;
    int fd;
    char *name, *value;
    int i;

    if (!rb->strcasecmp(themefile, cfgfile))
        return;
    fd = rb->open(cfgfile, O_RDONLY);
    if (fd < 0)
        return;
    while (rb->read_line(fd, line, sizeof(line)) > 0)
    {
        if (!rb->settings_parseline(line, &name, &value))
            continue;
        /* remove trailing spaces. */
        p = value+rb->strlen(value)-1;
        while (*p == ' ') *p-- = 0;
        if (*value == 0 || !rb->strcmp(value, "-"))
            continue;
        for (i=0; i<NUM_REMOVE_ITEMS; i++)
        {
            struct remove_setting *setting = &remove_list[i];
            if (!rb->strcmp(name, setting->name))
            {
                if (setting->value[0])
                {
                    set_file_name(settingfile, value, setting);
                    if (!rb->strcasecmp(settingfile, setting->value))
                        setting->used = true;
                }
                break;
            }
        }
    }
    rb->close(fd);
}
static void check_whether_used(void)
{
    char cfgfile[MAX_PATH];
    DIR *dir;

    check_whether_used_in_setting();
#ifdef HAVE_LCD_BITMAP
    /* mark font files come from rockbox-font.zip as used and don't remove
     * them automatically as themes may depend on those fonts. */
    if (remove_list[REMOVE_FONT].option == REMOVE_IF_NOT_USED)
        check_whether_used_in_file(RB_FONTS_CONFIG);
#endif

    dir = rb->opendir(THEME_DIR);
    if (!dir)
        return; /* open error */

    while (true)
    {
        struct dirent* entry;
        char *p;
        int i;
        /* walk through the directory content */
        entry = rb->readdir(dir);
        if (!entry)
            break;
        p = rb->strrchr(entry->d_name, '.');
        if (!p || rb->strcmp(p, ".cfg"))
            continue;

        rb->snprintf(cfgfile, MAX_PATH, "%s/%s", THEME_DIR, entry->d_name);
        check_whether_used_in_file(cfgfile);
        /* break the loop if all files need to be checked in the theme
         * turned out to be used. */
        for (i = 0; i < NUM_REMOVE_ITEMS; i++)
        {
            struct remove_setting *setting = &remove_list[i];
            if (setting->option == REMOVE_IF_NOT_USED)
            {
                if (setting->value[0] && !setting->used)
                    break;
            }
        }
        if (i == NUM_REMOVE_ITEMS)
            break;
    }
    rb->closedir(dir);
}

static int remove_file(struct remove_setting *setting)
{
    if (!rb->file_exists(setting->value))
    {
        show_mess("Doesn't exist", setting->value);
        return 0;
    }
    if (is_deny_file(setting->value))
    {
        show_mess("Denied", setting->value);
        return 0;
    }
    switch (setting->option)
    {
        case ALWAYS_REMOVE:
            break;
        case NEVER_REMOVE:
            show_mess("Skipped", setting->value);
            return 0;
            break;
        case REMOVE_IF_NOT_USED:
            if (setting->used)
            {
                show_mess("Used", setting->value);
                return 0;
            }
            break;
        case ASK_FOR_REMOVAL:
        default:
        {
            const char *message_lines[] = { "Delete?", setting->value };
            const struct text_message text_message = { message_lines, 2 };
            if (rb->gui_syncyesno_run(&text_message, NULL, NULL) != YESNO_YES)
            {
                show_mess("Skipped", setting->value);
                return 0;
            }
        }
            break;
    }
    if (rb->remove(setting->value))
    {
        show_mess("Failed", setting->value);
        return -1;
    }
    if (setting->func && setting->func(setting))
        return -1;
    show_mess("Removed", setting->value);
    return 1;
}
static int remove_theme(void)
{
    static char line[MAX_PATH];
    int fd;
    int i, num_removed = 0;
    char *name, *value;
    bool needs_to_check_whether_used = false;

    /* initialize for safe */
    for (i=0; i<NUM_REMOVE_ITEMS; i++)
        remove_list[i].value[0] = 0;

    /* load settings */
    fd = rb->open(themefile, O_RDONLY);
    if (fd < 0) return fd;
    while (rb->read_line(fd, line, sizeof(line)) > 0)
    {
        if (!rb->settings_parseline(line, &name, &value))
            continue;
        /* remove trailing spaces. */
        char *p = value+rb->strlen(value)-1;
        while (*p == ' ') *p-- = 0;
        if (*value == 0 || !rb->strcmp(value, "-"))
            continue;
        for (i=0; i<NUM_REMOVE_ITEMS; i++)
        {
            struct remove_setting *setting = &remove_list[i];
            if (!rb->strcmp(name, setting->name))
            {
                set_file_name(setting->value, value, setting);
                if(setting->option == REMOVE_IF_NOT_USED)
                    needs_to_check_whether_used = true;
                break;
            }
        }
    }
    rb->close(fd);

    if(needs_to_check_whether_used)
        check_whether_used();

    /* now remove file assosiated to the theme. */
    for (i=0; i<NUM_REMOVE_ITEMS; i++)
    {
        if (remove_list[i].value[0])
        {
            int ret = remove_file(&remove_list[i]);
            if (ret < 0)
                return ret;
            num_removed += ret;
        }
    }

    /* remove the setting file iff it is in theme directory to protect
     * aginst accidental removal of non theme cfg file. if the file is
     * not in the theme directory, the file may not be a theme cfg file. */
    if (rb->strncasecmp(themefile, THEME_DIR "/", sizeof(THEME_DIR "/")-1))
    {
        show_mess("Skipped", themefile);
    }
    else if (rb->remove(themefile))
    {
        show_mess("Failed", themefile);
        return -1;
    }
    else
    {
        show_mess("Removed", themefile);
        rb->reload_directory();
        num_removed++;
    }
    return num_removed;
}

static bool option_changed = false;
static bool option_menu(void)
{
    MENUITEM_STRINGLIST(option_menu, "Remove Options", NULL,
                        /* same order as remove_list */
#ifdef HAVE_LCD_BITMAP
                        "Font",
#endif
                        "WPS",
#ifdef HAVE_LCD_BITMAP
                        "Statusbar Skin",
#endif
#ifdef HAVE_REMOTE_LCD
                        "Remote WPS",
                        "Remote Statusbar Skin",
#endif
#if LCD_DEPTH > 1
                        "Backdrop",
#endif
#ifdef HAVE_LCD_BITMAP
                        "Iconset", "Viewers Iconset",
#endif
#ifdef HAVE_REMOTE_LCD
                        "Remote Iconset", "Remote Viewers Iconset",
#endif
#ifdef HAVE_LCD_COLOR
                        "Filetype Colours",
#endif
                        "Create Log File");
    struct opt_items remove_names[] = {
        {"Always Remove", -1}, {"Never Remove", -1},
        {"Remove if not Used", -1}, {"Ask for Removal", -1},
    };
    int selected = 0, result;

    while (1)
    {
        result = rb->do_menu(&option_menu, &selected, NULL, false);
        if (result >= 0 && result < NUM_REMOVE_ITEMS)
        {
            struct remove_setting *setting = &remove_list[result];
            int prev_option = setting->option;
            if (rb->set_option(option_menu_[result], &setting->option, INT,
                               remove_names, NUM_REMOVE_OPTION, NULL))
                return true;
            if (prev_option != setting->option)
                option_changed = true;
        }
        else if (result == NUM_REMOVE_ITEMS)
        {
            bool prev_value = create_log;
            if(rb->set_bool("Create Log File", &create_log))
                return true;
            if (prev_value != create_log)
                option_changed = true;
        }
        else if (result == MENU_ATTACHED_USB)
            return true;
        else
            return false;
    }

    return false;
}

enum plugin_status plugin_start(const void* parameter)
{
    static char title[64];
    char *p;
    MENUITEM_STRINGLIST(menu, title, NULL,
                        "Remove Theme", "Remove Options",
                        "Quit");
    int selected = 0, ret;
    bool exit = false;

    if (!parameter)
        return PLUGIN_ERROR;

    rb->snprintf(title, sizeof(title), "Remove %s",
                 rb->strrchr(parameter, '/')+1);
    if((p = rb->strrchr(title, '.')))
        *p = 0;

#ifdef HAVE_LCD_BITMAP
    rb->snprintf(font_file, MAX_PATH, FONT_DIR "/%s.fnt",
                    rb->global_settings->font_file);
#endif
    rb->strlcpy(themefile, parameter, MAX_PATH);
    if (!rb->file_exists(themefile))
    {
        rb->splash(HZ, "File open error!");
        return PLUGIN_ERROR;
    }
    configfile_load(CONFIG_FILENAME, config, nb_config, 0);
    while (!exit)
    {
        switch (rb->do_menu(&menu, &selected, NULL, false))
        {
            case 0:
                if(create_log)
                {
                    log_fd = rb->open(LOG_FILENAME, O_WRONLY|O_CREAT|O_APPEND);
                    if(log_fd >= 0)
                        rb->fdprintf(log_fd, "---- %s ----\n", title);
                    else
                        show_mess("Couldn't open log file.", NULL);
                }
                ret = remove_theme();
                p = (ret >= 0? "Successfully removed!": "Remove failure");
                show_mess(p, NULL);
                if(log_fd >= 0)
                {
                    rb->fdprintf(log_fd, "----------------\n");
                    rb->close(log_fd);
                    log_fd = -1;
                }
                rb->lcd_clear_display();
                rb->lcd_update();
                rb->splashf(0, "%s %s", p, "Press any key to exit.");
                rb->button_clear_queue();
                rb->button_get(true);
                exit = true;
                break;
            case 1:
                if (option_menu())
                    return PLUGIN_USB_CONNECTED;
                break;
            case 2:
                exit = true;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
                break;
            default:
                break;
        }
    }
    if(option_changed)
        configfile_save(CONFIG_FILENAME, config, nb_config, 0);
    return PLUGIN_OK;
}
