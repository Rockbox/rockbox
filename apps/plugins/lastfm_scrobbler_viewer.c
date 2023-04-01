/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _// __ \_/ ___\|  |/ /| __ \ / __ \  \/  /
 *   Jukebox    |    |   ( (__) )  \___|    ( | \_\ ( (__) )    (
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 William Wilgus
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
#include "lang_enum.h"

#include "lib/printcell_helper.h"

#include "lib/configfile.h"
#define CFG_FILE "/lastfm_scrobbler_viewer.cfg"
#define TMP_FILE ""PLUGIN_DATA_DIR "/lscrobbler_viewer_%d.tmp"
#define CFG_VER  1

#ifdef ROCKBOX_HAS_LOGF
#define logf rb->logf
#else
#define logf(...) do { } while(0)
#endif

/*#ARTIST #ALBUM #TITLE #TRACKNUM #LENGTH #RATING #TIMESTAMP #MUSICBRAINZ_TRACKID*/

#define SCROBBLE_HDR_FMT "$*%d$%s$*%d$%s$*%d$%s$Track#$Length$Rating$TimeStamp$TrackId"
//#define SCROBBLE_HDR "$*128$Artist$*128$Album$*128$Title$Track#$Length$Rating$TimeStamp$TrackId"

#define SCROBBLER_MIN_COLUMNS (6) /* a valid scrobbler file should have at least this many columns */

#define GOTO_ACTION_DEFAULT_HANDLER (PLUGIN_OK + 1)
#define CANCEL_CONTEXT_MENU (PLUGIN_OK + 2)
#define QUIT_CONTEXT_MENU (PLUGIN_OK + 3)
/* global lists, for everything */
static struct gui_synclist lists;

/* printcell data for the current file */
struct printcell_data_t {
    int view_columns;
    int view_lastcol;

    int items_buffered;
    int items_total;
    int fd_cur;
    char *filename;

    char *buf;
    size_t buf_size;
    off_t buf_used;
    char header[PRINTCELL_MAXLINELEN];

};

enum e_find_type {
    FIND_ALL = 0,
    FIND_EXCLUDE,
    FIND_EXCLUDE_CASE,
    FIND_EXCLUDE_ANY,
    FIND_INCLUDE,
    FIND_INCLUDE_CASE,
    FIND_INCLUDE_ANY,
    FIND_CUSTOM,
};

static void synclist_set(int selected_item, int items, int sel_size, struct printcell_data_t *pc_data);
static void pc_data_set_header(struct printcell_data_t *pc_data);

static void browse_file(char *buf, size_t bufsz)
{
    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY,
        .title = "Select a scrobbler log file",
        .icon = Icon_Playlist,
        .buf = buf,
        .bufsize = bufsz,
        .root = "/",
    };

    if (rb->rockbox_browse(&browse) != GO_TO_PREVIOUS)
    {
        buf[0] = '\0';
    }
}

static struct plugin_config
{
    bool separator;
    bool talk;
    int  col_width;
    uint32_t hidecol_flags;
} gConfig;

static struct configdata config[] =
{
   {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.separator }, "Cell Separator", NULL},
   {TYPE_BOOL, 0, 1,  { .bool_p = &gConfig.talk }, "Voice", NULL},
   {TYPE_INT, 32, LCD_WIDTH, { .int_p = &gConfig.col_width }, "Cell Width",  NULL},
   {TYPE_INT, INT_MIN, INT_MAX, { .int_p = &gConfig.hidecol_flags }, "Hidden Columns",  NULL},
};
const int gCfg_sz = sizeof(config)/sizeof(*config);
/****************** config functions *****************/
static void config_set_defaults(void)
{
    gConfig.col_width = MIN(LCD_WIDTH, 128);
    gConfig.hidecol_flags = 0;
    gConfig.separator = true;
    gConfig.talk = rb->global_settings->talk_menu;
}

static void config_save(void)
{
    configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
}

