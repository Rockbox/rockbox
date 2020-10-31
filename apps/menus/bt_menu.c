/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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

#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "bt_menu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "list.h"
#include "menu.h"
#include "action.h"
#include "settings.h"
#include "screens.h"
#include "icons.h"
#include "font.h"
#include "rbunicode.h"
#include "lang.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#include "tree.h"
#include "screen_access.h"
#include "keyboard.h"
#include "gui/scrollbar.h"
#include "menu_common.h"
#include "viewport.h"
#include "exported_menus.h"

#include "bluetooth-target.h"

/* ---------------------------------------------------------------------------
 * |||||||||||||||||||||||||||||BLUETOOTH MENU||||||||||||||||||||||||||||||||
 * --------------------------------------------------------------------------- */
#define KEYBD_PIN_LAYOUT "1234567890"
#define KEYBD_ADDR_LAYOUT "12345678:\n9ABCDEF0:"

enum bt_menus {BT_MENU_SCAN = 0, BT_MENU_SCANEXP, BT_MENU_TRUSTED, BT_MENU_PROFILE};

static struct bluetooth_device bt_device_buf[BT_DEVICE_MAX_DEVICES];

static struct bt_menu_params
{
    int last_device;
    int total_devices;
    enum bt_menus menu;
    enum bt_menus parent;
    bool expanded;
} bt_menu_params;



struct bt_menu_items
{
    unsigned char *desc; /* string or ID */
    int icon_id; /* gui/icon.h */
};

int default_cb (void *param)
{
    (void)param;
    return 0;
}

#define BT_ITEM(name, icon) {.desc=ID2P(name),.icon_id=icon}

/*Scan Expanded Menu Items */
enum bt_scanexp_menu{BT_SXMENU_CONNECT = 0, BT_SXMENU_PROFILE, BT_SXMENU_REMEMBER};

const struct bt_menu_items bt_menu_scan_exp[] =
{
    [BT_SXMENU_CONNECT]    = BT_ITEM(VOICE_BLANK, Icon_NOICON), /* LANG_BLUETOOTH_CONNECT/LANG_BLUETOOTH_DISCONNECT */
    [BT_SXMENU_PROFILE]    = BT_ITEM(LANG_BLUETOOTH_PROFILE, Icon_NOICON),
    [BT_SXMENU_REMEMBER]     = BT_ITEM(VOICE_BLANK, Icon_NOICON),
};

#define BT_MENU_SCAN_EXP_ITEMS sizeof(bt_menu_scan_exp) / sizeof(*bt_menu_scan_exp)


/* Profile menu items */
enum bt_profile_menu{BT_PMENU_NAME = 0, BT_PMENU_NAME_FMT,
                     BT_PMENU_ADDRESS, BT_PMENU_ADDRESS_FMT,
                     BT_PMENU_PIN, BT_PMENU_PIN_FMT,
                     BT_PMENU_AUTO, BT_PMENU_AUTO_FMT,
                     BT_PMENU_BLANK, BT_PMENU_BACK};
const struct bt_menu_items bt_menu_profile[] =
{
    [BT_PMENU_NAME] = BT_ITEM(LANG_BLUETOOTH_NAME, Icon_Menu_setting),
    [BT_PMENU_NAME_FMT] = BT_ITEM(VOICE_BLANK, Icon_NOICON),
    [BT_PMENU_ADDRESS] = BT_ITEM(LANG_BLUETOOTH_ADDRESS, Icon_Menu_setting),
    [BT_PMENU_ADDRESS_FMT] = BT_ITEM(VOICE_BLANK, Icon_NOICON),
    [BT_PMENU_PIN] = BT_ITEM(LANG_BLUETOOTH_PIN, Icon_Menu_setting),
    [BT_PMENU_PIN_FMT] = BT_ITEM(VOICE_BLANK, Icon_NOICON),
    [BT_PMENU_AUTO] = BT_ITEM(LANG_BLUETOOTH_AUTO_CONNECT, Icon_Menu_setting),
    [BT_PMENU_AUTO_FMT] = BT_ITEM(VOICE_BLANK, Icon_NOICON),
    [BT_PMENU_BLANK] = BT_ITEM(VOICE_BLANK, Icon_NOICON),
    [BT_PMENU_BACK] = BT_ITEM(LANG_BACK, Icon_NOICON),
};

