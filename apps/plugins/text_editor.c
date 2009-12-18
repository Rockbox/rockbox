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

#if PLUGIN_BUFFER_SIZE > 0x45000
#define MAX_CHARS   0x40000 /* 256 kiB */
#else
#define MAX_CHARS   0x6000  /* 24 kiB */
#endif
#define MAX_LINE_LEN 2048
PLUGIN_HEADER

static char buffer[MAX_CHARS];
static int char_count = 0;
static int line_count = 0;
static int last_action_line = 0;
static int last_char_index = 0;

#define ACTION_INSERT 0
#define ACTION_GET    1
#define ACTION_REMOVE 2
#define ACTION_UPDATE 3
#define ACTION_CONCAT 4

char* _do_action(int action, char* str, int line);
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
#define do_action _do_action
#else
char* do_action(int action, char* str, int line)
{
    char *r;
    rb->cpu_boost(1);
    r = _do_action(action,str,line);
    rb->cpu_boost(0);
    return r;
}
#endif

char* _do_action(int action, char* str, int line)
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
        c += rb->strlen(&buffer[c])+1;
        i++;
    }
    switch (action)
    {
        case ACTION_INSERT:
            len = rb->strlen(str)+1;
            if ( char_count+ len > MAX_CHARS )
                return NULL;
            rb->memmove(&buffer[c+len],&buffer[c],char_count-c);
            rb->strcpy(&buffer[c],str);
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
            len = rb->strlen(&buffer[c])+1;
            rb->memmove(&buffer[c],&buffer[c+len],char_count-c-len);
            char_count -= len;
            line_count--;
            break;
        case ACTION_UPDATE:
            if (line > line_count)
                return NULL;
            len = rb->strlen(&buffer[c])+1;
            lennew = rb->strlen(str)+1;
            if ( char_count+ lennew-len > MAX_CHARS )
                return NULL;
            rb->memmove(&buffer[c+lennew],&buffer[c+len],char_count-c-len);
            rb->strcpy(&buffer[c],str);
            char_count += lennew-len;
            break;
        case ACTION_CONCAT:
            if (line > line_count)
                return NULL;
            rb->memmove(&buffer[c-1],&buffer[c],char_count-c);
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
    if (rb->strlcpy(buf, b, buf_len) >= buf_len)
    {
        rb->strcpy(&buf[buf_len-10], " ...");
    }
    return buf;
}

char filename[MAX_PATH];
char eol[3];
bool newfile;
void get_eol_string(char* fn)
{
    int fd;
    char t;

    /* assume LF first */
    rb->strcpy(eol,"\n");

    if (!fn || !fn[0])
        return;
    fd = rb->open(fn,O_RDONLY);
    if (fd<0)
        return;

    while (1)
    {
        if (!rb->read(fd,&t,1) || t == '\n')
        {
            break;
        }
        if (t == '\r')
        {
            if (rb->read(fd,&t,1) && t=='\n')
                rb->strcpy(eol,"\r\n");
            else
                rb->strcpy(eol,"\r");
            break;
        }
    }
    rb->close(fd);
    return;
}

bool save_changes(int overwrite)
{
    int fd;
    int i;

    if (newfile || !overwrite)
    {
        if(rb->kbd_input(filename,MAX_PATH) < 0)
        {
            newfile = true;
            return false;
        }
    }

    fd = rb->open(filename,O_WRONLY|O_CREAT|O_TRUNC);
    if (fd < 0)
    {
        newfile = true;
        rb->splash(HZ*2, "Changes NOT saved");
        return false;
    }

    if (!overwrite)
        /* current directory may have changed */
        rb->reload_directory();

    rb->lcd_clear_display();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(1);
#endif
    for (i=0;i<line_count;i++)
    {
        rb->fdprintf(fd,"%s%s", do_action(ACTION_GET, 0, i), eol);
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(0);
#endif
    rb->close(fd);
    newfile = false;
    return true;
}

void setup_lists(struct gui_synclist *lists, int sel)
{
    rb->gui_synclist_init(lists,list_get_name_cb,0, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(lists,NULL);
    rb->gui_synclist_set_nb_items(lists,line_count);
    rb->gui_synclist_limit_scroll(lists,true);
    rb->gui_synclist_select_item(lists, sel);
    rb->gui_synclist_draw(lists);
}

enum {
    MENU_RET_SAVE = -1,
    MENU_RET_NO_UPDATE,
    MENU_RET_UPDATE,
};
int do_item_menu(int cur_sel, char* copy_buffer)
{
    int ret = MENU_RET_NO_UPDATE;
    MENUITEM_STRINGLIST(menu, "Line Options", NULL,
                        "Cut/Delete", "Copy",
                        "Insert Above", "Insert Below",
                        "Concat To Above",
                        "Save", "Playback Control");

    switch (rb->do_menu(&menu, NULL, NULL, false))
    {
        case 0: /* cut */
            rb->strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                        MAX_LINE_LEN);
            do_action(ACTION_REMOVE, 0, cur_sel);
            ret = MENU_RET_UPDATE;
        break;
        case 1: /* copy */
            rb->strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                        MAX_LINE_LEN);
            ret = MENU_RET_NO_UPDATE;
        break;
        case 2: /* insert above */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                do_action(ACTION_INSERT,copy_buffer,cur_sel);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 3: /* insert below */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
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
            playback_control(NULL);
            ret = MENU_RET_NO_UPDATE;
        break;
        default:
            ret = MENU_RET_NO_UPDATE;
        break;
    }
    return ret;
}