static int config_settings_menu(struct printcell_data_t *pc_data)
{
    int selection = 0;

    struct viewport parentvp[NB_SCREENS];
    FOR_NB_SCREENS(l)
    {
        rb->viewport_set_defaults(&parentvp[l], l);
        rb->viewport_set_fullscreen(&parentvp[l], l);
    }

    MENUITEM_STRINGLIST(settings_menu, ID2P(LANG_SETTINGS), NULL,
                        ID2P(LANG_LIST_SEPARATOR),
                        "Cell Width",
                        ID2P(LANG_VOICE),
                        ID2P(VOICE_BLANK),
                        ID2P(VOICE_BLANK),
                        ID2P(LANG_CANCEL_0),
                        ID2P(LANG_SAVE_EXIT));

    do {
        selection=rb->do_menu(&settings_menu,&selection, parentvp, true);
        switch(selection) {

            case 0:
                rb->set_bool(rb->str(LANG_LIST_SEPARATOR), &gConfig.separator);
                break;
            case 1:
                rb->set_int("Cell Width", "", UNIT_INT,
                            &gConfig.col_width, NULL, 1, 32, LCD_WIDTH, NULL );
                break;
            case 2:
                rb->set_bool(rb->str(LANG_VOICE), &gConfig.talk);
                break;
            case 3:
                continue;
            case 4: /*sep*/
                continue;
            case 5:
                return -1;
                break;
            case 6:
            {
                pc_data_set_header(pc_data);
                synclist_set(0, pc_data->items_total, 1, pc_data);
                int res = configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);
                if (res >= 0)
                {
                    logf("Cfg saved %s %d bytes", CFG_FILE, gCfg_sz);
                    return PLUGIN_OK;
                }
                logf("Cfg FAILED (%d) %s", res, CFG_FILE);
                return PLUGIN_ERROR;
            }
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                return PLUGIN_OK;
        }
    } while ( selection >= 0 );
    return 0;
}

/* open file pass back file descriptor and return file size */
static size_t file_open(const char *filename, int *fd)
{
    size_t fsize = 0;

    if (filename && fd)
    {
        *fd = rb->open(filename, O_RDONLY);
        if (*fd >= 0)
        {
            fsize = rb->filesize(*fd);
        }
    }
    return fsize;
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    const int slush_pos = 32; /* entries around the current entry to keep buffered */
    /* keep track of the last position in the buffer */
    static int buf_line_num = 0;
    static off_t buf_last_pos = 0;
    /* keep track of the last position in the file */
    static int file_line_num = 0;
    static off_t file_last_seek = 0;

    if (selected_item < 0)
    {
        logf("[%s] Reset positions", __func__);
        buf_line_num = 0;
        buf_last_pos = 0;
        file_line_num = 0;
        file_last_seek = 0;
        return "";
    }

    int line_num;
    struct printcell_data_t *pc_data = (struct printcell_data_t*) data;
    bool found = false;

    if (pc_data->buf && selected_item < pc_data->items_buffered)
    {
        int buf_pos;

        if (buf_line_num > selected_item || buf_last_pos >= pc_data->buf_used
         || buf_line_num < 0)
        {
            logf("[%s] Set pos buffer 0", __func__);
            buf_line_num = 0;
            buf_last_pos = 0;
        }
        buf_pos = buf_last_pos;
        line_num = buf_line_num;

        while (buf_pos < pc_data->buf_used
            && line_num < pc_data->items_buffered)
        {
            size_t len = rb->strlen(&pc_data->buf[buf_pos]);
            if(line_num == selected_item)
            {
                rb->strlcpy(buf, &pc_data->buf[buf_pos], MIN(buf_len, len));
                logf("(%d) in buffer: %s", line_num, buf);
                found = true;
                break;
            }
            else
            {
                buf_pos += len + 1; /* need to go past the NULL terminator */
                line_num++;

                if (buf_line_num + slush_pos < selected_item)
                {
                    logf("[%s] Set pos buffer %d", __func__, line_num);
                    buf_line_num = line_num;
                    buf_last_pos = buf_pos;
                }
            }
        }
    }

    /* didn't find the item try the file */
    if(!found && pc_data->fd_cur >= 0)
    {
        int fd = pc_data->fd_cur;

        if (file_line_num < 0 || file_line_num > selected_item)
        {
            logf("[%s] Set seek file 0", __func__);
            file_line_num = 0;
            file_last_seek = 0;
        }

        rb->lseek(fd, file_last_seek, SEEK_SET);
        line_num = file_line_num;

        while ((rb->read_line(fd, buf, buf_len)) > 0)
        {
            if(buf[0] == '#')
                continue;
            if(line_num == selected_item)
            {
                logf("(%d) in file: %s", line_num, buf);
                found = true;
                break;
            }
            else
            {
                line_num++;

                if (file_line_num + slush_pos < selected_item)
                {
                    logf("[%s] Set seek file %d", __func__, line_num);
                    file_line_num = line_num;
                    file_last_seek = rb->lseek(fd, 0, SEEK_CUR);
                }
            }
        }
    }

    if(!found)
    {
        logf("(%d) Not Found!", selected_item);
        buf_line_num = -1;
        file_line_num = -1;
        buf[0] = '\0';
    }
    return buf;
}