#define BT_MENU_PROFILE_ITEMS sizeof(bt_menu_profile) / sizeof(*bt_menu_profile)

/*
 * Utility functions
 */

const char* bt_addr_format(char* buffer, size_t buffer_size, uint64_t addr)
{
    (void)addr;
    snprintf(buffer, buffer_size, "??:??:??:??:??:??");
    return buffer;
}

/* ->plugins/lib/kbd_helper.h NEEDS MOVED TO CORE */
/*  USAGE:
    unsigned short kbd[64];
    unsigned short *kbd_p = kbd;
    if (!kbd_create_layout("ABCD1234\n", kbd, sizeof(kbd)))
        kbd_p = NULL;+

    rb->kbd_input(buf,sizeof(buf), kbd_p);
*/

/* create a custom keyboard layout for kbd_input
 * success returns size of buffer used
 * failure returns 0
*/
int kbd_create_layout(char *layout, unsigned short *buf, int bufsz)
{
    unsigned short *pbuf;
    const unsigned char *p = layout;
    int len = 0;
    pbuf = buf;
    while (*p && (pbuf - buf + (ptrdiff_t) sizeof(unsigned short)) < bufsz)
    {
        p = utf8decode(p, &pbuf[len+1]);
        if (pbuf[len+1] == '\n')
        {
            *pbuf = len;
            pbuf += len+1;
            len = 0;
        }
        else
            len++;
    }

    if (len+1 < bufsz)
    {
        *pbuf = len;
        pbuf[len+1] = 0xFEFF;   /* mark end of characters */
        return len + 1;
    }
    return 0;
}

/*
 * Settings functions
 */
static void bt_apply(void)
{
    return;
}

static int bt_setting_callback(int action,
                               const struct menu_item_ex *this_item,
                               struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            action = ACTION_ENTER_MENUITEM;
            break;
        case ACTION_EXIT_MENUITEM:
            bt_apply();
            action = ACTION_EXIT_MENUITEM;
            break;
    }

    return action;
}




#if 0
static int32_t get_dec_talkid(int value, int unit)
{
    return TALK_ID_DECIMAL(value, 1, unit);
}
#endif

static int bt_device_find_connected(void)
{
    /* returns -1 if no connected device is found */
    /* scans */
    int ret = -1;
    int i;
    struct bluetooth_device *device;
    int items = BT_DEVICE_MAX_DEVICES;
    bluetooth_scan(bt_device_buf, &items);
    bt_menu_params.last_device = 0;
    bt_menu_params.total_devices = items;

    for (i = 0; i < items; i++)
    {
        device = &bt_device_buf[i];
        if ((device->flags & BT_DEVICE_CONNECTED) == BT_DEVICE_CONNECTED)
        {
            ret = i;
            break;
        }
    }
    return ret;
}

static int bt_device_connect(void)
{
    int i;
    struct bluetooth_device *device;
    bool need_connect = false;
    for (i = 0; i < bt_menu_params.total_devices; i++)
    {
        /* Disconnect all devices */
        device = &bt_device_buf[i];
        if (i == bt_menu_params.last_device &&
           (device->flags & BT_DEVICE_CONNECTED) != BT_DEVICE_CONNECTED)
            need_connect = true;

        device->flags &= ~BT_DEVICE_CONNECTED;
    }

    device = &bt_device_buf[bt_menu_params.last_device];

    if (need_connect)
    {
        /* Connect Selected device */
        splash(100, "CONNECT");
        device->flags |= BT_DEVICE_CONNECTED;
        return 1;
    }
    return 0;
}