#ifdef HAVE_LCD_COLOR
/* in misc.h but no need to polute the api */
#define toupper(c) (((c >= 'a') && (c <= 'z'))?c+'A':c)
#define isxdigit(c) ((c>='a' && c<= 'f') || (c>='A' && c<= 'F') \
                    || (c>='0' && c<= '9'))
#define hex2dec(c) (((c) >= '0' && ((c) <= '9')) ? (toupper(c)) - '0' : \
                                                   (toupper(c)) - 'A' + 10)
int hex_to_rgb(const char* hex, int* color)
{   int ok = 1;
    int i;
    int red, green, blue;

    if (rb->strlen(hex) == 6) {
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
    static char temp_line[MAX_LINE_LEN];

    struct gui_synclist lists;
    bool exit = false;
    int button;
    bool changed = false;
    int cur_sel=0;
    static char copy_buffer[MAX_LINE_LEN];
#ifdef HAVE_LCD_COLOR
    bool edit_colors_file = false;
#endif

    copy_buffer[0]='\0';

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(1);
#endif
    if (parameter)
    {
#ifdef HAVE_LCD_COLOR
        char *c = NULL;
#endif
        rb->strlcpy(filename, (char*)parameter, MAX_PATH);
        get_eol_string(filename);
        fd = rb->open(filename,O_RDONLY);
        if (fd<0)
        {
            rb->splashf(HZ*2, "Couldnt open file: %s", filename);
            return PLUGIN_ERROR;
        }
#ifdef HAVE_LCD_COLOR
        c = rb->strrchr(filename, '.');
        if (c && !rb->strcmp(c, ".colours"))
            edit_colors_file = true;
#endif
        /* read in the file */
        while (rb->read_line(fd,temp_line,MAX_LINE_LEN) > 0)
        {
            if (!do_action(ACTION_INSERT,temp_line,line_count))
            {
                rb->splashf(HZ*2,"Error reading file: %s",(char*)parameter);
                rb->close(fd);
                return PLUGIN_ERROR;
            }
        }
        rb->close(fd);
        newfile = false;
    }
    else
    {
        rb->strcpy(filename,"/");
        rb->strcpy(eol,"\n");
        newfile = true;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(0);
#endif
    /* now dump it in the list */
    setup_lists(&lists,0);
    rb->lcd_update();
    while (!exit)
    {
        rb->gui_synclist_draw(&lists);
        cur_sel = rb->gui_synclist_get_sel_pos(&lists);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&lists,&button,LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_OK:
            {
                if (line_count)
                    rb->strlcpy(temp_line, do_action(ACTION_GET, 0, cur_sel),
                                MAX_LINE_LEN);
#ifdef HAVE_LCD_COLOR
                if (edit_colors_file && line_count)
                {
                    char *name = temp_line, *value = NULL;
                    char extension[MAX_LINE_LEN];
                    int color, old_color;
                    bool temp_changed = false;
                    rb->settings_parseline(temp_line, &name, &value);
                    if (line_count)
                    {
                        MENUITEM_STRINGLIST(menu, "Edit What?", NULL,
                                            "Extension", "Colour");
                        rb->strcpy(extension, name);
                        if (value)
                            hex_to_rgb(value, &color);
                        else
                            color = 0;

                        switch (rb->do_menu(&menu, NULL, NULL, false))
                        {
                            case 0:
                                temp_changed = !rb->kbd_input(extension,MAX_LINE_LEN);
                                break;
                            case 1:
                                old_color = color;
                                rb->set_color(rb->screens[SCREEN_MAIN], name, &color, -1);
                                temp_changed = (value == NULL) || (color != old_color);
                                break;
                        }

                        if (temp_changed)
                        {
                            rb->snprintf(temp_line, MAX_LINE_LEN, "%s: %02X%02X%02X",
                                         extension, RGB_UNPACK_RED(color),
                                         RGB_UNPACK_GREEN(color),
                                         RGB_UNPACK_BLUE(color));
                            do_action(ACTION_UPDATE, temp_line, cur_sel);
                            changed = true;
                        }
                    }
                }
                else
#endif
                if (!rb->kbd_input(temp_line,MAX_LINE_LEN))
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
                rb->strlcpy(copy_buffer, do_action(ACTION_GET, 0, cur_sel),
                            MAX_LINE_LEN);
                do_action(ACTION_REMOVE, 0, cur_sel);
                changed = true;
            break;
            case ACTION_STD_MENU:
            { /* do the item menu */
                switch (do_item_menu(cur_sel, copy_buffer))
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
                    switch (rb->do_menu(&menu, NULL, NULL, false))
                    {
                        case 0:
                        break;
                        case 1:
                            playback_control(NULL);
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
        rb->gui_synclist_set_nb_items(&lists,line_count);
        if(line_count > 0 && line_count <= cur_sel)
            rb->gui_synclist_select_item(&lists,line_count-1);
    }
    return PLUGIN_OK;
}
