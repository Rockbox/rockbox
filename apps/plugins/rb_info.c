/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 William Wilgus
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

/* WIP rb_info common info that you wonder about when rockboxing?
 */

#include "plugin.h"
#include "lang_enum.h"
#include "../open_plugin.h"
#include "logf.h"
#include "lib/action_helper.h"
#include "lib/button_helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/printcell_helper.h"

#define MENU_ID(x) (((void*)&"RPBUTACNGSX\0" + x))
enum {
    M_ROOT = 0,
    M_PATHS,
    M_BUFFERS,
    M_BUTTONS,
    M_BTNTEST,
    M_ACTIONS,
    M_CONTEXTS,
    M_ACTTEST,
    M_PLUGINS,
    M_TESTPUT,
    M_EXIT,
    M_LAST_ITEM //ITEM COUNT
};

#define MENU_ID_PLUGINS_ITEMS 5

/*Action test and Button test*/
static struct menu_test_t {
    int count;
    int context;
    int last_btn_or_act;
} m_test;

struct menu_buffer_t { const char *name; size_t size;};
static const struct menu_buffer_t m_buffer[] =
{
#ifndef MAX_LOGF_SIZE
#define MAX_LOGF_SIZE (0)
#endif
#ifndef CACHE_SIZE
#define CACHE_SIZE (0)
#endif
    {"thread stack", DEFAULT_STACK_SIZE},
    {"plugin buffer", PLUGIN_BUFFER_SIZE},
    {"frame_buffer", FRAMEBUFFER_SIZE},
    {"codec_buffer", CODEC_SIZE},
    {"logf_buffer", MAX_LOGF_SIZE},
    {"cache", CACHE_SIZE},
};

/* stringify the macro value */
#define MACROVAL(x) MACROSTR(x)
#define MACROSTR(x) #x
static int main_last_sel = 0;
static struct gui_synclist lists;
static void synclist_set(char*, int, int, int);

struct paths { const char *name; const char *path; };
static const struct paths paths[] = {
    {"Home",        ""HOME_DIR},
    {"Rockbox",     ""ROCKBOX_DIR},
    {"Plugins",     ""PLUGIN_DIR},
    {"Codecs",      ""CODECS_DIR},
    {"WPS",         ""WPS_DIR},
    {"SBS",         ""SBS_DIR},
    {"Theme",       ""THEME_DIR},
    {"Font",        ""FONT_DIR},
    {"Icon",        ""ICON_DIR},
    {"Backdrop",    ""BACKDROP_DIR},
    {"Eq",          ""EQS_DIR},
    {"Rec Presets", ""RECPRESETS_DIR},
    {"Recordings",  ""REC_BASE_DIR,},
    {"Fm Presets",  ""FMPRESET_PATH},
    {"MAX_PATH",    ""MACROVAL(MAX_PATH)" bytes"},
};
#define TESTPUT_HEADER "$*64$col1$col2$*128$col3$col4$col5$col6$*64$col7$col8"
static int testput_cols = 0;
struct mainmenu { const char *name; void *menuid; int items;};
static struct mainmenu mainmenu[M_LAST_ITEM] = {
#define MENU_ITEM(ID, NAME, COUNT) [ID]{NAME, MENU_ID(ID), (int)COUNT}
MENU_ITEM(M_ROOT, "Rockbox Info Plugin", M_LAST_ITEM),
MENU_ITEM(M_PATHS, ID2P(LANG_SHOW_PATH), ARRAYLEN(paths)),
MENU_ITEM(M_BUFFERS, ID2P(LANG_BUFFER_STAT), ARRAYLEN(m_buffer)),
MENU_ITEM(M_BUTTONS, "Buttons", -1), /* Set at runtime in plugin_start: */
MENU_ITEM(M_BTNTEST, "Button test", 2),
MENU_ITEM(M_ACTIONS, "Actions", LAST_ACTION_PLACEHOLDER),
MENU_ITEM(M_CONTEXTS, "Contexts", LAST_CONTEXT_PLACEHOLDER ),
MENU_ITEM(M_ACTTEST, "Action test", 3),
MENU_ITEM(M_PLUGINS, ID2P(LANG_PLUGINS), MENU_ID_PLUGINS_ITEMS),
MENU_ITEM(M_TESTPUT, "Printcell test", 36),
MENU_ITEM(M_EXIT, ID2P(LANG_MENU_QUIT), 0),
#undef MENU_ITEM
};

static const struct mainmenu *mainitem(int selected_item)
{
    static const struct mainmenu empty = {0};
    if (selected_item >= 0 && selected_item < (int) ARRAYLEN(mainmenu))
        return &mainmenu[selected_item];
    else
        return &empty;
}

