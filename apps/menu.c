/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
2005 Kevin Ferrare :
 - Multi screen support
 - Rewrote/removed a lot of code now useless with the new gui API
*/
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"

#include "hwcompat.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "usb.h"
#include "panic.h"
#include "settings.h"
#include "settings_list.h"
#include "status.h"
#include "screens.h"
#include "talk.h"
#include "lang.h"
#include "misc.h"
#include "action.h"
#include "menus/exported_menus.h"
#include "string.h"
#include "root_menu.h"
#include "bookmark.h"
#include "gwps-common.h" /* for fade() */
#include "audio.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "buttonbar.h"
/* needed for the old menu system */
struct menu {
    struct menu_item* items;
    int count;
    int (*callback)(int, int);
    int current_selection;
};
#define MAX_MENUS 6
static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };
static void init_oldmenu(const struct menu_item_ex *menu,
                     struct gui_synclist *lists, int selected, bool callback);
static void menu_talk_selected(int m);

/* used to allow for dynamic menus */
#define MAX_MENU_SUBITEMS 64
static int current_subitems[MAX_MENU_SUBITEMS];
static int current_subitems_count = 0;

void get_menu_callback(const struct menu_item_ex *m,
                        menu_callback_type *menu_callback) 
{
    if (m->flags&(MENU_HAS_DESC|MENU_DYNAMIC_DESC))
        *menu_callback= m->callback_and_desc->menu_callback;
    else 
        *menu_callback = m->menu_callback;
}

static int get_menu_selection(int selected_item, const struct menu_item_ex *menu)
{
    int type = (menu->flags&MENU_TYPE_MASK);
    if (type == MT_MENU && (selected_item<current_subitems_count))
        return current_subitems[selected_item];
    return selected_item;
}
static int find_menu_selection(int selected)
{
    int i;
    for (i=0; i< current_subitems_count; i++)
        if (current_subitems[i] == selected)
            return i;
    return 0;
}
static char * get_menu_item_name(int selected_item,void * data, char *buffer)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex *)data;
    int type = (menu->flags&MENU_TYPE_MASK);
    selected_item = get_menu_selection(selected_item, menu);
    
    (void)buffer;
    /* only MT_MENU or MT_RETURN_ID is allowed in here */
    if (type == MT_RETURN_ID)
    {
        if (menu->flags&MENU_DYNAMIC_DESC)
            return menu->menu_get_name_and_icon->list_get_name(selected_item,
                    menu->menu_get_name_and_icon->list_get_name_data, buffer);
        return (char*)menu->strings[selected_item];
    }
    
    menu = menu->submenus[selected_item];
    
    if ((menu->flags&MENU_DYNAMIC_DESC) && (type != MT_SETTING_W_TEXT))
        return menu->menu_get_name_and_icon->list_get_name(selected_item,
                    menu->menu_get_name_and_icon->list_get_name_data, buffer);
    
    type = (menu->flags&MENU_TYPE_MASK);
    if ((type == MT_SETTING) || (type == MT_SETTING_W_TEXT))
    {
        const struct settings_list *v
                = find_setting(menu->variable, NULL);
        if (v)
            return str(v->lang_id);
        else return "Not Done yet!";
    }
    return P2STR(menu->callback_and_desc->desc);
}
#ifdef HAVE_LCD_BITMAP
static void menu_get_icon(int selected_item, void * data, ICON * icon)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex *)data;
    int menu_icon = Icon_NOICON;
    selected_item = get_menu_selection(selected_item, menu);
    
    menu = menu->submenus[selected_item];
    if (menu->flags&MENU_HAS_DESC)
        menu_icon = menu->callback_and_desc->icon_id;
    else if (menu->flags&MENU_DYNAMIC_DESC)
        menu_icon = menu->menu_get_name_and_icon->icon_id;
    
    switch (menu->flags&MENU_TYPE_MASK)
    {
        case MT_SETTING:
        case MT_SETTING_W_TEXT:
            *icon = bitmap_icons_6x8[Icon_Menu_setting];
            break;
        case MT_MENU:
            if (menu_icon == Icon_NOICON)
                *icon = bitmap_icons_6x8[Icon_Submenu];
            else
                *icon = bitmap_icons_6x8[menu_icon];
            break;
        case MT_FUNCTION_CALL:
        case MT_FUNCTION_WITH_PARAM:
        case MT_RETURN_VALUE:
            if (menu_icon == Icon_NOICON)
                *icon = bitmap_icons_6x8[Icon_Menu_functioncall];
            else 
                *icon = bitmap_icons_6x8[menu_icon];
            break;
        default:
            *icon = NOICON;
    }
}
#endif

