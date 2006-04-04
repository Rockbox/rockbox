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
/* button definitions, every keypad must only have select,menu and cancel */
#if CONFIG_KEYPAD == RECORDER_PAD
#define TEXT_EDITOR_SELECT BUTTON_PLAY
#define TEXT_EDITOR_CANCEL BUTTON_OFF
#define TEXT_EDITOR_ITEM_MENU BUTTON_F1

#elif CONFIG_KEYPAD == ONDIO_PAD
#define TEXT_EDITOR_SELECT_PRE BUTTON_MENU
#define TEXT_EDITOR_SELECT (BUTTON_MENU|BUTTON_REL)
#define TEXT_EDITOR_CANCEL BUTTON_OFF
#define TEXT_EDITOR_ITEM_MENU BUTTON_MENU|BUTTON_REPEAT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define TEXT_EDITOR_SELECT BUTTON_SELECT
#define TEXT_EDITOR_CANCEL BUTTON_OFF
#define TEXT_EDITOR_DELETE BUTTON_REC
#define TEXT_EDITOR_ITEM_MENU BUTTON_MODE

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define TEXT_EDITOR_SELECT_PRE    BUTTON_SELECT
#define TEXT_EDITOR_SELECT ( BUTTON_SELECT | BUTTON_REL)
#define TEXT_EDITOR_CANCEL_PRE    BUTTON_SELECT
#define TEXT_EDITOR_CANCEL (BUTTON_SELECT | BUTTON_MENU)
#define TEXT_EDITOR_DELETE (BUTTON_LEFT)
#define TEXT_EDITOR_ITEM_MENU (BUTTON_MENU)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define TEXT_EDITOR_SELECT BUTTON_SELECT
#define TEXT_EDITOR_CANCEL BUTTON_POWER
#define TEXT_EDITOR_ITEM_MENU BUTTON_PLAY

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#else
    #error TEXT_EDITOR: Unsupported keypad
#endif

#define MAX_LINE_LEN 128

#if PLUGIN_BUFFER_SIZE > 0x45000
#define MAX_LINES    2048
#else
#define MAX_LINES   128
#endif

PLUGIN_HEADER
static struct plugin_api* rb;

struct LineStruct {
    char line[MAX_LINE_LEN];
    int prev; /* index to prev item, or -1 */
    int next; /* index to next item, or -1 */
};

struct LineStruct lines[MAX_LINES];
int line_count = 0;
int first = -1, last = -1;
int indicies[MAX_LINES];
/**************************** stuff for the linked lists  ***************/
int build_indicies(void)
{
    int i=0, index = first;
    struct LineStruct *line;
    if (first==-1)
        return 0;
    while (i<line_count)
    {
        indicies[i++] = index;
        DEBUGF("%d,",index);
        line = &lines[index];
        index = line->next;

    }
    DEBUGF("\n");
    return 1;
}

int find_first_free(int start)
{
    int i;
    if ((start <0) || (start >=MAX_LINES))
        start = 0;
    i = start;
    do
    {
        if (lines[i].line[0] == '\0')
            return i;
        i = (i+1)%MAX_LINES;
    } while (i!=start);
    return -1;
}

int add_line(char *line, int idx_after_me)
{
    struct LineStruct *temp;
    int next;
    int this_idx = find_first_free(idx_after_me);
    if ((line_count >= MAX_LINES) || (this_idx == -1))
        return -1;
    DEBUGF("line:%s ,idx_after_me=%d\n",line,idx_after_me);
    if (idx_after_me == -1) /* add as the first item */
    {
        rb->strcpy(lines[this_idx].line,line);
        lines[this_idx].prev = -1;
        if (first != -1)
            lines[first].prev = this_idx;
        lines[this_idx].next = first;
        first = this_idx;
        if (last == idx_after_me)
            last = this_idx;
        line_count++;
        return 1;
    }

    temp = &lines[idx_after_me];
    next = lines[idx_after_me].next;
    temp->next = this_idx;
    rb->strcpy(lines[this_idx].line,line);
    temp = &lines[this_idx];
    temp->next = next;
    temp->prev = idx_after_me;
    if (last == idx_after_me)
        last = this_idx;
    if (first == -1)
        first = this_idx;
    line_count ++;
    return this_idx;
}

void del_line(int line)
{
    int idx_prev, idx_next;
    idx_prev = (&lines[line])->prev;
    idx_next = (&lines[line])->next;
    lines[line].line[0] = '\0';
    lines[idx_prev].next = idx_next;
    lines[idx_next].prev = idx_prev;
    line_count --;
}
char *list_get_name_cb(int selected_item,void* data,char* buf)
{
    (void)data;
    rb->strcpy(buf,lines[indicies[selected_item]].line);
    return buf;
}
char filename[1024];
void save_changes(int overwrite)
{
    int fd;
    int i;

    if (!filename[0] || !overwrite)
    {
        rb->strcpy(filename,"/");
        rb->kbd_input(filename,1024);
    }

    fd = rb->open(filename,O_WRONLY|O_CREAT);
    if (!fd)
    {
        rb->splash(HZ*2,1,"Changes NOT saved");
        return;
    }

    rb->lcd_clear_display();
    build_indicies();
    for (i=0;i<line_count;i++)
    {
        rb->fdprintf(fd,"%s\n",lines[indicies[i]].line);
    }

    rb->close(fd);
}

