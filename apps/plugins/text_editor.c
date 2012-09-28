/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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
#include "lib/playback_control.h"

#define MAX_LINE_LEN 2048


static unsigned char *buffer;
static size_t buffer_size;
static size_t char_count = 0;
static int line_count = 0;
static int last_action_line = 0;
static int last_char_index = 0;
static bool audio_buf = false;

static char temp_line[MAX_LINE_LEN];
static char copy_buffer[MAX_LINE_LEN];
static char filename[MAX_PATH];
static char eol[3];
static bool newfile;

#define ACTION_INSERT 0
#define ACTION_GET    1
#define ACTION_REMOVE 2
#define ACTION_UPDATE 3
#define ACTION_CONCAT 4

static char* _do_action(int action, char* str, int line);
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
#define do_action _do_action
#else
static char* do_action(int action, char* str, int line)
{
    char *r;
    cpu_boost(1);
    r = _do_action(action,str,line);
    cpu_boost(0);
    return r;
}
#endif

static char* _do_action(int action, char* str, int line)
{
    int len, lennew;
    int i=0,c=0;
    if (line>=last_action_line)
    {
        i = last_action_line;
        c = last_char_index;
    }
    while (i<line && i<line_count)
    {
        c += strlen(&buffer[c])+1;
        i++;
    }
    switch (action)
    {
        case ACTION_INSERT:
            len = strlen(str)+1;
            if ( char_count+ len > buffer_size )
                return NULL;
            memmove(&buffer[c+len],&buffer[c],char_count-c);
            strcpy(&buffer[c],str);
            char_count += len;
            line_count++;
            break;
        case ACTION_GET:
            if (line > line_count)
                return &buffer[0];
            break;
        case ACTION_REMOVE:
            if (line > line_count)
                return NULL;
            len = strlen(&buffer[c])+1;
            memmove(&buffer[c],&buffer[c+len],char_count-c-len);
            char_count -= len;
            line_count--;
            break;
        case ACTION_UPDATE:
            if (line > line_count)
                return NULL;
            len = strlen(&buffer[c])+1;
            lennew = strlen(str)+1;
            if ( char_count+ lennew-len > buffer_size )
                return NULL;
            memmove(&buffer[c+lennew],&buffer[c+len],char_count-c-len);
            strcpy(&buffer[c],str);
            char_count += lennew-len;
            break;
        case ACTION_CONCAT:
            if (line > line_count)
                return NULL;
            memmove(&buffer[c-1],&buffer[c],char_count-c);
            char_count--;
            line_count--;
            break;
        default:
            return NULL;
    }
    last_action_line = i;
    last_char_index = c;
    return &buffer[c];
}
static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    (void)data;
    char *b = do_action(ACTION_GET, 0, selected_item);
    /* strlcpy(dst, src, siz) returns strlen(src) */
    if (strlcpy(buf, b, buf_len) >= buf_len)
    {
        strcpy(&buf[buf_len-10], " ...");
    }
    return buf;
}

static void get_eol_string(char* fn)
{
    int fd;
    char t;

    /* assume LF first */
    strcpy(eol,"\n");

    if (!fn || !fn[0])
        return;
    fd = open(fn,O_RDONLY);
    if (fd<0)
        return;

    while (1)
    {
        if (!read(fd,&t,1) || t == '\n')
        {
            break;
        }
        if (t == '\r')
        {
            if (read(fd,&t,1) && t=='\n')
                strcpy(eol,"\r\n");
            else
                strcpy(eol,"\r");
            break;
        }
    }
    close(fd);
    return;
}

static bool save_changes(int overwrite)
{
    int fd;
    int i;

    if (newfile || !overwrite)
    {
        if(kbd_input(filename,MAX_PATH) < 0)
        {
            newfile = true;
            return false;
        }
    }

    fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0)
    {
        newfile = true;
        splash(HZ*2, "Changes NOT saved");
        return false;
    }

    lcd_clear_display();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(1);
#endif
    for (i=0;i<line_count;i++)
    {
        fdprintf(fd,"%s%s", do_action(ACTION_GET, 0, i), eol);
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(0);
#endif
    close(fd);

    if (newfile || !overwrite)
        /* current directory may have changed */
        reload_directory();

    newfile = false;
    return true;
}