static void init_menu_lists(const struct menu_item_ex *menu,
                     struct gui_synclist *lists, int selected, bool callback)
{
    int i, count = (menu->flags&MENU_COUNT_MASK)>>MENU_COUNT_SHIFT;
    menu_callback_type menu_callback = NULL;
    ICON icon = NOICON;
    current_subitems_count = 0;

    if ((menu->flags&MENU_TYPE_MASK) == MT_OLD_MENU)
    {
        init_oldmenu(menu, lists, selected, callback);
        return;
    }
    
    for (i=0; i<count; i++)
    {
        get_menu_callback(menu->submenus[i],&menu_callback);
        if (menu_callback)
        {
            if (menu_callback(ACTION_REQUEST_MENUITEM,menu->submenus[i])
                    != ACTION_EXIT_MENUITEM)
            {
                current_subitems[current_subitems_count] = i;
                current_subitems_count++;
            }
        }
        else 
        {
            current_subitems[current_subitems_count] = i;
            current_subitems_count++;
        }
    }
    
    gui_synclist_init(lists,get_menu_item_name,(void*)menu,false,1);
#ifdef HAVE_LCD_BITMAP
    if (menu->callback_and_desc->icon_id == Icon_NOICON)
        icon = bitmap_icons_6x8[Icon_Submenu_Entered];
    else
        icon = bitmap_icons_6x8[menu->callback_and_desc->icon_id];
    gui_synclist_set_title(lists, P2STR(menu->callback_and_desc->desc), icon);  
    gui_synclist_set_icon_callback(lists, menu_get_icon);
#else
    (void)icon;
    gui_synclist_set_icon_callback(lists, NULL);
#endif
    gui_synclist_set_nb_items(lists,current_subitems_count);
    gui_synclist_limit_scroll(lists,true);
    gui_synclist_select_item(lists, find_menu_selection(selected));
    
    get_menu_callback(menu,&menu_callback);
    if (callback && menu_callback)
        menu_callback(ACTION_ENTER_MENUITEM,menu);
}

static void talk_menu_item(const struct menu_item_ex *menu,
                    struct gui_synclist *lists)
{
    int id = -1;
    int type;
    unsigned char *str;
    int sel;
    