static void bt_menu_change(struct gui_synclist *lists, char *title, int sel_size,
                            int selected_item, enum bt_menus menu, int nb_items)
{
    if (lists->data) /* NOTE lists is not the same as struct simplelist_info */
    {
        *((int*)lists->data) = menu;
        lists->selected_size = sel_size;
        lists->selected_item = selected_item;
        lists->nb_items = nb_items;
        if (title && lists->title)
            strncpy(lists->title, title, MAX_PATH);
        gui_synclist_draw(lists);
    }
}

static char *bt_menu_scan_name(int selected_item, void *data, char *buffer, size_t len)
{
    (void)data;
    if (selected_item >= 0 && selected_item < bt_menu_params.total_devices)
    {
        struct bluetooth_device *device = &bt_device_buf[selected_item];
        snprintf(buffer, len, " %s [%s]", device->name, device->address);
        return buffer;
    }
    return "?";
}

static int bt_menu_scan_action(int item, int action, struct gui_synclist *lists)
{
    if (action == ACTION_STD_OK)
    {
        bt_menu_params.last_device = item;
        bt_menu_change(lists, bt_device_buf[item].name, 1, item, BT_MENU_SCANEXP,
                       bt_menu_params.total_devices + BT_MENU_SCAN_EXP_ITEMS);
        return ACTION_NONE;
    }
    return action;
}

static char *bt_menu_scan_expanded_name(int selected_item, void *data, char *buffer, size_t len)
{
    (void)data;
    int item = selected_item;
    struct bluetooth_device *device;
    char *lang = NULL;


    if ((item > bt_menu_params.last_device) &&
            (item <= (signed int)(bt_menu_params.last_device + BT_MENU_SCAN_EXP_ITEMS)))
    {
        device = &bt_device_buf[bt_menu_params.last_device];
        item = selected_item - bt_menu_params.last_device - 1;

        if (item == BT_SXMENU_CONNECT)
        {
            if ((device->flags & BT_DEVICE_CONNECTED) == BT_DEVICE_CONNECTED)
                lang = str(LANG_BLUETOOTH_DISCONNECT);
            else
                lang = str(LANG_BLUETOOTH_CONNECT);

            snprintf(buffer, len, "\t%s", lang);
        }
        else if (item == BT_SXMENU_REMEMBER)
        {
            if ((device->flags & BT_DEVICE_TRUSTED) == BT_DEVICE_TRUSTED)
                lang = str(LANG_BLUETOOTH_FORGET);
            else
                lang = str(LANG_BLUETOOTH_REMEMBER);

            snprintf(buffer, len, "\t%s", lang);
        }
        else if (item < (signed int) (BT_MENU_SCAN_EXP_ITEMS))
            snprintf(buffer, len, "\t%s", str(P2ID(bt_menu_scan_exp[item].desc)));

        return buffer;
    }
    else if (item > bt_menu_params.last_device)
        item = selected_item - BT_MENU_SCAN_EXP_ITEMS;

    if (bt_menu_params.parent == BT_MENU_SCAN)
    {
        return bt_menu_scan_name(item, data, buffer, len);
    }
    return "?";
}

