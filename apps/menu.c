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

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "buttonbar.h"

struct menu {
    struct menu_item* items;
    int (*callback)(int, int);
#ifdef HAS_BUTTONBAR
    struct gui_buttonbar buttonbar;
#endif
    struct gui_synclist synclist;
};

#define MAX_MENUS 6

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

static char * menu_get_itemname(int selected_item, void * data, char *buffer)
{
    struct menu *local_menus=(struct menu *)data;
    (void)buffer;
    return(P2STR(local_menus->items[selected_item].desc));
}

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
    int menu=menu_find_free();
    if(menu==-1)/* Out of menus */
        return -1;
    menus[menu].items = (struct menu_item*)mitems; /* de-const */
    gui_synclist_init(&(menus[menu].synclist),
                      &menu_get_itemname, &menus[menu], false, 1);
    gui_synclist_set_icon_callback(&(menus[menu].synclist), NULL);
    gui_synclist_set_nb_items(&(menus[menu].synclist), count);
    menus[menu].callback = callback;
#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&(menus[menu].buttonbar));
    gui_buttonbar_set_display(&(menus[menu].buttonbar), &(screens[SCREEN_MAIN]) );
    gui_buttonbar_set(&(menus[menu].buttonbar), button1, button2, button3);
#else
    (void)button1;
    (void)button2;
    (void)button3;
#endif
    return menu;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

int menu_show(int m)
{
#ifdef HAS_BUTTONBAR
    gui_buttonbar_draw(&(menus[m].buttonbar));
#endif
    bool exit = false;
    int key;

    gui_synclist_draw(&(menus[m].synclist));
    gui_syncstatusbar_draw(&statusbars, true);
    menu_talk_selected(m);
    while (!exit) {
        key = get_action(CONTEXT_MAINMENU,HZ/2);
        /*
         * "short-circuit" the default keypresses by running the
         * callback function
         * The callback may return a new key value, often this will be
         * BUTTON_NONE or the same key value, but it's perfectly legal
         * to "simulate" key presses by returning another value.
         */
        if( menus[m].callback != NULL )
            key = menus[m].callback(key, m);
        /* If moved, "say" the entry under the cursor */
        if(gui_synclist_do_button(&(menus[m].synclist), key,LIST_WRAP_UNLESS_HELD))
            menu_talk_selected(m);
        switch( key ) {
            case ACTION_STD_OK:
                action_signalscreenchange();
                return gui_synclist_get_sel_pos(&(menus[m].synclist));


            case ACTION_STD_CANCEL:
            case ACTION_STD_MENU:
                exit = true;
                break;

            default:
                if(default_event_handler(key) == SYS_USB_CONNECTED)
                    return MENU_ATTACHED_USB;
                break;
        }
        gui_syncstatusbar_draw(&statusbars, false);
    }
    action_signalscreenchange();
    return MENU_SELECTED_EXIT;
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
                if (menus[m].items[selected].function &&
                    menus[m].items[selected].function())
                    return  true;
                gui_syncstatusbar_draw(&statusbars, true);
            }
        }
    }
    return false;
}

/*
 *  Property function - return the current cursor for "menu"
 */

int menu_cursor(int menu)
{
    return gui_synclist_get_sel_pos(&(menus[menu].synclist));
}

/*
 *  Property function - return the "menu" description at "position"
 */

char* menu_description(int menu, int position)
{
    return P2STR(menus[menu].items[position].desc);
}

/*
 *  Delete the element "position" from the menu items in "menu"
 */

void menu_delete(int menu, int position)
{
    int i;
    int nb_items=gui_synclist_get_nb_items(&(menus[menu].synclist));
    /* copy the menu item from the one below */
    for( i = position; i < nb_items - 1; i++)
        menus[menu].items[i] = menus[menu].items[i + 1];

    gui_synclist_del_item(&(menus[menu].synclist));
}

void menu_insert(int menu, int position, char *desc, bool (*function) (void))
{
    int i;
    int nb_items=gui_synclist_get_nb_items(&(menus[menu].synclist));
    if(position < 0)
       position = nb_items;

    /* Move the items below one position forward */
    for( i = nb_items; i > position; i--)
       menus[menu].items[i] = menus[menu].items[i - 1];

    /* Update the current item */
    menus[menu].items[position].desc = (unsigned char *)desc;
    menus[menu].items[position].function = function;
    gui_synclist_add_item(&(menus[menu].synclist));
}