static int list_voice_cb(int list_index, void* data)
{
    (void) list_index;
    struct printcell_data_t *pc_data = (struct printcell_data_t*) data;
    if (!gConfig.talk)
        return -1;

    int selcol = printcell_get_column_selected();
    char buf[MAX_PATH];
    char* name;
    long id;

    if (pc_data->view_lastcol != selcol)
    {
        name = printcell_get_title_text(selcol, buf, sizeof(buf));

        id = P2ID((const unsigned char *)name);

        if(id>=0)
            rb->talk_id(id, true);
        else if (selcol >= 0)
        {
            switch (selcol)
            {
                case 0:
                    rb->talk_id(LANG_ID3_ARTIST, true);
                    break;
                case 1:
                    rb->talk_id(LANG_ID3_ALBUM, true);
                    break;
                case 2:
                    rb->talk_id(LANG_ID3_TITLE, true);
                    break;
                case 3:
                    rb->talk_id(LANG_ID3_TRACKNUM, true);
                    break;
                case 4:
                    rb->talk_id(LANG_ID3_LENGTH, true);
                    break;

                default:
                    rb->talk_spell(name, true);
                    break;
            }
        }
        else
            rb->talk_id(LANG_ALL, true);

        rb->talk_id(VOICE_PAUSE, true);
    }

    name = printcell_get_column_text(selcol, buf, sizeof(buf));

    id = P2ID((const unsigned char *)name);

    if(id>=0)
        rb->talk_id(id, true);
    else if (selcol >= 0)
        rb->talk_spell(name, true);

    return 0;
}

static enum themable_icons list_icon_cb(int selected_item, void *data)
{
    struct printcell_data_t *pc_data = (struct printcell_data_t*) data;
    if (lists.selected_item == selected_item)
    {
        if (pc_data->view_lastcol < 0)
            return Icon_Config;
        return Icon_Audio;
    }
    return Icon_NOICON;
}

/* load file entries into pc_data buffer, file should already be opened
 * and will be closed if the whole file was buffered */
static int file_load_entries(struct printcell_data_t *pc_data)
{
    int items = 0;
    int count = 0;
    int buffered = 0;
    unsigned int pos = 0;
    bool comment = false;
    char ch;
    int fd = pc_data->fd_cur;
    if (fd < 0)
        return 0;

    rb->lseek(fd, 0, SEEK_SET);

    while (rb->read(fd, &ch, 1) > 0)
    {
        if (count++ == 0 && ch == '#') /* skip comments */
            comment = true;
        else if (!comment && ch != '\r' && pc_data->buf_size > pos)
            pc_data->buf[pos++] = ch;

        if (items == 0 && pos > PRINTCELL_MAXLINELEN * 2)
            break;

        if (ch == '\n')
        {
            if (!comment)
            {
                pc_data->buf[pos] = '\0';
                if (pc_data->buf_size > pos)
                {
                    pos++;
                    buffered++;
                }
                items++;
            }
            comment = false;
            count = 0;
            rb->yield();
        }
    }

    logf("[%s] items: %d buffered: %d", __func__, items, buffered);

    pc_data->items_total = items;
    pc_data->items_buffered = buffered;
    pc_data->buf[pos] = '\0';
    pc_data->buf_used = pos;

    if (items == buffered) /* whole file fit into buffer; close file */
    {
        rb->close(pc_data->fd_cur);
        pc_data->fd_cur = -1;
    }

    list_get_name_cb(-1, NULL, NULL, 0); /* prime name cb */
    return items;
}

static int filter_items(struct printcell_data_t *pc_data,
                        enum e_find_type find_type, int col)
{
    /* saves filtered items to a temp file and loads it */
    int fd;
    bool reload = false;
    char buf[PRINTCELL_MAXLINELEN];
    char find_exclude_buf[PRINTCELL_MAXLINELEN];
    int selcol = printcell_get_column_selected();
    char *find_exclude = printcell_get_column_text(selcol, find_exclude_buf,
                                                      sizeof(find_exclude_buf));
    const char colsep = '\t';
    int find_len = strlen(find_exclude);

