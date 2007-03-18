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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "action.h"

#if PLUGIN_BUFFER_SIZE > 0x45000
#define MAX_CHARS    0x40000 /* 128 kiB */
#else
#define MAX_CHARS   0x6000 /* 24 kiB */
#endif
#define MAX_LINE_LEN 2048
PLUGIN_HEADER
static struct plugin_api* rb;

static char buffer[MAX_CHARS];
static char eol[3];
static int char_count = 0;
static int line_count = 0;
static int last_action_line = 0;
static int last_char_index = 0;

#define ACTION_INSERT 0
#define ACTION_GET    1
#define ACTION_REMOVE 2
#define ACTION_UPDATE 3
#define ACTION_CONCAT 4

int _do_action(int action, char* str, int line);
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
#define do_action _do_action
#else
int do_action(int action, char* str, int line)
{
    int r;
    rb->cpu_boost(1);
    r = _do_action(action,str,line);
    rb->cpu_boost(0);
    return r;
}
#endif

int _do_action(int action, char* str, int line)
{
    int len;
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
                return 0;
            rb->memmove(&buffer[c+len],&buffer[c],char_count);
            rb->strcpy(&buffer[c],str);
            char_count += len;
            line_count++;
            break;
        case ACTION_GET:
            if (line > line_count)
                return 0;
            last_action_line = i;
            last_char_index = c;
            return c;
            break;
        case ACTION_REMOVE:
            if (line > line_count)
                return 0;
            len = rb->strlen(&buffer[c])+1;
            char_count -= len;
            rb->memmove(&buffer[c],&buffer[c+len],char_count);
            line_count--;
            break;
        case ACTION_UPDATE:
            if (line > line_count)
                return 0;
            len = rb->strlen(&buffer[c])+1;
            rb->memmove(&buffer[c+rb->strlen(str)+1],&buffer[c+len],char_count);
            rb->strcpy(&buffer[c],str);
            char_count += rb->strlen(str)+1-len;
            break;
        case ACTION_CONCAT:
            if (line > line_count)
                return 0;
            rb->memmove(&buffer[c-1],&buffer[c],char_count);
            break;
        default:
            return 0;
    }
    last_action_line = i;
    last_char_index = c;
    return 1;
}
char *list_get_name_cb(int selected_item,void* data,char* buf)
{
    char *b = &buffer[do_action(ACTION_GET,0,selected_item)];
    (void)data;
    if (rb->strlen(b) >= MAX_PATH)
    {
        char t = b[MAX_PATH-10];
        b[MAX_PATH-10] = '\0';
        rb->snprintf(buf,MAX_PATH,"%s ...",b);
        b[MAX_PATH-10] = t;
    }
    else rb->strcpy(buf,b);
    return buf;
}
char filename[MAX_PATH];
int get_eol_string(char* fn)
{
    int fd=-1;
    char t;
    if (!fn)
        return 0;
    else if  (!fn[0])
        return 0;
    fd = rb->PREFIX(open(fn,O_RDONLY));
    if (fd<0)
        return 0;
    eol[0] = '\0';
    while (!eol[0])
    {
        if (!rb->read(fd,&t,1))
        {
            rb->strcpy(eol,"\n");
            return 0;
        }
        if (t == '\r')
        {
            if (rb->read(fd,&t,1) && t=='\n')
                rb->strcpy(eol,"\r\n");
            else rb->strcpy(eol,"\r");
        }
        else if (t == '\n')
        {
            rb->strcpy(eol,"\n");
        }
    }
    rb->close(fd);
    return 1;
}