static void setup_lists(struct gui_synclist *lists, int sel)
{
    gui_synclist_init(lists,list_get_name_cb,0, false, 1, NULL);
    gui_synclist_set_icon_callback(lists,NULL);
    gui_synclist_set_nb_items(lists,line_count);
    gui_synclist_limit_scroll(lists,true);
    gui_synclist_select_item(lists, sel);
    gui_synclist_draw(lists);
}

enum {
    MENU_RET_SAVE = -1,
    MENU_RET_NO_UPDATE,
    MENU_RET_UPDATE,
};
static int do_item_menu(int cur_sel)
{
    int ret = MENU_RET_NO_UPDATE;
    MENUITEM_STRINGLIST(menu, "Line Options", NULL,
                        "Cut/Delete", "Copy",
                        "Insert Above", "Insert Below",
                        "Concat To Above",
                        "Save", "Playback Control");

    switch (do_menu(&menu, NULL, NULL, false))
    {
        case 0: /* cut */
            strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                        MAX_LINE_LEN);
            do_action(ACTION_REMOVE, 0, cur_sel);
            ret = MENU_RET_UPDATE;
        break;
        case 1: /* copy */
            strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                        MAX_LINE_LEN);
            ret = MENU_RET_NO_UPDATE;
        break;
        case 2: /* insert above */
            if (!kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                do_action(ACTION_INSERT,copy_buffer,cur_sel);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 3: /* insert below */
            if (!kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                do_action(ACTION_INSERT,copy_buffer,cur_sel+1);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 4: /* cat to above */
            if (cur_sel>0)
            {
                do_action(ACTION_CONCAT,0,cur_sel);
                ret = MENU_RET_UPDATE;
            }
        break;
        case 5: /* save */
            ret = MENU_RET_SAVE;
        break;
        case 6: /* playback menu */
            if (!audio_buf)
                playback_control(NULL);
            else
                splash(HZ, "Cannot restart playback");
            ret = MENU_RET_NO_UPDATE;
        break;
        default:
            ret = MENU_RET_NO_UPDATE;
        break;
    }
    return ret;
}

#ifdef HAVE_LCD_COLOR

#define hex2dec(c) (((c) >= '0' && ((c) <= '9')) ? (toupper(c)) - '0' : \
                                                   (toupper(c)) - 'A' + 10)

static int my_hex_to_rgb(const char* hex, int* color)
{   int ok = 1;
    int i;
    int red, green, blue;

    if (strlen(hex) == 6) {
        for (i=0; i < 6; i++ ) {
            if (!isxdigit(hex[i])) {
                ok=0;
                break;
            }
        }

        if (ok) {
            red = (hex2dec(hex[0]) << 4) | hex2dec(hex[1]);
            green = (hex2dec(hex[2]) << 4) | hex2dec(hex[3]);
            blue = (hex2dec(hex[4]) << 4) | hex2dec(hex[5]);
            *color = LCD_RGBPACK(red,green,blue);
            return 0;
        }
    }

    return -1;
}
#endif /* HAVE_LCD_COLOR */

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    int fd;

    struct gui_synclist lists;
    bool exit = false;
    int button;
    bool changed = false;
    int cur_sel=0;
#ifdef HAVE_LCD_COLOR
    bool edit_colors_file = false;
#endif

    copy_buffer[0]='\0';

#if LCD_DEPTH > 1
    lcd_set_backdrop(NULL);
#endif
    buffer = plugin_get_buffer(&buffer_size);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(1);