    if (find_type == FIND_CUSTOM || find_len == 0)
    {
        int option = 0;
        struct opt_items find_types[] = {
            {"Exclude", -1}, {"Exclude Case Sensitive", -1},
            {"Exclude Any", -1}, {"Include", -1},
            {"Include Case Sensitive", -1}, {"Include Any", -1}
        };
        if (rb->set_option("Find Type", &option, INT,
                           find_types, 6, NULL))
        {
            return 0;
        }
        switch (option)
        {
            case 0:
                find_type = FIND_EXCLUDE;
                break;
            case 1:
                find_type = FIND_EXCLUDE_CASE;
                break;
            case 2:
                find_type = FIND_EXCLUDE_ANY;
                break;
            case 3:
                find_type = FIND_INCLUDE;
                break;
            case 4:
                find_type = FIND_INCLUDE_CASE;
                break;
            case 5:
                find_type = FIND_INCLUDE_ANY;
                break;
            default:
                find_type = FIND_ALL;
                break;
        }

        /* copy data to beginning of buf */
        rb->memmove(find_exclude_buf, find_exclude, find_len);
        find_exclude_buf[find_len] = '\0';

        if (rb->kbd_input(find_exclude_buf, sizeof(find_exclude_buf), NULL) < 0)
            return -1;
        find_exclude = find_exclude_buf;
        find_len = strlen(find_exclude);
    }

    char tmp_filename[MAX_PATH];
    static int tmp_num = 0;
    rb->snprintf(tmp_filename, sizeof(tmp_filename), TMP_FILE, tmp_num);
    tmp_num++;

    fd = rb->open(tmp_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd >= 0)
    {
        rb->splash_progress_set_delay(HZ * 5);
        for (int i = 0; i < pc_data->items_total; i++)
        {
            rb->splash_progress(i, pc_data->items_total, "Filtering...");
            const char *  data = list_get_name_cb(i, pc_data, buf, sizeof(buf));

            if (find_type != FIND_ALL)
            {
                int index = col;

                if (index < 0)
                    index = 0;
                if (find_len > 0)
                {
                    char *bcol = buf;
                    while (*bcol != '\0' && index > 0)
                    {
                        if (*bcol == colsep)
                            index--;
                        bcol++;
                    }
                    if (index > 0)
                        continue;

                    if (find_type == FIND_EXCLUDE)
                    {
                        if (rb->strncasecmp(bcol, find_exclude, find_len) == 0)
                        {
                            logf("[%s] exclude [%s]", find_exclude, bcol);
                            continue;
                        }
                    }
                    else if (find_type == FIND_INCLUDE)
                    {
                        if (rb->strncasecmp(bcol, find_exclude, find_len) != 0)
                        {
                            logf("%s include %s", find_exclude, bcol);
                            continue;
                        }
                    }
                    else if (find_type == FIND_EXCLUDE_CASE)
                    {
                        if (rb->strncmp(bcol, find_exclude, find_len) == 0)
                        {
                            logf("[%s] exclude case [%s]", find_exclude, bcol);
                            continue;
                        }
                    }
                    else if (find_type == FIND_INCLUDE_CASE)
                    {
                        if (rb->strncmp(bcol, find_exclude, find_len) != 0)
                        {
                            logf("%s include case %s", find_exclude, bcol);
                            continue;
                        }
                    }
                    else if (find_type == FIND_EXCLUDE_ANY)
                    {
                        bool found = false;
                        while (*bcol != '\0' && *bcol != colsep)
                        {
                            if (rb->strncasecmp(bcol, find_exclude, find_len) == 0)
                            {
                                logf("[%s] exclude any [%s]", find_exclude, bcol);
                                found = true;
                                break;
                            }
                            bcol++;
                        }
                        if (found)
                            continue;
                    }
                    else if (find_type == FIND_INCLUDE_ANY)
                    {
                        bool found = false;
                        while (*bcol != '\0' && *bcol != colsep)
                        {
                            if (rb->strncasecmp(bcol, find_exclude, find_len) == 0)
                            {
                                found = true;
                                logf("[%s] include any [%s]", find_exclude, bcol);
                                break;
                            }
                            bcol++;
                        }
                        if (!found)
                            continue;
                    }
                }
            }
            int len = strlen(data);
            if (len > 0)
            {
                logf("writing [%d bytes][%s]", len + 1, data);
                rb->write(fd, data, len);
                rb->write(fd, "\n", 1);
            }
        }
        reload = true;
    }