static int bt_menu_scan_expanded_action(int item, int action, struct gui_synclist *lists)
{
    if (action == ACTION_STD_OK && item != bt_menu_params.last_device)
    {
        if ((item > bt_menu_params.last_device) &&
                (item <= (signed int)(bt_menu_params.last_device + BT_MENU_SCAN_EXP_ITEMS)))
        {
            item -= bt_menu_params.last_device + 1;
            if (item == BT_SXMENU_CONNECT)
            {
                bt_device_connect();
            }
            else if (item == BT_SXMENU_PROFILE)
            {
                bt_menu_change(lists, str(LANG_BLUETOOTH_PROFILE),
                       2, 0, BT_MENU_PROFILE, BT_MENU_PROFILE_ITEMS);
                return ACTION_NONE;

            }
            else if (item == BT_SXMENU_REMEMBER)
            {

            }

            return ACTION_REDRAW;
        }
        else if (item > bt_menu_params.last_device)
        {
            item = item - BT_MENU_SCAN_EXP_ITEMS;
        }

        bt_menu_params.last_device = item;
        bt_menu_change(lists, bt_device_buf[item].name, 1, item, BT_MENU_SCANEXP,
                       bt_menu_params.total_devices + BT_MENU_SCAN_EXP_ITEMS);
        return ACTION_NONE;
    }
    else if (action == ACTION_STD_CANCEL ||
            (action == ACTION_STD_OK && item == bt_menu_params.last_device))
    {
        if (bt_menu_params.parent == BT_MENU_SCAN)
        {
            bt_menu_change(lists, str(LANG_BLUETOOTH), 1,
                       bt_menu_params.last_device,
                       BT_MENU_SCAN, bt_menu_params.total_devices);
        }
        return ACTION_NONE;
    }
    return action;
}

static char *bt_menu_trusted_name(int selected_item, void *data, char *buffer, size_t len)
{
    (void)selected_item;
    (void)data;
    (void)buffer;
    (void)len;

    return "?";
}

static int bt_menu_trusted_action(int item, int action, struct gui_synclist *lists)
{
    (void)item;
    (void)lists;
    return action;
}

static char *bt_menu_profile_name(int selected_item, void *data, char *buffer, size_t len)
{
    (void)data;
    int item = selected_item;

    struct bluetooth_device *device = &bt_device_buf[bt_menu_params.last_device];

    switch (item)
    {

        case BT_PMENU_NAME_FMT:
            snprintf(buffer, len, " %s", device->name);
            return buffer;
        case BT_PMENU_ADDRESS_FMT:
            snprintf(buffer, len, " %s", device->address);
            return buffer;
        case BT_PMENU_PIN_FMT:
            snprintf(buffer, len, " %s", device->pin);
            return buffer;
        case BT_PMENU_AUTO_FMT:
            snprintf(buffer, len, " %s",
            str((device->flags & BT_DEVICE_AUTO_CONNECT) ? LANG_SET_BOOL_YES: LANG_SET_BOOL_NO));
            return buffer;
        case BT_PMENU_BLANK:
            return "";
    }

    return bt_menu_profile[item].desc;
}

static int bt_menu_profile_action(int item, int action, struct gui_synclist *lists)
{
    static unsigned short kbd[64];
    unsigned short *kbd_p = kbd;

    struct bluetooth_device *device = &bt_device_buf[bt_menu_params.last_device];

    (void)lists;
    if (action == ACTION_STD_OK && item != BT_PMENU_BLANK)
    {
        if (item == BT_PMENU_NAME)
        {
        }
        else if (item == BT_PMENU_ADDRESS)
        {
            if (!kbd_create_layout(KEYBD_ADDR_LAYOUT, kbd, sizeof(kbd)))
                kbd_p = NULL;
            if (kbd_input(device->address, BT_ADDR_BYTES, kbd_p))
                return ACTION_NONE;
        }
        else if (item == BT_PMENU_PIN)
        {
            if (!kbd_create_layout(KEYBD_PIN_LAYOUT, kbd, sizeof(kbd)))
                kbd_p = NULL;
            if (kbd_input(device->pin, BT_PIN_BYTES, kbd_p))
                return ACTION_NONE;
        }
        else if (item == BT_PMENU_AUTO)
        {
        }
        else /* error */
            return action;

        return ACTION_NONE;
    }
    else if (action == ACTION_STD_CANCEL ||
            (action == ACTION_STD_OK && item == BT_PMENU_BLANK))
    {
        bt_menu_change(lists, bt_device_buf[item].name, 1,
                       bt_menu_params.last_device, BT_MENU_SCANEXP,
                       bt_menu_params.total_devices + BT_MENU_SCAN_EXP_ITEMS);
        return ACTION_NONE;
    }
    return action;
}