#endif
    if (parameter)
    {
#ifdef HAVE_LCD_COLOR
        char *c = NULL;
#endif
        strlcpy(filename, (char*)parameter, MAX_PATH);
        get_eol_string(filename);
        fd = open(filename,O_RDONLY);
        if (fd<0)
        {
            splashf(HZ*2, "Couldnt open file: %s", filename);
            return PLUGIN_ERROR;
        }
#ifdef HAVE_LCD_COLOR
        c = strrchr(filename, '.');
        if (c && !strcmp(c, ".colours"))
            edit_colors_file = true;
#endif
        if (buffer_size <= (size_t)filesize(fd) + 0x400)
        {
            buffer = plugin_get_audio_buffer(&buffer_size);
            audio_buf = true;
        }
        /* read in the file */
        while (read_line(fd,temp_line,MAX_LINE_LEN) > 0)
        {
            if (!do_action(ACTION_INSERT,temp_line,line_count))
            {
                splashf(HZ*2,"Error reading file: %s",(char*)parameter);
                close(fd);
                return PLUGIN_ERROR;
            }
        }
        close(fd);
        newfile = false;
    }
    else
    {
        strcpy(filename,"/");
        strcpy(eol,"\n");
        newfile = true;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(0);
#endif
    /* now dump it in the list */
    setup_lists(&lists,0);
    lcd_update();
    while (!exit)
    {
        gui_synclist_draw(&lists);
        cur_sel = gui_synclist_get_sel_pos(&lists);
        button = get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (gui_synclist_do_button(&lists,&button,LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_OK:
            {
                if (line_count)
                    strlcpy(temp_line, do_action(ACTION_GET, 0, cur_sel),
                                MAX_LINE_LEN);
#ifdef HAVE_LCD_COLOR
                if (edit_colors_file && line_count)
                {
                    char *name = temp_line, *value = NULL;
                    char extension[16];
                    int color, old_color;
                    bool temp_changed = false;

                    MENUITEM_STRINGLIST(menu, "Edit What?", NULL,
                                        "Extension", "Colour");

                    settings_parseline(temp_line, &name, &value);
                    strlcpy(extension, name, sizeof(extension));
                    if (value)
                        my_hex_to_rgb(value, &color);
                    else
                        color = 0;

                    switch (do_menu(&menu, NULL, NULL, false))
                    {
                        case 0:
                            temp_changed = !kbd_input(extension, sizeof(extension));
                            break;
                        case 1:
                            old_color = color;
                            set_color(&screens[SCREEN_MAIN], name, &color, -1);
                            temp_changed = (value == NULL) || (color != old_color);
                            break;
                    }

                    if (temp_changed)
                    {
                        snprintf(temp_line, MAX_LINE_LEN, "%s: %02X%02X%02X",
                                     extension, RGB_UNPACK_RED(color),
                                     RGB_UNPACK_GREEN(color),
                                     RGB_UNPACK_BLUE(color));
                        do_action(ACTION_UPDATE, temp_line, cur_sel);
                        changed = true;
                    }
                }
                else
#endif
                if (!kbd_input(temp_line,MAX_LINE_LEN))
                {
                    if (line_count)
                        do_action(ACTION_UPDATE,temp_line,cur_sel);
                    else
                        do_action(ACTION_INSERT,temp_line,cur_sel);
                    changed = true;
                }
            }
            break;
            case ACTION_STD_CONTEXT:
                if (!line_count) break;
                strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                            MAX_LINE_LEN);
                do_action(ACTION_REMOVE, 0, cur_sel);
                changed = true;
            break;
            case ACTION_STD_MENU:
            {
                /* do the item menu */
                switch (do_item_menu(cur_sel))
                {
                    case MENU_RET_SAVE:
                        if(save_changes(1))
                            changed = false;
                    break;
                    case MENU_RET_UPDATE:
                        changed = true;
                    break;
                    case MENU_RET_NO_UPDATE:
                    break;
                }
            }
            break;
            case ACTION_STD_CANCEL:
                if (changed)
                {
                    MENUITEM_STRINGLIST(menu, "Do What?", NULL,
                                        "Return",
                                        "Playback Control", "Save Changes",
                                        "Save As...", "Save and Exit",
                                        "Ignore Changes and Exit");
                    switch (do_menu(&menu, NULL, NULL, false))
                    {
                        case 0:
                        break;
                        case 1:
                            if (!audio_buf)
                                playback_control(NULL);
                            else
                                splash(HZ, "Cannot restart playback");
                        break;
                        case 2: //save to disk
                            if(save_changes(1))
                                changed = 0;
                        break;
                        case 3:
                            if(save_changes(0))
                                changed = 0;
                        break;
                        case 4:
                            if(save_changes(1))
                                exit=1;
                        break;
                        case 5:
                            exit=1;
                        break;
                    }
                }
                else exit=1;
            break;
        }
        gui_synclist_set_nb_items(&lists,line_count);
        if(line_count > 0 && line_count <= cur_sel)
            gui_synclist_select_item(&lists,line_count-1);
    }
    return PLUGIN_OK;
}