    if (global_settings.talk_menu)
    {
        if ((menu->flags&MENU_TYPE_MASK) == MT_OLD_MENU)
        {
            menus[menu->value].current_selection = 
                gui_synclist_get_sel_pos(lists);
            menu_talk_selected(menu->value);
            return;
        }
        sel = get_menu_selection(gui_synclist_get_sel_pos(lists),menu);
        if ((menu->flags&MENU_TYPE_MASK) == MT_MENU)
        {
            type = menu->submenus[sel]->flags&MENU_TYPE_MASK;
            if ((type == MT_SETTING) || (type == MT_SETTING_W_TEXT))
                talk_setting(menu->submenus[sel]->variable);
            else 
            {
                if (menu->submenus[sel]->flags&(MENU_DYNAMIC_DESC))
                {
                    char buffer[80];
                    str = menu->submenus[sel]->menu_get_name_and_icon->
                        list_get_name(sel, menu->submenus[sel]->
                                      menu_get_name_and_icon->
                                      list_get_name_data, buffer);
                    id = P2ID(str);
                }
                else
                    id = P2ID(menu->submenus[sel]->callback_and_desc->desc);
                if (id != -1)
                   talk_id(id,false);
            }
        }
    }
}
#define MAX_OPTIONS 32
/* returns true if the menu needs to be redrwan */
bool do_setting_from_menu(const struct menu_item_ex *temp)
{
    int setting_id;
    const struct settings_list *setting = find_setting(
                                               temp->variable,
                                               &setting_id);
    bool ret_val = false;
    unsigned char *title;
    if (setting)
    {
        if ((temp->flags&MENU_TYPE_MASK) == MT_SETTING_W_TEXT)
            title = temp->callback_and_desc->desc;
        else
            title = ID2P(setting->lang_id);

        if ((setting->flags&F_BOOL_SETTING) == F_BOOL_SETTING)
        {
            bool temp_var, *var;
            bool show_icons = global_settings.show_icons;
            if (setting->flags&F_TEMPVAR)
            {
                temp_var = *(bool*)setting->setting;
                var = &temp_var;
            }
            else
            {
                var = (bool*)setting->setting;
            }
            set_bool_options(P2STR(title), var,
                        STR(setting->bool_setting->lang_yes),
                        STR(setting->bool_setting->lang_no),
                        setting->bool_setting->option_callback);
            if (setting->flags&F_TEMPVAR)
                *(bool*)setting->setting = temp_var;
            if (show_icons != global_settings.show_icons)
                ret_val = true;
        }
        else if (setting->flags&F_T_SOUND)
        {
            set_sound(P2STR(title), setting->setting,
                        setting->sound_setting->setting);
        }
        else /* other setting, must be an INT type */
        {
            int temp_var, *var;
            if (setting->flags&F_TEMPVAR)
            {
                temp_var = *(int*)setting->setting;
                var = &temp_var;
            }
            else
            {
                var = (int*)setting->setting;
            }
            if (setting->flags&F_INT_SETTING)
            {
                int min, max, step;
                if (setting->flags&F_FLIPLIST)
                {
                    min = setting->int_setting->max;
                    max = setting->int_setting->min;
                    step = -setting->int_setting->step;
                }
                else
                {
                    max = setting->int_setting->max;
                    min = setting->int_setting->min;
                    step = setting->int_setting->step;
                }
                set_int_ex(P2STR(title), NULL,
                        setting->int_setting->unit,var,
                        setting->int_setting->option_callback,
                        step, min, max,
                        setting->int_setting->formatter,
                        setting->int_setting->get_talk_id);
            }
            else if (setting->flags&F_CHOICE_SETTING)
            {
                static struct opt_items options[MAX_OPTIONS];
                char buffer[256];
                char *buf_start = buffer;
                int buf_free = 256;
                int i,j, count = setting->choice_setting->count;
                for (i=0, j=0; i<count && i<MAX_OPTIONS; i++)
                {
                    if (setting->flags&F_CHOICETALKS)
                    {
                        if (cfg_int_to_string(setting_id, i,
                                            buf_start, buf_free))
                        {
                            int len = strlen(buf_start) +1;
                            options[j].string = buf_start;
                            buf_start += len;
                            buf_free -= len;
                            options[j].voice_id = 
                                setting->choice_setting->talks[i];
                            j++;
                        }
                    }
                    else
                    {
                        options[j].string =
                            P2STR(setting->
                                  choice_setting->desc[i]);
                        options[j].voice_id = 
                            P2ID(setting->
                                  choice_setting->desc[i]);
                        j++;
                    }
                }
                set_option(P2STR(title), var, INT,
                            options,j,
                            setting->
                               choice_setting->option_callback);
            }
            if (setting->flags&F_TEMPVAR)
                *(int*)setting->setting = temp_var;
        }
    }
    return ret_val;
}