    if (reload)
    {
        if (pc_data->fd_cur >= 0)
            rb->close(pc_data->fd_cur);

        pc_data->fd_cur = fd;
        int items = file_load_entries(pc_data);
        if (items >= 0)
        {
            lists.selected_item = 0;
            rb->gui_synclist_set_nb_items(&lists, items);
        }
    }
    logf("Col text '%s'", find_exclude);
    logf("text '%s'", find_exclude_buf);

   return pc_data->items_total;
}

static int scrobbler_context_menu(struct printcell_data_t *pc_data)
{
    struct viewport parentvp[NB_SCREENS];
    FOR_NB_SCREENS(l)
    {
        rb->viewport_set_defaults(&parentvp[l], l);
        rb->viewport_set_fullscreen(&parentvp[l], l);
    }

    int col = printcell_get_column_selected();
    int selection = 0;
    int items = 0;
    uint32_t visible = printcell_get_column_visibility(-1);
    bool hide_col = PRINTCELL_COLUMN_IS_VISIBLE(visible, col);

    char namebuf[PRINTCELL_MAXLINELEN];
    enum e_find_type find_type = FIND_ALL;

    char *colname = pc_data->filename;
    if (col >= 0)
        colname = printcell_get_title_text(col, namebuf, sizeof(namebuf));

#define MENUITEM_STRINGLIST_CUSTOM(name, str, callback, ... )           \
    const char *name##_[] = {__VA_ARGS__};                                    \
    const struct menu_callback_with_desc name##__ =              \
            {callback,str, Icon_NOICON};                                \
    const struct menu_item_ex name =                                    \
        {MT_RETURN_ID|MENU_HAS_DESC|                                    \
         MENU_ITEM_COUNT(sizeof( name##_)/sizeof(*name##_)),            \
            { .strings = name##_},{.callback_and_desc = & name##__}};

    const char *menu_item[4];

    menu_item[0]= hide_col ? "Hide":"Show";
    menu_item[1]= "Exclude";
    menu_item[2]= "Include";
    menu_item[3]= "Custom Filter";

    if (col == -1)
    {
        menu_item[0]= hide_col ? "Hide All":"Show All";
        menu_item[1]= "Open";
        menu_item[2]= "Reload";
        menu_item[3]= ID2P(LANG_SETTINGS);
    }

    MENUITEM_STRINGLIST_CUSTOM(context_menu, colname, NULL,
                               menu_item[0],
                               menu_item[1],
                               menu_item[2],
                               menu_item[3],
                               ID2P(VOICE_BLANK),
                               ID2P(LANG_CANCEL_0),
                               ID2P(LANG_MENU_QUIT));

#undef MENUITEM_STRINGLIST_CUSTOM
    do {
        selection=rb->do_menu(&context_menu,&selection, parentvp, true);
        switch(selection) {

            case 0:
            {
                printcell_set_column_visible(col, !hide_col);
                if (hide_col)
                {
                    do
                    {
                        col = printcell_increment_column(1, true);
                    } while (col >= 0 && printcell_get_column_visibility(col) == 1);
                    pc_data->view_lastcol = col;
                }
                gConfig.hidecol_flags = printcell_get_column_visibility(-1);
                break;
            }
            case 1: /* Exclude / Open */
            {
                if (col == -1)/*Open*/
                {
                    char buf[MAX_PATH];
                    browse_file(buf, sizeof(buf));
                    if (rb->file_exists(buf))
                    {
                        rb->strlcpy(pc_data->filename, buf, MAX_PATH);
                        if (pc_data->fd_cur >= 0)
                            rb->close(pc_data->fd_cur);
                        if (file_open(pc_data->filename, &pc_data->fd_cur) > 0)
                            items = file_load_entries(pc_data);
                        if (items >= 0)
                            synclist_set(0, items, 1, pc_data);
                    }
                    else
                        rb->splash(HZ *2, "Error Opening");

                    return CANCEL_CONTEXT_MENU;
                }

                find_type = FIND_EXCLUDE;
            }
            /* fall-through */
            case 2: /* Include / Reload */
            {
                if (col == -1) /*Reload*/
                {
                    if (pc_data->fd_cur >= 0)
                        rb->close(pc_data->fd_cur);
                    if (file_open(pc_data->filename, &pc_data->fd_cur) > 0)
                        items = file_load_entries(pc_data);
                    if (items >= 0)
                        rb->gui_synclist_set_nb_items(&lists, items);
                    return CANCEL_CONTEXT_MENU;
                }

                if (find_type == FIND_ALL)
                    find_type = FIND_INCLUDE;
                /* fall-through */
            }
            case 3: /*Custom Filter / Settings */
            {
                if (col == -1)/*Settings*/
                    return config_settings_menu(pc_data);

                if (find_type == FIND_ALL)
                    find_type = FIND_CUSTOM;
                items = filter_items(pc_data, find_type, col);

                if (items >= 0)
                    rb->gui_synclist_set_nb_items(&lists, items);
                break;
            }
            case 4: /*sep*/
                continue;
            case 5:
                return CANCEL_CONTEXT_MENU;
                break;
            case 6: /* Quit */
                return QUIT_CONTEXT_MENU;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
            default:
                return PLUGIN_OK;
        }
    } while ( selection < 0 );
    return 0;
}

static void cleanup(void *parameter)
{
    struct printcell_data_t *pc_data = (struct printcell_data_t*) parameter;
    if (pc_data->fd_cur >= 0)
        rb->close(pc_data->fd_cur);
    pc_data->fd_cur = -1;
}

static void menu_action_printcell(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{
    (void) exit;
    struct printcell_data_t *pc_data = (struct printcell_data_t*) lists->data;
    if (*action == ACTION_STD_OK)
    {
        if (selected_item < lists->nb_items)
        {
            pc_data->view_lastcol = printcell_increment_column(1, true);
            *action = ACTION_NONE;
        }
    }
    else if (*action == ACTION_STD_CANCEL)
    {
        pc_data->view_lastcol = printcell_increment_column(-1, true);
        if (pc_data->view_lastcol != pc_data->view_columns - 1)
        {
            *action = ACTION_NONE;
        }
    }
    else if (*action == ACTION_STD_CONTEXT)
    {
        int ctxret = scrobbler_context_menu(pc_data);
        if (ctxret == QUIT_CONTEXT_MENU)
            *exit = true;
    }
}

int menu_action_cb(int *action, int selected_item, bool* exit, struct gui_synclist *lists)
{

    menu_action_printcell(action, selected_item, exit, lists);

    if (rb->default_event_handler_ex(*action, cleanup, lists->data) == SYS_USB_CONNECTED)
    {
        *exit = true;
        return PLUGIN_USB_CONNECTED;
    }
    return PLUGIN_OK;
}

static int count_max_columns(int items, char delimeter,
                             int expected_cols, struct printcell_data_t *pc_data)
{
    int max_cols = 0;
    int cols = 0;
    char buf[PRINTCELL_MAXLINELEN];
    for (int i = 0; i < items; i++)
    {
        const char *txt = list_get_name_cb(i, pc_data, buf, sizeof(buf));
        while (*txt != '\0')
        {
            if (*txt == delimeter)
            {
                cols++;
                if (cols == expected_cols)
                {
                    max_cols = cols;
                    break;
                }
            }
            txt++;
        }

        if(max_cols < expected_cols && i > 32)
            break;

        if (cols > max_cols)
            max_cols = cols;
        cols = 0;
    }
    return max_cols;
}

static void synclist_set(int selected_item, int items, int sel_size, struct printcell_data_t *pc_data)
{
    if (items <= 0)
        return;
    if (selected_item < 0)
        selected_item = 0;

    rb->gui_synclist_init(&lists,list_get_name_cb,
                          pc_data, false, sel_size, NULL);

    rb->gui_synclist_set_icon_callback(&lists, list_icon_cb);
    rb->gui_synclist_set_voice_callback(&lists, list_voice_cb);
    rb->gui_synclist_set_nb_items(&lists,items);
    rb->gui_synclist_select_item(&lists, selected_item);

    struct printcell_settings pcs = {.cell_separator = gConfig.separator,
                                     .title_delimeter = '$',
                                     .text_delimeter = '\t',
                                     .hidecol_flags = gConfig.hidecol_flags};

    pc_data->view_columns = printcell_set_columns(&lists, &pcs,
                                                  pc_data->header, Icon_Rockbox);
    printcell_enable(true);


    int max_cols = count_max_columns(items, pcs.text_delimeter,
                                     SCROBBLER_MIN_COLUMNS, pc_data);
    if (max_cols < SCROBBLER_MIN_COLUMNS) /* not a scrobbler file? */
    {
        rb->gui_synclist_set_voice_callback(&lists, NULL);
        pc_data->view_columns = printcell_set_columns(&lists, NULL,
                                                    "$*512$", Icon_Questionmark);
    }

    int curcol = printcell_get_column_selected();
    while (curcol >= 0)
        curcol = printcell_increment_column(-1, false);

    if (pc_data->view_lastcol >= pc_data->view_columns)
        pc_data->view_lastcol = -1;

    /* restore column position */
    while (pc_data->view_lastcol > -1 && curcol != pc_data->view_lastcol)
    {
        curcol = printcell_increment_column(1, true);
    }
    pc_data->view_lastcol = curcol;
    list_voice_cb(0, pc_data);
}

static void pc_data_set_header(struct printcell_data_t *pc_data)
{
    int col_w = gConfig.col_width;
    rb->snprintf(pc_data->header, PRINTCELL_MAXLINELEN,
                 SCROBBLE_HDR_FMT, col_w, rb->str(LANG_ID3_ARTIST),
                                   col_w, rb->str(LANG_ID3_ALBUM),
                                   col_w, rb->str(LANG_ID3_TITLE));
}

enum plugin_status plugin_start(const void* parameter)
{
    int ret = PLUGIN_OK;
    int selected_item = -1;
    int action;
    int items = 0;
#if CONFIG_RTC
    static char filename[MAX_PATH] = HOME_DIR "/.scrobbler.log";
#else /* !CONFIG_RTC */
    static char filename[MAX_PATH] = HOME_DIR "/.scrobbler-timeless.log";
#endif /* CONFIG_RTC */
    bool redraw = true;
    bool exit = false;

    static struct printcell_data_t printcell_data;

    if (parameter)
    {
        rb->strlcpy(filename, (const char*)parameter, MAX_PATH);
        filename[MAX_PATH - 1] = '\0';
    }

    if (!rb->file_exists(filename))
    {
        browse_file(filename, sizeof(filename));
        if (!rb->file_exists(filename))
        {
            rb->splash(HZ, "No Scrobbler file Goodbye.");
            return PLUGIN_ERROR;
        }

    }

    config_set_defaults();
    if (configfile_load(CFG_FILE, config, gCfg_sz, CFG_VER) < 0)
    {
        /* If the loading failed, save a new config file */
        config_set_defaults();
        configfile_save(CFG_FILE, config, gCfg_sz, CFG_VER);

        rb->splash(HZ, ID2P(LANG_REVERT_TO_DEFAULT_SETTINGS));
    }

    rb->memset(&printcell_data, 0, sizeof (struct printcell_data_t));
    printcell_data.fd_cur = -1;
    printcell_data.view_lastcol = -1;

    if (rb->file_exists(filename))
    {
        if (file_open(filename, &printcell_data.fd_cur) == 0)
            printcell_data.fd_cur = -1;
        else
        {
            size_t buf_size;
            printcell_data.buf = rb->plugin_get_buffer(&buf_size);
            printcell_data.buf_size = buf_size;
            printcell_data.buf_used = 0;
            printcell_data.filename = filename;
            items = file_load_entries(&printcell_data);
        }
    }
    int col_w = gConfig.col_width;
    rb->snprintf(printcell_data.header, PRINTCELL_MAXLINELEN,
                 SCROBBLE_HDR_FMT, col_w, rb->str(LANG_ID3_ARTIST),
                                   col_w, rb->str(LANG_ID3_ALBUM),
                                   col_w, rb->str(LANG_ID3_TITLE));

    if (!exit && items > 0)
    {
        synclist_set(0, items, 1, &printcell_data);
        rb->gui_synclist_draw(&lists);

        while (!exit)
        {
            action = rb->get_action(CONTEXT_LIST, HZ / 10);

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
    config_save();
    cleanup(&printcell_data);
    return ret;
}