void save_changes(int overwrite)
{
    int fd;
    int i;

    if (!filename[0] || !overwrite)
    {
        rb->strcpy(filename,"/");
        rb->kbd_input(filename,MAX_PATH);
    }

    fd = rb->open(filename,O_WRONLY|O_CREAT|O_TRUNC);
    if (fd < 0)
    {
        rb->splash(HZ*2, "Changes NOT saved");
        return;
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
        rb->fdprintf(fd,"%s%s",&buffer[do_action(ACTION_GET,0,i)],eol);
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(0);
#endif
    rb->close(fd);
}

void setup_lists(struct gui_synclist *lists, int sel)
{
    rb->gui_synclist_init(lists,list_get_name_cb,0, false, 1);
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
    int m, ret = 0;
    static const struct menu_item items[] = {
            { "Cut", NULL },
            { "Copy", NULL },
            { "", NULL },
            { "Insert Above", NULL },
            { "Insert Below", NULL },
            { "", NULL },
            { "Cat To Above",NULL },
            { "", NULL },
            { "Save", NULL },
    };
    m = rb->menu_init(items, sizeof(items) / sizeof(*items),
    NULL, NULL, NULL, NULL);

    switch (rb->menu_show(m))
    {
        case 0: /* cut */
            rb->strcpy(copy_buffer,&buffer[do_action(ACTION_GET,0,cur_sel)]);
            do_action(ACTION_REMOVE,0,cur_sel);
            ret = MENU_RET_UPDATE;
        break;
        case 1: /* copy */
            rb->strcpy(copy_buffer,&buffer[do_action(ACTION_GET,0,cur_sel)]);
            ret = MENU_RET_NO_UPDATE;
        break;
        case 2: /* blank */
            ret = MENU_RET_NO_UPDATE;
        break;

        case 3: /* insert above */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                do_action(ACTION_INSERT,copy_buffer,cur_sel);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 4: /* insert below */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                do_action(ACTION_INSERT,copy_buffer,cur_sel+1);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 5: /* blank */
            ret = MENU_RET_NO_UPDATE;
        break;
        case 6: /* cat to above */
            if (cur_sel>0)
            {
                do_action(ACTION_CONCAT,0,cur_sel);
                ret = MENU_RET_UPDATE;
            }
        break;
        case 7: /* save */
            ret = MENU_RET_SAVE;
        break;
        default:
            ret = MENU_RET_NO_UPDATE;
        break;
    }
    rb->menu_exit(m);
    return ret;
}
/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int fd;
    static char temp_line[MAX_LINE_LEN];

    struct gui_synclist lists;
    bool exit = false;
    int button;
    bool changed = false;
    int cur_sel=0;
    static char copy_buffer[MAX_LINE_LEN];
    bool prev_show_statusbar;

    rb = api;

    copy_buffer[0]='\0';
    prev_show_statusbar = rb->global_settings->statusbar;
    rb->global_settings->statusbar = false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(1);
#endif
    if (parameter)
    {
        rb->strcpy(filename,(char*)parameter);
        if (!get_eol_string(filename))
        {
            rb->strcpy(eol,"\n");
        }
        fd = rb->open(filename,O_RDONLY);
        if (fd<0)
        {
            rb->splash(HZ*2,"Couldnt open file: %s",(char*)parameter);
            return PLUGIN_ERROR;
        }
        /* read in the file */
        while (rb->read_line(fd,temp_line,MAX_LINE_LEN))
        {
            if (!do_action(ACTION_INSERT,temp_line,line_count))
            {
                rb->splash(HZ*2,"Error reading file: %s",(char*)parameter);
                rb->close(fd);
                return PLUGIN_ERROR;
            }
        }
        rb->close(fd);
    }
    else
    {
        filename[0] = '\0';
        rb->strcpy(eol,"\n");
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
        if (rb->gui_synclist_do_button(&lists,button,LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_OK:
            {
                if (line_count)
                    rb->strcpy(temp_line,&buffer[do_action(ACTION_GET,0,cur_sel)]);
                if (!rb->kbd_input(temp_line,MAX_LINE_LEN))
                {
                    if (line_count)
                    {
                        do_action(ACTION_UPDATE,temp_line,cur_sel);
                    }
                    else do_action(ACTION_INSERT,temp_line,cur_sel);
                    changed = true;
                }
            }
            break;
#ifdef TEXT_EDITOR_DELETE
            case TEXT_EDITOR_DELETE:
#ifdef TEXT_EDITOR_DELETE_PRE
                if (last_button != TEXT_EDITOR_DELETE_PRE)
                    break;
#endif
                if (!line_count) break;
                rb->strcpy(copy_buffer,&buffer[do_action(ACTION_GET,0,cur_sel)]);
                do_action(ACTION_REMOVE,0,cur_sel);
                changed = true;
            break;
#endif
            case ACTION_STD_MENU:
            { /* do the item menu */
                switch (do_item_menu(cur_sel, copy_buffer))
                {
                    case MENU_RET_SAVE:
                        save_changes(1);
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
                    int m;
                    int result;

                    static const struct menu_item items[] = {
                            { "Return", NULL },
                            { " ", NULL },
                            { "Save Changes", NULL },
                            { "Save As...", NULL },
                            { " ", NULL },
                            { "Save and Exit", NULL },
                            { "Ignore Changes and Exit", NULL },
                    };

                    m = rb->menu_init(items, sizeof(items) / sizeof(*items),
                    NULL, NULL, NULL, NULL);

                    result=rb->menu_show(m);

                    switch (result)
                    {
                        case 0:
                        break;
                        case 2: //save to disk
                            save_changes(1);
                            changed = 0;
                        break;
                        case 3:
                            save_changes(0);
                            changed = 0;
                        break;

                        case 5:
                            save_changes(1);
                            exit=1;
                        break;
                        case 6:
                            exit=1;
                        break;
                    }
                    rb->menu_exit(m);
                }
                else exit=1;
            break;
        }
        rb->gui_synclist_set_nb_items(&lists,line_count);
    }
    rb->global_settings->statusbar = prev_show_statusbar;
    return PLUGIN_OK;
}