static void cleanup(void *parameter)
{
    (void)parameter;
}

#if 0
static enum themable_icons menu_icon_cb(int selected_item, void * data)
{
    (void)data;
    (void)selected_item;
    return Icon_NOICON;
}
#endif

static const char *menu_plugin_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    (void)data;
    buf[0] = '\0';
    switch(selected_item)
    {
        case 0:
            rb->snprintf(buf, buf_len, "%s: [%d bytes] ", "plugin_api", (int)sizeof(struct plugin_api));
            break;
        case 1:
            rb->snprintf(buf, buf_len, "%s: [%d bytes] ", "plugin buffer", PLUGIN_BUFFER_SIZE);
            break;
        case 2:
            rb->snprintf(buf, buf_len, "%s: [%d bytes] ", "frame_buffer", (int)FRAMEBUFFER_SIZE);
            break;
        case 3:
            rb->snprintf(buf, buf_len, "%s: [W: %d H:%d] ", "LCD", LCD_WIDTH, LCD_HEIGHT);
            break;
        case 4:
            rb->snprintf(buf, buf_len, "%s: [%d bits] ", "fb_data", (int)(sizeof(fb_data) * CHAR_BIT));
            break;
        case 5:
            break;
    }
    return buf;
}

static const char *menu_button_test_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    (void)data;
    int curbtn = BUTTON_NONE;
    buf[0] = '\0';
    switch(selected_item)
    {
        case 0:
             rb->snprintf(buf, buf_len, "%s: [%s] ", "Button test",
                m_test.count > 0 ? "true":"false");
             break;
        case 1:
            if (m_test.count > 0)
            {
                if (m_test.count <= 2)
                    curbtn = rb->button_get_w_tmo(HZ * 2);
                else
                    m_test.last_btn_or_act = BUTTON_NONE;
                if (curbtn == BUTTON_NONE)
                {
                    m_test.count--;
                }
                else
                    m_test.last_btn_or_act = curbtn;
            }
            get_button_names(buf, buf_len, m_test.last_btn_or_act);

            break;
    }
    return buf;
}

static const char *menu_action_test_name_cb(int selected_item, void* data,
                                           char* buf, size_t buf_len)
{
    (void)data;
    const char *fmtstr;
    int curact = ACTION_NONE;
    buf[0] = '\0';
    switch(selected_item)
    {
        case 0:
             rb->snprintf(buf, buf_len, "%s: [%s] ", "Action test",
                m_test.count > 0 ? "true":"false");
             break;
        case 1:
            if (m_test.count <= 0)
            {
                if (m_test.context <= 0)
                    fmtstr = "%s > ";
                else if (m_test.context >= LAST_CONTEXT_PLACEHOLDER - 1)
                    fmtstr = "< %s   ";
                else
                    fmtstr = "< %s > ";
            }
            else
                fmtstr = "%s";

            rb->snprintf(buf, buf_len, fmtstr, context_name(m_test.context));
            break;
        case 2:
            if (m_test.count > 0)
            {
                if (m_test.count <= 2)
                    curact = rb->get_action(m_test.context, HZ * 2);
                else
                    m_test.last_btn_or_act = ACTION_NONE;
                if (curact == ACTION_NONE && rb->button_get(false) == BUTTON_NONE)
                {
                    m_test.count--;
                }
                else
                {
                    m_test.last_btn_or_act = curact;
                    m_test.count = 2;
                }
            }
            return action_name(m_test.last_btn_or_act);

            break;
    }
    return buf;
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    buf[0] = '\0';
    if (data == MENU_ID(M_ROOT))
        return mainitem(selected_item)->name;
    else if (selected_item == 0 && data != MENU_ID(M_TESTPUT)) /*header text*/
        return mainitem(main_last_sel)->name;
    else if (selected_item >= mainitem(main_last_sel)->items - 1)
            return ID2P(LANG_BACK);

    if (data == MENU_ID(M_PATHS))
    {
        selected_item--;
        if (selected_item >= 0 && selected_item <  mainitem(M_PATHS)->items)
        {
            const struct paths *cur = &paths[selected_item];
            rb->snprintf(buf, buf_len, "%s: [%s] ", cur->name, cur->path);
            return buf;
        }
    }
    else if (data == MENU_ID(M_BUTTONS))
    {
        const struct available_button *btn = &available_buttons[selected_item - 1];
        rb->snprintf(buf, buf_len, "%s: [0x%X] ", btn->name, (unsigned int) btn->value);
        return buf;
    }
    else if (data == MENU_ID(M_BTNTEST))
        return menu_button_test_name_cb(selected_item - 1, data, buf, buf_len);
    else if (data == MENU_ID(M_ACTIONS))
        return action_name(selected_item - 1);
    else if (data == MENU_ID(M_CONTEXTS))
        return context_name(selected_item - 1);
    else if (data == MENU_ID(M_ACTTEST))
        return menu_action_test_name_cb(selected_item - 1, data, buf, buf_len);
    else if (data == MENU_ID(M_BUFFERS))
    {
        const struct menu_buffer_t *bufm = &m_buffer[selected_item - 1];
        rb->snprintf(buf, buf_len, "%s: [%ld bytes] ", bufm->name, (long)bufm->size);
        return buf;
    }
    else if (data == MENU_ID(M_PLUGINS))
    {
        return menu_plugin_name_cb(selected_item - 1, data, buf, buf_len);
    }
    else if (data == MENU_ID(M_TESTPUT))
    {
        rb->snprintf(buf, buf_len, "put_line item: [ %d ]$Text %d$Text LONGER TEST text %d $4$5$6$7$8$9", selected_item, 1, 2);
        return buf;
    }
    return buf;
}