static int bt_menu_action_callback(int action, struct gui_synclist *lists)
{
    /* handles bt menu logic */

    int item = lists->selected_item;
    int menu_param = BT_MENU_SCAN;

    if (lists->data)
        menu_param = *((int*)lists->data);

    if (menu_param == BT_MENU_SCAN)
    {
        return bt_menu_scan_action(item, action, lists);
    }
    else if (menu_param == BT_MENU_SCANEXP)
    {
        return bt_menu_scan_expanded_action(item, action, lists);
    }
    else if (menu_param == BT_MENU_TRUSTED)
    {
        return bt_menu_trusted_action(item, action, lists);
    }
    else if (menu_param == BT_MENU_PROFILE)
    {
        return bt_menu_profile_action(item, action, lists);
    }

    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

static char *bt_menu_item_get_name(int selected_item, void *data, char *buffer, size_t len)
{
    int menu_param = BT_MENU_SCAN;
    if (data)
        menu_param = *((int*)data);

    if (menu_param == BT_MENU_SCAN)
    {
        return bt_menu_scan_name(selected_item, data, buffer, len);
    }
    else if (menu_param == BT_MENU_SCANEXP)
    {
        return bt_menu_scan_expanded_name(selected_item, data, buffer, len);
    }
    else if (menu_param == BT_MENU_TRUSTED)
    {
        return bt_menu_trusted_name(selected_item, data, buffer, len);
    }
    else if (menu_param == BT_MENU_PROFILE)
    {
        return bt_menu_profile_name(selected_item, data, buffer, len);
    }

    return "?";
}

static int bt_menu_speak_item(int selected_item, void *data)
{
    (void)data;
    int item = selected_item;
    int lang = -1;

    //selection_to_banditem(selected_item, *(intptr_t*)data, &band, &item);

    switch (item)
    {
    case 0:
        talk_number(0, true);
        return -1;
        break;
    case 1:
        talk_number(1, true);
        return -1;
        break;
    case 2:
        talk_number(2, true);
        return -1;
        break;
    case 3:
        talk_number(3, true);
        return -1;
        break;
    case 4:
        talk_number(4, true);
        return -1;
        break;
    }
    talk_id(lang, true);
    return -1;
}

static enum themable_icons bt_menu_get_icon(int selected_item, void * data)
{
    int menu_param = BT_MENU_SCAN;
    if (data)
        menu_param = *((int*)data);

    if (menu_param == BT_MENU_SCAN)
    {

    }
    else if (menu_param == BT_MENU_SCANEXP)
    {

    }
    else if (menu_param == BT_MENU_TRUSTED)
    {

    }
    else if (menu_param == BT_MENU_PROFILE)
    {
        return bt_menu_profile[selected_item].icon_id;
    }

    if (selected_item == 0)
        return Icon_Submenu;
    else
        return Icon_Menu_setting;
}

#if 0
enum bt_type {
    BT_AUDIO,
    BT_RECORDING,
    BT_DATA
};
#endif
/*
LANG_BLUETOOTH
LANG_BLUETOOTH_SCAN
LANG_BLUETOOTH_PAIR
LANG_BLUETOOTH_TRUSTED
LANG_BLUETOOTH_CONNECT
LANG_BLUETOOTH_DISCONNECT
LANG_BLUETOOTH_FORGET
LANG_BLUETOOTH_PROFILE
LANG_BLUETOOTH_ADDRESS
LANG_BLUETOOTH_NAME
LANG_BLUETOOTH_AUTO_CONNECT
LANG_BLUETOOTH_PIN
*/

int bt_menu(void * param)
{
    /* Initalize menu parameters */
    bt_menu_params.expanded = false;
    bt_menu_params.menu = BT_MENU_SCAN;
    bt_menu_params.parent = BT_MENU_SCAN;
    bt_menu_params.total_devices = 0;
    bt_menu_params.last_device = 0;

    int menu_param = BT_MENU_SCAN;
    if (param)
        menu_param = *((int*)param);

    char title[MAX_PATH];

    struct simplelist_info info;
    //struct settings_list setting;

    int connected_dev;
    int items = 1;

    strncpy(title, str(LANG_BLUETOOTH), sizeof(title));
    simplelist_info_init(&info, title, items, &menu_param);

    info.get_name = (list_get_name*)bt_menu_item_get_name;
    info.get_talk = bt_menu_speak_item;
    info.get_icon = bt_menu_get_icon;
    info.action_callback = bt_menu_action_callback;
    info.selection = -1;
    info.title_icon = Icon_Audio;
    info.selection_size = 1;
    info.scroll_all = false;


    switch (menu_param) /* Init for dynamic menus */
    {
        case BT_MENU_SCAN:
            connected_dev = bt_device_find_connected();
            /* expand menu under connected device */
            if (connected_dev >= 0)
            {
                bt_menu_params.menu = BT_MENU_SCANEXP;
                bt_menu_params.last_device = connected_dev;
                info.selection = connected_dev;
                info.count = bt_menu_params.total_devices + BT_MENU_SCAN_EXP_ITEMS;
                *((int*)info.callback_data) = bt_menu_params.menu;
            }
            else
            {
                bt_menu_params.menu = BT_MENU_SCAN;
                info.count = bt_menu_params.total_devices;
            }
            break;
        case BT_MENU_TRUSTED:
            bt_menu_params.menu = BT_MENU_TRUSTED;
            bt_menu_params.last_device = -1;
            info.selection = 0;
            info.count = 1;
            bt_menu_params.parent = BT_MENU_TRUSTED;
            break;
        default:
            splashf(100, "error %d", menu_param);
            break;
    }

    while (true)
    {
        simplelist_show_list(&info);
        if (info.selection < 0)
            break;

        bt_apply();
    }
    return 0;
}

#if 0
static int bt_save_trusted(void)
{
    bool result = false;

    return (result) ? 1 : 0;
}

/* Allows browsing of trusted list files */
int browse_callback(int action,
                        const struct menu_item_ex *this_item,
                        struct gui_synclist *this_list)
{
    (void)this_item;
    (void)this_list;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM: /* on entering an item */

            break;
        case ACTION_EXIT_MENUITEM: /* on exit */

            break;
    }
    return action;
}
static const struct browse_folder_info btd = {ROCKBOX_DIR, SHOW_CFG};
#endif

/*
MENUITEM_SETTING(name,var,callback)                  \
*/

MENUITEM_SETTING(bt_enable, &global_settings.bt_enabled, bt_setting_callback);

/*
MENUITEM_FUNCTION(name, flags, str, func, param, callback, icon)
*/
static int param_scan = BT_MENU_SCAN;
static int param_trusted = BT_MENU_TRUSTED;


MENUITEM_FUNCTION(bt_scan, MENU_FUNC_USEPARAM, ID2P(LANG_BLUETOOTH_SCAN),
                    bt_menu, &param_scan, NULL,
                    Icon_Radio_screen);

MENUITEM_FUNCTION(bt_trusted, MENU_FUNC_USEPARAM, ID2P(LANG_BLUETOOTH_TRUSTED),
                    bt_menu, &param_trusted, NULL,
                    Icon_Radio_screen);

/* Main Menu */
MAKE_MENU(bluetooth_audio_menu, ID2P(LANG_BLUETOOTH), NULL, Icon_Audio,
        &bt_enable, &bt_scan, &bt_trusted);