/*
 *  Property function - return the "count" of menu items in "menu"
 */

int menu_count(int menu)
{
    return gui_synclist_get_nb_items(&(menus[menu].synclist));
}

/*
 * Allows to set the cursor position. Doesn't redraw by itself.
 */

void menu_set_cursor(int menu, int position)
{
    gui_synclist_select_item(&(menus[menu].synclist), position);
}

void menu_talk_selected(int m)
{
    if(global_settings.talk_menu)
    {
        int selected=gui_synclist_get_sel_pos(&(menus[m].synclist));
        int voice_id = P2ID(menus[m].items[selected].desc);
        if (voice_id >= 0) /* valid ID given? */
            talk_id(voice_id, false); /* say it */
    }
}

void menu_draw(int m)
{
    gui_synclist_draw(&(menus[m].synclist));
}

/******************************************************************/
/*              New menu stuff here!!
 ******************************************************************/


/* used to allow for dynamic menus */
#define MAX_MENU_SUBITEMS 64
static int current_subitems[MAX_MENU_SUBITEMS];
static int current_subitems_count = 0;

void get_menu_callback(const struct menu_item_ex *m,
                        menu_callback_type *menu_callback) 
{
    if (m->flags&MENU_HAS_DESC)
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

static char * get_menu_item_name(int selected_item,void * data, char *buffer)
{
    const struct menu_item_ex *menu = (const struct menu_item_ex *)data;
    int type = (menu->flags&MENU_TYPE_MASK);
    selected_item = get_menu_selection(selected_item, menu);
    
    (void)buffer;
    /* only MT_MENU or MT_RETURN_ID is allowed in here */
    if (type == MT_RETURN_ID)
    {
        return (char*)menu->strings[selected_item];
    }
    
    menu = menu->submenus[selected_item];
    type = (menu->flags&MENU_TYPE_MASK);
    if (type == MT_SETTING)
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
    selected_item = get_menu_selection(selected_item, menu);
    
    menu = menu->submenus[selected_item];
    switch (menu->flags&MENU_TYPE_MASK)
    {
        case MT_SETTING:
            *icon = bitmap_icons_6x8[Icon_Menu_setting];
            break;
        case MT_MENU:
            if (menu->callback_and_desc->icon == NOICON)
                *icon = bitmap_icons_6x8[Icon_Submenu];
            else 
                *icon = menu->callback_and_desc->icon;
            break;
        case MT_FUNCTION_CALL:
        case MT_FUNCTION_WITH_PARAM:
            if (menu->callback_and_desc->icon == NOICON)
                *icon = bitmap_icons_6x8[Icon_Menu_functioncall];
            else 
                *icon = menu->callback_and_desc->icon;
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
    if (global_settings.show_icons == false)
        icon = NOICON;
    else if (menu->callback_and_desc->icon == NOICON)
        icon = bitmap_icons_6x8[Icon_Submenu_Entered];
    else
        icon = menu->callback_and_desc->icon;
    gui_synclist_set_title(lists, P2STR(menu->callback_and_desc->desc), icon);  
    if (global_settings.show_icons)
        gui_synclist_set_icon_callback(lists, menu_get_icon);
    else 
#else
    (void)icon;
#endif
        gui_synclist_set_icon_callback(lists, NULL);
    gui_synclist_set_nb_items(lists,current_subitems_count);
    gui_synclist_limit_scroll(lists,true);
    gui_synclist_select_item(lists, selected);
    
    get_menu_callback(menu,&menu_callback);
    if (callback && menu_callback)
        menu_callback(ACTION_ENTER_MENUITEM,menu);
}

static void talk_menu_item(const struct menu_item_ex *menu,
                    struct gui_synclist *lists)
{
    int id = -1;
    if (global_settings.talk_menu)
    {
        int sel = get_menu_selection(gui_synclist_get_sel_pos(lists),menu);
        if ((menu->flags&MENU_TYPE_MASK) == MT_MENU)
        {
           if ((menu->submenus[sel]->flags&MENU_TYPE_MASK) == MT_SETTING)
               talk_setting(menu->submenus[sel]->variable);
           else 
           {
               id = P2ID(menu->submenus[sel]->callback_and_desc->desc);
               if (id != -1)
                   talk_id(id,false);
           }
        }
    }
}
#define MAX_OPTIONS 32
int do_menu(const struct menu_item_ex *start_menu)
{
    int action;
    int selected = 0;
    struct gui_synclist lists;
    const struct menu_item_ex *temp, *menu;
    int ret = 0;
    
    const struct menu_item_ex *menu_stack[MAX_MENUS];
    int menu_stack_selected_item[MAX_MENUS];
    int stack_top = 0;
    bool in_stringlist;
    menu_callback_type menu_callback = NULL;
    if (start_menu == NULL)
        menu = &main_menu_;
    else menu = start_menu;

    init_menu_lists(menu,&lists,selected,true);
    in_stringlist = ((menu->flags&MENU_TYPE_MASK) == MT_RETURN_ID);
    
    talk_menu_item(menu, &lists);
    
    gui_synclist_draw(&lists);
    gui_syncstatusbar_draw(&statusbars, true);
    action_signalscreenchange();
    
    while (ret == 0)
    {
        action = get_action(CONTEXT_MAINMENU,HZ); 
        /* HZ so the status bar redraws corectly */
        if (action == ACTION_NONE)
        {
            gui_syncstatusbar_draw(&statusbars, true);
            continue;
        }

        get_menu_callback(menu,&menu_callback);
        if (menu_callback)
        {
            action = menu_callback(action,menu);
        }

        if (gui_synclist_do_button(&lists,action,LIST_WRAP_UNLESS_HELD))
        {
            talk_menu_item(menu, &lists);
        }
        else if (action == ACTION_MENU_WPS)
        {
            ret = MENU_RETURN_TO_WPS;
        }
        else if ((action == ACTION_STD_CANCEL) ||
                 (action == ACTION_STD_MENU))
        {
            in_stringlist = false;
            
            if (stack_top > 0)
            {
                get_menu_callback(menu,&menu_callback);
                if (menu_callback)
                {
                    if (menu_callback(action,menu) ==
                            ACTION_EXIT_MENUITEM)
                        break;
                }
                stack_top--;
                menu = menu_stack[stack_top];
                init_menu_lists(menu, &lists, 
                                 menu_stack_selected_item[stack_top], false);
                talk_menu_item(menu, &lists);
            }
            else 
            {
                get_menu_callback(menu, &menu_callback);
                if (menu_callback)
                    menu_callback(ACTION_EXIT_MENUITEM, menu);
                break;
            }
        }
        else if (action == ACTION_STD_OK)
        {
            int type;
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
                        menu_stack_selected_item[stack_top]
                                = gui_synclist_get_sel_pos(&lists);
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
                {
                    int setting_id;
                    const struct settings_list *setting = find_setting(
                                                               temp->variable,
                                                               &setting_id);
                    if (setting)
                    {
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
                            set_bool_options(str(setting->lang_id),var,
                                        STR(setting->bool_setting->lang_yes),
                                        STR(setting->bool_setting->lang_no),
                                        setting->bool_setting->option_callback);
                            if (setting->flags&F_TEMPVAR)
                                *(bool*)setting->setting = temp_var;
                            if (show_icons != global_settings.show_icons)
                                init_menu_lists(menu, &lists, 0, true);
                        }
                        else if (setting->flags&F_T_SOUND)
                        {
                            set_sound(str(setting->lang_id), setting->setting,
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
                                set_int_ex(str(setting->lang_id),
                                        NULL,
                                        setting->int_setting->unit,var,
                                        setting->int_setting->option_callback,
                                        setting->int_setting->step,
                                        setting->int_setting->min,
                                        setting->int_setting->max,
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
                                set_option(str(setting->lang_id), var, INT,
                                            options,j,
                                            setting->
                                               choice_setting->option_callback);
                            }
                            if (setting->flags&F_TEMPVAR)
                                *(int*)setting->setting = temp_var;
                        }
                    }
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
            }
            get_menu_callback(temp,&menu_callback);
            if (type != MT_MENU && menu_callback)
                menu_callback(ACTION_EXIT_MENUITEM,temp);
        }
        else if(default_event_handler(action) == SYS_USB_CONNECTED)
            ret = MENU_ATTACHED_USB;
        gui_syncstatusbar_draw(&statusbars, true);
        gui_synclist_draw(&lists);
    }
    action_signalscreenchange();
    return ret;
}

int main_menu(void)
{
    return do_menu(NULL);
}