int do_menu(const struct menu_item_ex *start_menu, int *start_selected)
{
    int selected = start_selected? *start_selected : 0;
    int action;
    struct gui_synclist lists;
    const struct menu_item_ex *temp, *menu;
    int ret = 0;
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif

    const struct menu_item_ex *menu_stack[MAX_MENUS];
    int menu_stack_selected_item[MAX_MENUS];
    int stack_top = 0;
    bool in_stringlist, done = false;
    menu_callback_type menu_callback = NULL;
    if (start_menu == NULL)
        menu = &main_menu_;
    else menu = start_menu;
#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&buttonbar);
    gui_buttonbar_set_display(&buttonbar, &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&buttonbar, "<<<", "", "");
    gui_buttonbar_draw(&buttonbar);
#endif
    init_menu_lists(menu,&lists,selected,true);
    in_stringlist = ((menu->flags&MENU_TYPE_MASK) == MT_RETURN_ID);
    
    talk_menu_item(menu, &lists);
    
    gui_synclist_draw(&lists);
    gui_syncstatusbar_draw(&statusbars, true);
    action_signalscreenchange();
    
    /* load the callback, and only reload it if menu changes */
    get_menu_callback(menu, &menu_callback);
    
    while (!done)
    {
        action = get_action(CONTEXT_MAINMENU,HZ); 
        /* HZ so the status bar redraws corectly */
        if (action == ACTION_NONE)
        {
            gui_syncstatusbar_draw(&statusbars, true);
            continue;
        }

        
        if (menu_callback)
        {
            int old_action = action;
            action = menu_callback(action, menu);
            if (action == ACTION_EXIT_AFTER_THIS_MENUITEM)
            {
                action = old_action;
                ret = MENU_SELECTED_EXIT; /* will exit after returning
                                             from selection */
            }
        }

        if (gui_synclist_do_button(&lists,action,LIST_WRAP_UNLESS_HELD))
        {
            talk_menu_item(menu, &lists);
        }
        else if (action == ACTION_MENU_WPS)
        {
            ret = GO_TO_PREVIOUS_MUSIC;
            done = true;
        }
        else if (action == ACTION_MENU_STOP)
        {
            if (audio_status() && !global_settings.party_mode)
            {
                if (global_settings.fade_on_stop)
                    fade(0);
                bookmark_autobookmark();
                audio_stop();
            }
        }
        else if (action == ACTION_STD_MENU)
        {
            if ((menu->flags&MENU_TYPE_MASK) == MT_OLD_MENU)
                return MENU_SELECTED_EXIT;
            if (menu != &root_menu_)
                ret = GO_TO_ROOT;
            else
                ret = GO_TO_PREVIOUS;
            done = true;
        }
        else if (action == ACTION_STD_CANCEL)
        {
            in_stringlist = false;
            if (menu_callback)
                menu_callback(ACTION_EXIT_MENUITEM, menu);
            
            if (stack_top > 0)
            {
                stack_top--;
                menu = menu_stack[stack_top];
                init_menu_lists(menu, &lists, 
                                 menu_stack_selected_item[stack_top], false);
                talk_menu_item(menu, &lists);
                /* new menu, so reload the callback */
                get_menu_callback(menu, &menu_callback);
            }
            else if (menu != &root_menu_)
            {
                ret = GO_TO_PREVIOUS;
                done = true;
            }
        }
        else if (action == ACTION_STD_OK)
        {
            int type;
#ifdef HAS_BUTTONBAR
            gui_buttonbar_unset(&buttonbar);
            gui_buttonbar_draw(&buttonbar);
#endif
            if ((menu->flags&MENU_TYPE_MASK) == MT_OLD_MENU)
            {
                selected = gui_synclist_get_sel_pos(&lists);
                menus[menu->value].current_selection = selected;
                return selected;
            }
            selected = get_menu_selection(gui_synclist_get_sel_pos(&lists), menu);
            temp = menu->submenus[selected];
            if (in_stringlist)
                type = (menu->flags&MENU_TYPE_MASK);
            else 
                type = (temp->flags&MENU_TYPE_MASK);
            get_menu_callback(temp, &menu_callback);
            if (menu_callback)
            {
                action = menu_callback(ACTION_ENTER_MENUITEM,temp);
                if (action == ACTION_EXIT_MENUITEM)
                    break;
            }
            switch (type)
            {
                case MT_MENU:
                    if (stack_top < MAX_MENUS)
                    {
                        menu_stack[stack_top] = menu;
                        menu_stack_selected_item[stack_top] = selected;
                        stack_top++;
                        init_menu_lists(temp, &lists, 0, true);
                        menu = temp;
                        talk_menu_item(menu, &lists);
                    }
                    break;
                case MT_FUNCTION_CALL:
                    action_signalscreenchange();
                    temp->function();
                    break;
                case MT_FUNCTION_WITH_PARAM:
                    action_signalscreenchange();
                    temp->func_with_param->function(
                                    temp->func_with_param->param);
                    break;
                case MT_SETTING:
                case MT_SETTING_W_TEXT:
                {
                    if (do_setting_from_menu(temp))
                        init_menu_lists(menu, &lists, 0, true);
                    break;
                }
                case MT_RETURN_ID:
                    if (in_stringlist)
                    {
                        action_signalscreenchange();
                        return selected;
                    }
                    else if (stack_top < MAX_MENUS)
                    {
                        menu_stack[stack_top] = menu;
                        menu_stack_selected_item[stack_top] = selected;
                        stack_top++;
                        menu = temp;
                        init_menu_lists(menu,&lists,0,false);
                        in_stringlist = true;
                    }
                    break;
                case MT_RETURN_VALUE:
                    ret = temp->value;
                    done = true;
                    break;
            }
            if (type != MT_MENU && menu_callback)
                menu_callback(ACTION_EXIT_MENUITEM,temp);
            /* callback was changed, so reload the menu's callback */
            get_menu_callback(menu, &menu_callback);
#ifdef HAS_BUTTONBAR
            gui_buttonbar_set(&buttonbar, "<<<", "", "");
            gui_buttonbar_draw(&buttonbar);
#endif
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            ret = MENU_ATTACHED_USB;
        gui_syncstatusbar_draw(&statusbars, true);
        gui_synclist_draw(&lists);
    }
    action_signalscreenchange();
    if (start_selected)
        *start_selected = get_menu_selection(
                            gui_synclist_get_sel_pos(&lists), menu);
    return ret;
}

int main_menu(void)
{
    return do_menu(NULL, 0);
}

/* wrappers for the old menu system to work with the new system */


static int menu_find_free(void)
{
    int i;
    /* Tries to find an unused slot to put the new menu */
    for ( i=0; i<MAX_MENUS; i++ ) {
        if ( !inuse[i] ) {
            inuse[i] = true;
            break;
        }
    }
    if ( i == MAX_MENUS ) {
        DEBUGF("Out of menus!\n");
        return -1;
    }
    return(i);
}

int menu_init(const struct menu_item* mitems, int count, int (*callback)(int, int),
              const char *button1, const char *button2, const char *button3)
{
    (void)button1;
    (void)button2;
    (void)button3;
    int menu=menu_find_free();
    if(menu==-1)/* Out of menus */
        return -1;
    menus[menu].items = (struct menu_item*)mitems; /* de-const */
    menus[menu].count = count;
    menus[menu].callback = callback;
    menus[menu].current_selection = 0;
    return menu;
}

void menu_exit(int m)
{
    inuse[m] = false;
}



static int oldmenuwrapper_callback(int action, 
                                    const struct menu_item_ex *this_item)
{
    if (menus[this_item->value].callback)
    {
        int val = menus[this_item->value].callback(action, this_item->value);
        switch (val)
        {
            case MENU_SELECTED_EXIT:
                return ACTION_EXIT_MENUITEM;
        }
        return val;
    }
    return action;
}

static char* oldmenuwrapper_getname(int selected_item,
                                    void * data, char *buffer)
{
    (void)buffer;
    unsigned char* desc = menus[(intptr_t)data].items[selected_item].desc;
    return P2STR(desc);
}
static void init_oldmenu(const struct menu_item_ex *menu,
                     struct gui_synclist *lists, int selected, bool callback)
{
    (void)callback;
    gui_synclist_init(lists, oldmenuwrapper_getname, 
                        (void*)menu->value, false, 1);
    gui_synclist_set_nb_items(lists,
        (menu->flags&MENU_COUNT_MASK)>>MENU_COUNT_SHIFT);
    gui_synclist_limit_scroll(lists, true);
    gui_synclist_select_item(lists, selected);
}

static void menu_talk_selected(int m)
{
    int selected = menus[m].current_selection;
    int voice_id = P2ID(menus[m].items[selected].desc);
    if (voice_id >= 0) /* valid ID given? */
        talk_id(voice_id, false); /* say it */
}

int menu_show(int m)
{
    struct menu_item_ex menu;
    struct menu_get_name_and_icon menu_info = 
    {
        oldmenuwrapper_callback, 
        oldmenuwrapper_getname,
        (void*)m, Icon_NOICON
    };

    menu.flags = (MENU_TYPE_MASK&MT_OLD_MENU) | MENU_DYNAMIC_DESC |
                 MENU_ITEM_COUNT(menus[m].count);
    menu.value = m;
    menu.menu_get_name_and_icon = &menu_info;
    return do_menu(&menu, &menus[m].current_selection);
}


bool menu_run(int m)
{
    int selected;
    while (1) {
        switch (selected=menu_show(m))
        {
            case MENU_SELECTED_EXIT:
                return false;

            case MENU_ATTACHED_USB:
                return true;

            default:
            {
                if (selected >= 0 && selected < menus[m].count)
                {
                    if (menus[m].items[selected].function &&
                        menus[m].items[selected].function())
                        return  true;
                }
                gui_syncstatusbar_draw(&statusbars, true);
            }
        }
    }
    return false;
}

/*
 *  Property function - return the "count" of menu items in "menu"
 */

int menu_count(int menu)
{
    return menus[menu].count;
}