void setup_lists(struct gui_synclist *lists)
{
    build_indicies();
    rb->gui_synclist_init(lists,list_get_name_cb,0);
    rb->gui_synclist_set_icon_callback(lists,NULL);
    rb->gui_synclist_set_nb_items(lists,line_count);
    rb->gui_synclist_limit_scroll(lists,true);
    rb->gui_synclist_select_item(lists, 0);
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
            /*       { "Split Line",NULL }, */
            { "", NULL },
            { "Save", NULL },
    };
    m = rb->menu_init(items, sizeof(items) / sizeof(*items),
    NULL, NULL, NULL, NULL);

    switch (rb->menu_show(m))
    {
        case 0: /* cut */
            rb->strcpy(copy_buffer,lines[indicies[cur_sel]].line);
            del_line(indicies[cur_sel]);
            ret = MENU_RET_UPDATE;
        break;
        case 1: /* copy */
            rb->strcpy(copy_buffer,lines[indicies[cur_sel]].line);
            ret = MENU_RET_NO_UPDATE;
        break;
        case 2: /* blank */
            ret = MENU_RET_NO_UPDATE;
        break;

        case 3: /* insert above */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                add_line(copy_buffer,lines[indicies[cur_sel]].prev);
                copy_buffer[0]='\0';
                ret = MENU_RET_UPDATE;
            }
        break;
        case 4: /* insert below */
            if (!rb->kbd_input(copy_buffer,MAX_LINE_LEN))
            {
                add_line(copy_buffer,indicies[cur_sel]);
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
                rb->strcat(lines[indicies[cur_sel-1]].line,lines[indicies[cur_sel]].line);
                del_line(indicies[cur_sel]);
                ret = MENU_RET_UPDATE;
            }
        break;
        /*   case 7: // split line */

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
    char temp_line[MAX_LINE_LEN];

    struct gui_synclist lists;
    bool exit = false;
    int button, last_button = BUTTON_NONE;
    bool changed = false;
    int cur_sel;
    char copy_buffer[MAX_LINE_LEN]; copy_buffer[0]='\0';

    rb = api;
    if (parameter)
    {
        rb->strcpy(filename,(char*)parameter);
        fd = rb->open(filename,O_RDONLY);
        if (fd<0)
        {
            rb->splash(HZ*2,true,"Couldnt open file: %s",(char*)parameter);
            return PLUGIN_ERROR;
        }
        /* read in the file */
        while (rb->read_line(fd,temp_line,MAX_LINE_LEN))
        {
            if (add_line(temp_line,last) < 0)
            {
                rb->splash(HZ*2,true,"Error reading file: %s",(char*)parameter);
                rb->close(fd);
                return PLUGIN_ERROR;
            }
        }
        rb->close(fd);
    }
    else filename[0] = '\0';
    /* now dump it in the list */
    setup_lists(&lists);
    while (!exit)
    {
        rb->gui_synclist_draw(&lists);
        cur_sel = rb->gui_synclist_get_sel_pos(&lists);
        button = rb->button_get(true);
        if (rb->gui_synclist_do_button(&lists,button))
            continue;
        switch (button)
        {
            case TEXT_EDITOR_SELECT:
            {
#ifdef TEXT_EDITOR_SELECT_PRE
                if (last_button != TEXT_EDITOR_SELECT_PRE)
                    break;
#endif
                char buf[MAX_LINE_LEN];buf[0]='\0';

                if (line_count)
                    rb->strcpy(buf,lines[indicies[cur_sel]].line);
                if (!rb->kbd_input(buf,MAX_LINE_LEN))
                {
                    if (line_count)
                        rb->strcpy(lines[indicies[cur_sel]].line,buf);
                    else { add_line(buf, first); setup_lists(&lists); }
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
                rb->strcpy(copy_buffer,lines[indicies[cur_sel]].line);
                del_line(indicies[cur_sel]);
                changed = true;
                setup_lists(&lists);
            break;
#endif
#ifdef TEXT_EDITOR_ITEM_MENU
            case TEXT_EDITOR_ITEM_MENU:
#ifdef TEXT_EDITOR_RC_ITEM_MENU
            case TEXT_EDITOR_RC_ITEM_MENU:
#endif
#ifdef TEXT_EDITOR_ITEM_MENU_PRE
                if (lastbutton != TEXT_EDITOR_ITEM_MENU_PRE
#ifdef TEXT_EDITOR_RC_ITEM_MENU_PRE
                    && lastbutton != TEXT_EDITOR_RC_ITEM_MENU_PRE
#endif
                    )
                    break;
#endif
            { /* do the item menu */
                switch (do_item_menu(cur_sel, copy_buffer))
                {
                    case MENU_RET_SAVE:
                        save_changes(1);
                        changed = false;
                    break;
                    case MENU_RET_UPDATE:
                        changed = true;
                        setup_lists(&lists);
                    break;
                    case MENU_RET_NO_UPDATE:
                    break;
                }
            }
            break;
#endif /* TEXT_EDITOR_ITEM_MENU */
            case TEXT_EDITOR_CANCEL:
#ifdef TEXT_EDITOR_CANCEL_PRE
                if (last_button != TEXT_EDITOR_CANCEL_PRE)
                    break;
#endif
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
        last_button = button;
    }

    return PLUGIN_OK;
}