static int list_voice_cb(int list_index, void* data)
{
    if (!rb->global_settings->talk_menu)
        return -1;

    if (data == MENU_ID(M_ROOT))
    {
        const char * name = mainitem(list_index)->name;
        long id = P2ID((const unsigned char *)name);
            if(id>=0)
                rb->talk_id(id, true);
            else
                rb->talk_spell(name, true);
    }
    else if (data == MENU_ID(M_BUFFERS) || data == MENU_ID(M_PLUGINS))
    {
        char buf[64];
        const char* name = list_get_name_cb(list_index, data, buf, sizeof(buf));
        long id = P2ID((const unsigned char *)name);
        if(id>=0)
            rb->talk_id(id, true);
        else
        {
            char* bytstr = rb->strcasestr(name, "bytes");
            if (bytstr != NULL)
                *bytstr = '\0';
            rb->talk_spell(name, true);
        }
    }
    else
    {
        char buf[64];
        const char* name = list_get_name_cb(list_index, data, buf, sizeof(buf));
        long id = P2ID((const unsigned char *)name);
        if(id>=0)
            rb->talk_id(id, true);
        else
            rb->talk_spell(name, true);
    }
    return 0;
}

int menu_action_cb(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    if (lists->data == MENU_ID(M_TESTPUT) && (selected_item < (mainitem(M_TESTPUT)->items) - 1)/*back*/)
    {
        if (*action == ACTION_STD_OK)
        {
            printcell_increment_column(lists, 1, true);
            *action = ACTION_NONE;
        }
        else if (*action == ACTION_STD_CANCEL)
        {
            if (printcell_increment_column(lists, -1, true) != testput_cols - 1)
            {
                *action = ACTION_NONE;
            }
        }
        else if (*action == ACTION_STD_CONTEXT)
        {
            char buf[PRINTCELL_MAXLINELEN];
            char* bufp = buf;
            bufp = printcell_get_selected_column_text(lists, bufp, PRINTCELL_MAXLINELEN);
            rb->splashf(HZ * 2, "Item: %s", bufp);
        }
    }
    else if (lists->data == MENU_ID(M_ACTTEST))
    {
        if (selected_item == 2) /* context */
        {
            int ctx = m_test.context;
            if (*action == ACTION_STD_OK)
                m_test.context++;
            else if (*action == ACTION_STD_CANCEL)
                m_test.context--;

            if (m_test.context < 0)
                m_test.context = 0;
            else if (m_test.context >= LAST_CONTEXT_PLACEHOLDER)
                m_test.context = LAST_CONTEXT_PLACEHOLDER - 1;

            if (ctx != m_test.context)
                rb->gui_synclist_speak_item(lists);

            goto default_handler;
        }
        if (*action == ACTION_STD_OK)
        {
            if (selected_item == 1 || selected_item == 3)
            {
                m_test.count = 3;
                rb->gui_synclist_select_item(lists, 3);
            }
        }
    }
    else if (lists->data == MENU_ID(M_BTNTEST))
    {
        if (*action == ACTION_STD_OK)
        {
            if (selected_item == 1 || selected_item == 2)
            {
                m_test.count = 3;
                rb->gui_synclist_select_item(lists, 2);
            }
        }
    }
/* common */
    if (*action == ACTION_STD_OK)
    {
            if (lists->data == MENU_ID(M_ROOT))
            {
                rb->memset(&m_test, 0, sizeof(struct menu_test_t));
                const struct mainmenu *cur = mainitem(selected_item);

                if (cur->menuid == NULL || cur->menuid == MENU_ID(M_EXIT))
                    *exit = true;
                else
                {
                    main_last_sel = selected_item;

                    if (cur->menuid == MENU_ID(M_TESTPUT))
                    {
                        synclist_set(cur->menuid, 0, cur->items, 1);
#if LCD_DEPTH > 1
                        /* If line sep is set to automatic then outline cells */
                        bool showlinesep = (rb->global_settings->list_separator_height < 0);
#else
                        bool showlinesep = (rb->global_settings->cursor_style == 0);
#endif
                        printcell_enable(lists, true, showlinesep);
                        //lists->callback_draw_item = test_listdraw_fn;
                    }
                    else
                    {
                        printcell_enable(lists, false, false);
                        synclist_set(cur->menuid, 1, cur->items, 1);
                    }
                    rb->gui_synclist_draw(lists);
                }
            }
            else if (selected_item <= 0) /* title */
            {
                rb->gui_synclist_select_item(lists, 1);
            }
            else if (selected_item >= (mainitem(main_last_sel)->items) - 1)/*back*/
            {
                *action = ACTION_STD_CANCEL;
            }
            else if (lists->data == MENU_ID(M_TESTPUT))
            {

            }
            else if (lists->data == MENU_ID(M_ACTIONS) ||
                     lists->data == MENU_ID(M_CONTEXTS))
            {
                char buf[MAX_PATH];
                const char *name = list_get_name_cb(selected_item, lists->data, buf, sizeof(buf));
                /* splash long enough to get fingers off button then wait for new button press */
                rb->splashf(HZ / 2, "%s %d (0x%X)", name, selected_item -1, selected_item -1);
                rb->button_get(true);
            }
    }
    if (*action == ACTION_STD_CANCEL)
    {
        if (lists->data == MENU_ID(M_TESTPUT))
        {
            //lists->callback_draw_item = NULL;
            printcell_enable(lists, false, false);
        }
        if (lists->data != MENU_ID(M_ROOT))
        {
            const struct mainmenu *mainm = &mainmenu[0];
            synclist_set(mainm->menuid, main_last_sel, mainm->items, 1);
            rb->gui_synclist_draw(lists);
        }
        else
            *exit = true;
    }
default_handler:
    if (rb->default_event_handler_ex(*action, cleanup, NULL) == SYS_USB_CONNECTED)
    {
        *exit = true;
        return PLUGIN_USB_CONNECTED;
    }
    return PLUGIN_OK;
}

static void synclist_set(char* menu_id, int selected_item, int items, int sel_size)
{
    if (items <= 0)
        return;
    if (selected_item < 0)
        selected_item = 0;

    list_voice_cb(0, menu_id);
    rb->gui_synclist_init(&lists,list_get_name_cb,
                          menu_id, false, sel_size, NULL);
    if (menu_id == MENU_ID(M_TESTPUT))
    {
        testput_cols = printcell_set_columns(&lists, TESTPUT_HEADER, Icon_Rockbox);
    }
    else
    {
        rb->gui_synclist_set_title(&lists, NULL,-1);
    }
    rb->gui_synclist_set_icon_callback(&lists,NULL);
    rb->gui_synclist_set_voice_callback(&lists, list_voice_cb);
    rb->gui_synclist_set_nb_items(&lists,items);
    rb->gui_synclist_select_item(&lists, selected_item);

}

enum plugin_status plugin_start(const void* parameter)
{
    int ret = PLUGIN_OK;
    int selected_item = -1;
    int action;
    bool redraw = true;
    bool exit = false;
    if (parameter)
    {
        //
    }
    mainmenu[M_BUTTONS].items = available_button_count;
    /* add header and back item to each submenu */
    for (int i = 1; i < M_LAST_ITEM; i++)
        mainmenu[i].items += 2;
    mainmenu[M_TESTPUT].items -= 1;
    if (!exit)
    {
        const struct mainmenu *mainm = &mainmenu[0];
        synclist_set(mainm->menuid, main_last_sel, mainm->items, 1);
        rb->gui_synclist_draw(&lists);

        while (!exit)
        {
            action = rb->get_action(CONTEXT_LIST, HZ / 10);
            if (m_test.count > 0)
                action = ACTION_REDRAW;

            if (action == ACTION_NONE)
            {
                if (redraw)
                {
                    action = ACTION_REDRAW;
                    redraw = false;
                }
            }
            else
                redraw = true;
            ret = menu_action_cb(&action, selected_item, &exit, &lists);
            if (rb->gui_synclist_do_button(&lists, &action))
                continue;
            selected_item = rb->gui_synclist_get_sel_pos(&lists);
        }
    }

    return ret;
}
