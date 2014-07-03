/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin, Dan Everton, Matthias Mohr
 *               2010 Jonathan Gordon
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#ifndef __PCTOOL__
#include "core_alloc.h"
#endif
#include "file.h"
#include "misc.h"
#include "plugin.h"
#include "viewport.h"

#include "skin_buffer.h"
#include "skin_debug.h"
#include "skin_parser.h"
#include "tag_table.h"

#ifdef __PCTOOL__
#ifdef WPSEDITOR
#include "proxy.h"
#include "sysfont.h"
#else
#include "action.h"
#include "checkwps.h"
#include "audio.h"
#define lang_is_rtl() (false)
#define DEBUGF printf
#endif /*WPSEDITOR*/
#else
#include "debug.h"
#include "language.h"
#endif /*__PCTOOL__*/

#include <ctype.h>
#include <stdbool.h>
#include "font.h"

#include "wps_internals.h"
#include "skin_engine.h"
#include "settings.h"
#include "settings_list.h"
#if CONFIG_TUNER
#include "radio.h"
#include "tuner.h"
#endif

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#endif

#ifdef HAVE_ALBUMART
#include "playback.h"
#endif

#include "backdrop.h"
#include "statusbar-skinned.h"

#define WPS_ERROR_INVALID_PARAM         -1

#define SAFE_MOVE_PTR(x, diff) if (x) { x = PTR_ADD(x, diff); }
#define SAFE_MOVE_PTR_CHECK(x, diff, check) if ((x) && (check)) { x = PTR_ADD(x, diff); }

#define SAFE_MOVE_BUFLIB_PTR(x, diff) SAFE_MOVE_PTR((x).__data, diff)
#define SAFE_MOVE_BUFLIB_PTR_CHECK(x, diff, check) SAFE_MOVE_PTR_CHECK((x).__data, diff, check)

struct movepointersdata {
    ptrdiff_t diff;
};

void movepointers_callback(struct skin_element *element, void *data);

static char* skin_buffer = NULL;
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
static char *backdrop_filename;
#endif
static struct skin_stats *_stats = NULL;

static bool isdefault(struct skin_tag_parameter *param)
{
    return param->type == DEFAULT;
}

static inline char*
get_param_text(struct skin_element *element, int param_number)
{
    return element->params[param_number].data.text;
}

static inline struct skin_element*
get_param_code(struct skin_element *element, int param_number)
{
    return element->params[param_number].data.code;
}

static inline struct skin_tag_parameter*
get_param(struct skin_element *element, int param_number)
{
    return &element->params[param_number];
}

/* which screen are we parsing for? */
static enum screen_type curr_screen;

/* the current viewport */
static struct skin_element *curr_viewport_element;
static struct skin_viewport *curr_vp;
static struct skin_element *first_viewport;

static struct line *curr_line;

static int follow_lang_direction = 0;

typedef int (*parse_function)(struct skin_element *element,
                              struct wps_token *token,
                              struct wps_data *wps_data);

#ifdef HAVE_LCD_BITMAP
/* add a skin_token_list item to the list chain. ALWAYS appended because some of the
 * chains require the order to be kept.
 */
static void add_to_ll_chain(struct skin_token_list **listoffset,
        struct skin_token_list *item)
{
    struct skin_token_list *list = *listoffset;
    if (list == NULL)
    {
        *listoffset = item;
    }
    else
    {
        while (list->next.__data)
            list = list->next.__data;
        list->next.__data = item;
    }
}

#endif
typedef void(*traverse_callback)(struct skin_element *element, void *data);
void skin_traverse_tree(struct skin_element *tree, traverse_callback callback, void *data)
{
    int i;
    struct skin_element *temp;
    
    while (tree)
    {
        for (i = 0; i < tree->children_count; i++)
        {
            skin_traverse_tree(tree->children[i], callback, data);
        }
        temp = tree;
        tree = tree->next;
        callback(temp, data);
    }
}

void *skin_find_item(const char *label, enum skin_find_what what,
                     struct wps_data *data)
{
    const char *itemlabel = NULL;
    union {
        struct skin_token_list *linkedlist;
        struct skin_element *vplist;
    } list = {NULL};
    bool isvplist = false;
    void *ret = NULL;
    switch (what)
    {
        case SKIN_FIND_UIVP:
        case SKIN_FIND_VP:
            list.vplist = data->tree;
            isvplist = true;
        break;
#ifdef HAVE_LCD_BITMAP
        case SKIN_FIND_IMAGE:
            list.linkedlist = data->images.__data;
        break;
#endif
#ifdef HAVE_TOUCHSCREEN
        case SKIN_FIND_TOUCHREGION:
            list.linkedlist = data->touchregions.__data;
        break;
#endif
#ifdef HAVE_SKIN_VARIABLES
        case SKIN_VARIABLE:
            list.linkedlist = data->skinvars.__data;
        break;
#endif
    }

    while (list.linkedlist)
    {
        bool skip = false;
#ifdef HAVE_LCD_BITMAP
        struct wps_token *token = NULL;
        if (!isvplist)
            token = list.linkedlist->token.__data;
#endif
        switch (what)
        {
            case SKIN_FIND_UIVP:
            case SKIN_FIND_VP:
                ret = list.vplist->data;
                if (((struct skin_viewport *)ret)->label.__data == VP_DEFAULT_LABEL)
                    itemlabel = VP_DEFAULT_LABEL_STRING;
                else
                    itemlabel = ((struct skin_viewport *)ret)->label.__data;
                skip = !(((struct skin_viewport *)ret)->is_infovp ==
                    (what==SKIN_FIND_UIVP));
                break;
#ifdef HAVE_LCD_BITMAP
            case SKIN_FIND_IMAGE:
                ret = token->value.data;
                itemlabel = ((struct gui_img *)ret)->label.__data;
                break;
#endif
#ifdef HAVE_TOUCHSCREEN
            case SKIN_FIND_TOUCHREGION:
                ret = token->value.data;
                itemlabel = ((struct touchregion *)ret)->label.__data;
                break;
#endif
#ifdef HAVE_SKIN_VARIABLES
            case SKIN_VARIABLE:
                ret = token->value.data;
                itemlabel = ((struct skin_var *)ret)->label.__data;
                break;
#endif

        }
        if (!skip && itemlabel && !strcmp(itemlabel, label))
        {
            return ret;
        }

        if (isvplist)
            list.vplist = list.vplist->next;
        else
            list.linkedlist = list.linkedlist->next.__data;
    }
    return NULL;
}

#ifdef HAVE_LCD_BITMAP

/* create and init a new wpsll item.
 * passing NULL to token will alloc a new one.
 * You should only pass NULL for the token when the token type (table above)
 * is WPS_NO_TOKEN which means it is not stored automatically in the skins token array
 */
static struct skin_token_list *new_skin_token_list_item(struct wps_token *token,
                                                        void* token_data)
{
    struct skin_token_list *llitem = skin_buffer_alloc(sizeof(*llitem));
    llitem->new_token = token == NULL;
    if (!token)
        token = skin_buffer_alloc(sizeof(*token));
    if (!llitem || !token)
        return NULL;
    llitem->next.__data = NULL;
    llitem->token.__data = token;
    if (token_data)
        token->value.data = token_data;
    return llitem;
}

/*
 * Use this move callback if the only thing that needs to be moved is the
 * element's token->data pointer.
 */
static void move_token_data(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;

    SAFE_MOVE_PTR(token->value.data, diff);
}
static const struct skin_tag_callbacks move_token_data_tag_callbacks =
{
    .move = move_token_data
};

static int parse_statusbar_tags(struct skin_element* element,
                                struct wps_token *token,
                                struct wps_data *wps_data)
{
    (void)element;
    if (token->type == SKIN_TOKEN_DRAW_INBUILTBAR)
    {
        token->value.data = (void*)&curr_vp->vp;
        token->callbacks = &move_token_data_tag_callbacks;
    }
    else
    {
        struct skin_viewport *default_vp = first_viewport->data;
        if (first_viewport->params_count == 0)
        {
            wps_data->wps_sb_tag = true;
            wps_data->show_sb_on_wps = (token->type == SKIN_TOKEN_ENABLE_THEME);
        }
        if (wps_data->show_sb_on_wps)
        {
            viewport_set_defaults(&default_vp->vp, curr_screen);
        }
        else
        {
            viewport_set_fullscreen(&default_vp->vp, curr_screen);
        }
#ifdef HAVE_REMOTE_LCD
        /* This parser requires viewports which will use the settings font to
         * have font == 1, but the above viewport_set() calls set font to
         * the current real font id. So force 1 here it will be set correctly
         * at the end
         */
        default_vp->vp.font = 1;
#endif
    }
    return 0;
}

static int get_image_id(int c)
{
    if(c >= 'a' && c <= 'z')
        return c - 'a';
    else if(c >= 'A' && c <= 'Z')
        return c - 'A' + 26;
    else
        return -1;
}

void get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size)
{
    snprintf(buf, buf_size, "%s/%s", bmpdir, start);
}

static void move_image_display(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct image_display *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->label, diff);
    SAFE_MOVE_BUFLIB_PTR(data->token, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_image_display(struct skin_element *element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks = {
        .move = move_image_display,
    };
    char *label = get_param_text(element, 0);
    char sublabel = '\0';
    int subimage;
    struct gui_img *img;
    struct image_display *id = skin_buffer_alloc(sizeof(*id));
    token->callbacks = &tag_callbacks;

    if (element->params_count == 1 && strlen(label) <= 2)
    {
        /* backwards compatability. Allow %xd(Aa) to still work */
        sublabel = label[1];
        label[1] = '\0';
    }
    /* sanity check */
    img = skin_find_item(label, SKIN_FIND_IMAGE, wps_data);
    if (!img || !id)
    {
        return WPS_ERROR_INVALID_PARAM;
    }
    id->label.__data = img->label.__data;
    id->offset = 0;
    id->token.__data = NULL;
    if (img->using_preloaded_icons)
    {
        token->type = SKIN_TOKEN_IMAGE_DISPLAY_LISTICON;
    }

    if (token->type == SKIN_TOKEN_IMAGE_DISPLAY_9SEGMENT)
        img->is_9_segment = true;

    if (element->params_count > 1)
    {
        if (get_param(element, 1)->type == CODE)
            id->token.__data = get_param_code(element, 1)->data;
        /* specify a number. 1 being the first subimage (i.e top) NOT 0 */
        else if (get_param(element, 1)->type == INTEGER)
            id->subimage = get_param(element, 1)->data.number - 1;
        if (element->params_count > 2)
            id->offset = get_param(element, 2)->data.number;
    }
    else
    {
        if ((subimage = get_image_id(sublabel)) != -1)
        {
            if (subimage >= img->num_subimages)
                return WPS_ERROR_INVALID_PARAM;
            id->subimage = subimage;
        } else {
            id->subimage = 0;
        }
    }
    token->value.data = id;
    return 0;
}

static void move_image_load(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct gui_img *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->label, diff);
    SAFE_MOVE_BUFLIB_PTR(data->vp, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_image_load(struct skin_element *element,
                            struct wps_token *token,
                            struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks = {
        .move = move_image_load,
    };
    
    const char* filename;
    char* id;
    int x = 0,y = 0, subimages = 1;
    struct gui_img *img;

    /* format: %x(n,filename.bmp[,x,y])
       or %xl(n,filename.bmp[,x,y])
       or %xl(n,filename.bmp[,x,y,num_subimages])
    */

    id = get_param_text(element, 0);
    filename = get_param_text(element, 1);
    /* x,y,num_subimages handling:
     * If all 3 are left out use sane defaults.
     * If there are 2 params it must be x,y
     * if there is only 1 param it must be the num_subimages
     */
    if (element->params_count == 3)
        subimages = get_param(element, 2)->data.number;
    else if (element->params_count > 3)
    {
        x = get_param(element, 2)->data.number;
        y = get_param(element, 3)->data.number;
        if (element->params_count == 5)
            subimages = get_param(element, 4)->data.number;
    }
    /* check the image number and load state */
    if(skin_find_item(id, SKIN_FIND_IMAGE, wps_data))
    {
        /* Invalid image ID */
        return WPS_ERROR_INVALID_PARAM;
    }
    img = skin_buffer_alloc(sizeof(*img));
    if (!img)
        return WPS_ERROR_INVALID_PARAM;
    /* save a pointer to the filename */
    img->bm.data = (char*)filename;
    img->label.__data = id;
    img->x = x;
    img->y = y;
    img->num_subimages = subimages;
    img->display = -1;
    img->using_preloaded_icons = false;
    img->buflib_handle = -1;
    img->is_9_segment = false;
    img->loaded = false;

    /* save current viewport */
    img->vp.__data = &curr_vp->vp;

    struct skin_token_list *item = new_skin_token_list_item(NULL, img);
    if (!item)
        return WPS_ERROR_INVALID_PARAM;
    add_to_ll_chain(&wps_data->images.__data, item);

    if (token->type == SKIN_TOKEN_IMAGE_DISPLAY)
    {
        token->callbacks = &tag_callbacks;
        token->value.data = img;
    }
    else
    {
        item->token.__data->callbacks = &tag_callbacks;
    }

    if (!strcmp(img->bm.data, "__list_icons__"))
    {
        img->num_subimages = Icon_Last_Themeable;
        img->using_preloaded_icons = true;
    }


    return 0;
}
struct skin_font {
    int id; /* the id from font_load */
    char *name;  /* filename without path and extension */
    int glyphs;  /* how many glyphs to reserve room for */
};
static struct skin_font skinfonts[MAXUSERFONTS];
static int parse_font_load(struct skin_element *element,
                           struct wps_token *token,
                           struct wps_data *wps_data)
{
    (void)wps_data; (void)token;
    int id = get_param(element, 0)->data.number;
    char *filename = get_param_text(element, 1);
    int  glyphs;
    char *ptr;

    if(element->params_count > 2)
        glyphs = get_param(element, 2)->data.number;
    else
        glyphs = global_settings.glyphs_to_cache;
    if (id < 2)
    {
        DEBUGF("font id must be >= 2 (%d)\n", id);
        return -1;
    }
#if defined(DEBUG) || defined(SIMULATOR)
    if (skinfonts[id-2].name != NULL)
    {
        DEBUGF("font id %d already being used\n", id);
    }
#endif
    /* make sure the filename contains .fnt,
     * we dont actually use it, but require it anyway */
    ptr = strchr(filename, '.');
    if (!ptr || strncmp(ptr, ".fnt", 4))
        return WPS_ERROR_INVALID_PARAM;
    skinfonts[id-2].id = -1;
    skinfonts[id-2].name = filename;
    skinfonts[id-2].glyphs = glyphs;

    return 0;
}


#ifdef HAVE_LCD_BITMAP

static int parse_playlistview(struct skin_element *element,
                              struct wps_token *token,
                              struct wps_data *wps_data)
{
    (void)wps_data;
    struct playlistviewer *viewer = skin_buffer_alloc(sizeof(*viewer));
    if (!viewer)
        return WPS_ERROR_INVALID_PARAM;
    viewer->vp.__data = &curr_vp->vp;
    viewer->show_icons = true;
    viewer->start_offset = get_param(element, 0)->data.number;
    viewer->line.__data = get_param_code(element, 1); // FIXME: definitly test this

    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = viewer;

    return 0;
}
#endif
#ifdef HAVE_LCD_COLOR
static int parse_viewport_gradient_setup(struct skin_element *element,
                                   struct wps_token *token,
                                   struct wps_data *wps_data)
{
    (void)wps_data;
    struct gradient_config *cfg;
    if (element->params_count < 2) /* only start and end are required */
        return 1;
    cfg = skin_buffer_alloc(sizeof(*cfg));
    if (!cfg)
        return 1;
    if (!parse_color(curr_screen, get_param_text(element, 0), &cfg->start) ||
        !parse_color(curr_screen, get_param_text(element, 1), &cfg->end))
        return 1;
    if (element->params_count > 2)
    {
        if (!parse_color(curr_screen, get_param_text(element, 2), &cfg->text))
            return 1;
    }
    else
    {
        cfg->text = curr_vp->vp.fg_pattern;
    }

    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = cfg;
    return 0;
}
#endif

static int parse_listitem(struct skin_element *element,
                        struct wps_token *token,
                        struct wps_data *wps_data)
{
    (void)wps_data;
    struct listitem *li = skin_buffer_alloc(sizeof(*li));
    if (!li)
        return -1;
    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = li;
    if (element->params_count == 0)
        li->offset = 0;
    else
    {
        li->offset = get_param(element, 0)->data.number;
        if (element->params_count > 1)
            li->wrap = strcasecmp(get_param_text(element, 1), "nowrap") != 0;
        else
            li->wrap = true;
    }
    return 0;
}
#ifndef __PCTOOL__
static void move_listitemviewport(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct listitem_viewport_cfg *data = token->value.data;

    SAFE_MOVE_BUFLIB_PTR(data->label, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}
#endif

static int parse_listitemviewport(struct skin_element *element,
                                  struct wps_token *token,
                                  struct wps_data *wps_data)
{
#ifndef __PCTOOL__
    static const struct skin_tag_callbacks tag_callbacks = {
        .move = move_listitemviewport,
    };
    struct listitem_viewport_cfg *cfg = skin_buffer_alloc(sizeof(*cfg));
    if (!cfg)
        return -1;
    cfg->data = wps_data;
    cfg->tile = false;
    cfg->label.__data = get_param_text(element, 0);
    cfg->width = -1;
    cfg->height = -1;
    if (!isdefault(get_param(element, 1)))
        cfg->width = get_param(element, 1)->data.number;
    if (!isdefault(get_param(element, 2)))
        cfg->height = get_param(element, 2)->data.number;
    if (element->params_count > 3 &&
        !strcmp(get_param_text(element, 3), "tile"))
        cfg->tile = true;
    token->callbacks = &tag_callbacks;
    token->value.data = cfg;
#endif
    return 0;
}

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
static int parse_viewporttextstyle(struct skin_element *element,
                                   struct wps_token *token,
                                   struct wps_data *wps_data)
{
    (void)wps_data;
    char *mode = get_param_text(element, 0);
    struct line_desc *line = skin_buffer_alloc(sizeof(*line));
    *line = (struct line_desc)LINE_DESC_DEFINIT;
    unsigned colour;

    if (!strcmp(mode, "invert"))
    {
        line->style = STYLE_INVERT;
    }
    else if (!strcmp(mode, "colour") || !strcmp(mode, "color"))
    {
        if (element->params_count < 2 ||
            !parse_color(curr_screen, get_param_text(element, 1), &colour))
            return 1;
        line->style = STYLE_COLORED;
        line->text_color = colour;
    }
#ifdef HAVE_LCD_COLOR
    else if (!strcmp(mode, "gradient"))
    {
        int num_lines;
        if (element->params_count < 2)
            num_lines = 1;
        else /* atoi() instead of using a number in the parser is because [si]
              * will select the number for something which looks like a colour
              * making the "colour" case (above) harder to parse */
            num_lines = atoi(get_param_text(element, 1));
        line->style = STYLE_GRADIENT;
        line->nlines = num_lines;
    }
#endif
    else if (!strcmp(mode, "clear"))
    {
        line->style = STYLE_DEFAULT;
    }
    else
        return 1;

    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = line;
    return 0;
}

static int parse_drawrectangle( struct skin_element *element,
                                struct wps_token *token,
                                struct wps_data *wps_data)
{
    (void)wps_data;
    struct draw_rectangle *rect = skin_buffer_alloc(sizeof(*rect));

    if (!rect)
        return -1;

    rect->x = get_param(element, 0)->data.number;
    rect->y = get_param(element, 1)->data.number;

    if (isdefault(get_param(element, 2)))
        rect->width = curr_vp->vp.width - rect->x;
    else
        rect->width = get_param(element, 2)->data.number;

    if (isdefault(get_param(element, 3)))
        rect->height = curr_vp->vp.height - rect->y;
    else
        rect->height = get_param(element, 3)->data.number;

    rect->start_colour = curr_vp->vp.fg_pattern;
    rect->end_colour = curr_vp->vp.fg_pattern;

    if (element->params_count > 4)
    {
        if (!parse_color(curr_screen, get_param_text(element, 4),
                    &rect->start_colour))
            return -1;
        rect->end_colour = rect->start_colour;
    }
    if (element->params_count > 5)
    {
        if (!parse_color(curr_screen, get_param_text(element, 5),
                    &rect->end_colour))
            return -1;
    }
    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = rect;

    return 0;
}

static void move_viewportcolour(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct viewport_colour *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->vp, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_viewportcolour(struct skin_element *element,
                                struct wps_token *token,
                                struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_viewportcolour
    };
    (void)wps_data;
    struct skin_tag_parameter *param = get_param(element, 0);
    struct viewport_colour *colour = skin_buffer_alloc(sizeof(*colour));
    if (!colour)
        return -1;
    if (isdefault(param))
    {
        colour->colour = get_viewport_default_colour(curr_screen,
                                   token->type == SKIN_TOKEN_VIEWPORT_FGCOLOUR);
    }
    else
    {
        if (!parse_color(curr_screen, param->data.text, &colour->colour))
            return -1;
    }
    colour->vp.__data = &curr_vp->vp;
    token->callbacks = &tag_callbacks;
    token->value.data = colour;
    if (element->line == curr_viewport_element->line)
    {
        if (token->type == SKIN_TOKEN_VIEWPORT_FGCOLOUR)
            curr_vp->vp.fg_pattern = colour->colour;
        else
            curr_vp->vp.bg_pattern = colour->colour;
    }
    return 0;
}

static int parse_image_special(struct skin_element *element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    (void)wps_data; /* kill warning */
    (void)token;

#if LCD_DEPTH > 1
    char *filename;
    if (token->type == SKIN_TOKEN_IMAGE_BACKDROP)
    {
        if (isdefault(get_param(element, 0)))
        {
            filename = "-";
        }
        else
        {
            filename = get_param_text(element, 0);
            /* format: %X(filename.bmp) or %X(d) */
            if (!strcmp(filename, "d"))
                filename = NULL;
        }
        backdrop_filename = filename;
    }
#endif

    return 0;
}
#endif

#endif /* HAVE_LCD_BITMAP */

static int parse_progressbar_tag(struct skin_element* element,
                                 struct wps_token *token,
                                 struct wps_data *wps_data);

static int parse_setting_and_lang(struct skin_element *element,
                                  struct wps_token *token,
                                  struct wps_data *wps_data)
{
    /* NOTE: both the string validations that happen in here will
     * automatically PASS on checkwps because its too hard to get
     * settings_list.c and english.lang built for it.
     * If that ever changes remove the #ifndef __PCTOOL__'s here
     */
    (void)wps_data;
    char *temp = get_param_text(element, 0);
    int i;

    if (token->type == SKIN_TOKEN_TRANSLATEDSTRING)
    {
#ifndef __PCTOOL__
        i = lang_english_to_id(temp);
        if (i < 0)
            return WPS_ERROR_INVALID_PARAM;
#endif
    }
    else if (element->params_count > 1)
    {
        if (element->params_count > 4)
            return parse_progressbar_tag(element, token, wps_data);
        else
            return WPS_ERROR_INVALID_PARAM;
    }
    else
    {
#ifndef __PCTOOL__
        if (find_setting_by_cfgname(temp, &i) == NULL)
            return WPS_ERROR_INVALID_PARAM;
#endif
    }
    /* Store the setting number */
    token->value.i = i;
    return 0;
}

static int parse_logical_andor(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    (void)wps_data;
    token->callbacks = &move_token_data_tag_callbacks;
    token->value.data = element;
    return 0;
}

static void move_logical_if(struct skin_element *element, ptrdiff_t diff)
{
#ifndef __PCTOOL__
    struct wps_token *token = element->data;
    struct logical_if *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->token, diff);
    
    if (data->operand.type == STRING)
    {
        SAFE_MOVE_PTR(data->operand.data.text, diff);
    }
    else if (data->operand.type == CODE)
    {
        struct movepointersdata movedata = { .diff = diff};
        movepointers_callback(data->operand.data.code, &movedata);
        SAFE_MOVE_PTR(data->operand.data.code, diff);
    }
    SAFE_MOVE_PTR(token->value.data, diff);
#endif
}
    
static int parse_logical_if(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_logical_if
    };
    (void)wps_data;
    char *op = get_param_text(element, 1);
    struct logical_if *lif = skin_buffer_alloc(sizeof(*lif));
    if (!lif)
        return -1;
    token->callbacks = &tag_callbacks;
    token->value.data = lif;
    lif->token.__data = get_param_code(element, 0)->data;

    if (!strncmp(op, "=", 1))
        lif->op = IF_EQUALS;
    else if (!strncmp(op, "!=", 2))
        lif->op = IF_NOTEQUALS;
    else if (!strncmp(op, ">=", 2))
        lif->op = IF_GREATERTHAN_EQ;
    else if (!strncmp(op, "<=", 2))
        lif->op = IF_LESSTHAN_EQ;
    else if (!strncmp(op, ">", 2))
        lif->op = IF_GREATERTHAN;
    else if (!strncmp(op, "<", 1))
        lif->op = IF_LESSTHAN;

    memcpy(&lif->operand, get_param(element, 2), sizeof(lif->operand));
    if (element->params_count > 3)
        lif->num_options = get_param(element, 3)->data.number;
    else
        lif->num_options = TOKEN_VALUE_ONLY;
    return 0;

}

static int parse_timeout_tag(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    (void)wps_data;
    int val = 0;
    if (element->params_count == 0)
    {
        switch (token->type)
        {
            case SKIN_TOKEN_SUBLINE_TIMEOUT:
                return -1;
            case SKIN_TOKEN_BUTTON_VOLUME:
            case SKIN_TOKEN_TRACK_STARTING:
            case SKIN_TOKEN_TRACK_ENDING:
                val = 10;
                break;
            default:
                break;
        }
    }
    else
        val = get_param(element, 0)->data.number;
    token->value.i = val * TIMEOUT_UNIT;
    return 0;
}

static void move_substring(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct substring *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->token, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_substring_tag(struct skin_element* element,
                                 struct wps_token *token,
                                 struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_substring
    };
    (void)wps_data;
    struct substring *ss = skin_buffer_alloc(sizeof(*ss));
    if (!ss)
        return 1;
    ss->start = get_param(element, 0)->data.number;
    if (get_param(element, 1)->type == DEFAULT)
        ss->length = -1;
    else
        ss->length = get_param(element, 1)->data.number;
    ss->token.__data = get_param_code(element, 2)->data;
    if (element->params_count > 3)
        ss->expect_number = !strcmp(get_param_text(element, 3), "number");
    else
        ss->expect_number = false;
    token->callbacks = &tag_callbacks;
    token->value.data = ss;
    return 0;
}

static void move_progressbar(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct progressbar *data = token->value.data;

    SAFE_MOVE_BUFLIB_PTR(data->vp, diff);
    SAFE_MOVE_BUFLIB_PTR(data->image, diff);
    SAFE_MOVE_BUFLIB_PTR(data->slider, diff);
    SAFE_MOVE_BUFLIB_PTR(data->backdrop, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

#ifdef HAVE_TOUCHSCREEN
static void move_touchregion(struct skin_element *element, ptrdiff_t diff);
#endif

static int parse_progressbar_tag(struct skin_element* element,
                                 struct wps_token *token,
                                 struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_progressbar
    };
#ifdef HAVE_LCD_BITMAP
    struct progressbar *pb;
    struct viewport *vp = &curr_vp->vp;
    struct skin_tag_parameter *param = get_param(element, 0);
    int curr_param = 0;
    char *image_filename = NULL;
#ifdef HAVE_TOUCHSCREEN
    bool suppress_touchregion = false;
#endif

    if (element->params_count == 0 &&
        element->tag->type != SKIN_TOKEN_PROGRESSBAR)
        return 0; /* nothing to do */
    pb = skin_buffer_alloc(sizeof(*pb));

    token->callbacks = &tag_callbacks;
    token->value.data = pb;

    if (!pb)
        return WPS_ERROR_INVALID_PARAM;
    pb->vp.__data = vp;
    pb->follow_lang_direction = follow_lang_direction > 0;
    pb->nofill = false;
    pb->noborder = false;
    pb->nobar = false;
    pb->image.__data = NULL;
    pb->slider.__data = NULL;
    pb->backdrop.__data = NULL;
    pb->setting_id = -1;
    pb->invert_fill_direction = false;
    pb->horizontal = true;

    if (element->params_count == 0)
    {
        pb->x = 0;
        pb->width = vp->width;
        pb->height = SYSFONT_HEIGHT-2;
        pb->y = -1; /* Will be computed during the rendering */
        pb->type = element->tag->type;
        return 0;
    }

    /* (x, y, width, height, ...) */
    if (!isdefault(param))
    {
        pb->x = param->data.number;
        if (pb->x < 0 || pb->x >= vp->width)
            return WPS_ERROR_INVALID_PARAM;
    }
    else
        pb->x = 0;
    param++;

    if (!isdefault(param))
    {
        pb->y = param->data.number;
        if (pb->y < 0 || pb->y >= vp->height)
            return WPS_ERROR_INVALID_PARAM;
    }
    else
        pb->y = -1; /* computed at rendering */
    param++;

    if (!isdefault(param))
    {
        pb->width = param->data.number;
        if (pb->width <= 0 || (pb->x + pb->width) > vp->width)
            return WPS_ERROR_INVALID_PARAM;
    }
    else
        pb->width = vp->width - pb->x;
    param++;

    if (!isdefault(param))
    {
        int max;
        pb->height = param->data.number;
        /* include y in check only if it was non-default */
        max = (pb->y > 0) ? pb->y + pb->height : pb->height;
        if (pb->height <= 0 || max > vp->height)
            return WPS_ERROR_INVALID_PARAM;
    }
    else
    {
        if (vp->font > FONT_UI)
            pb->height = -1; /* calculate at display time */
        else
        {
#ifndef __PCTOOL__
            pb->height = font_get(vp->font)->height;
#else
            pb->height = 8;
#endif
        }
    }
    /* optional params, first is the image filename if it isnt recognised as a keyword */

    curr_param = 4;
    if (isdefault(get_param(element, curr_param)))
    {
        param++;
        curr_param++;
    }

    pb->horizontal = pb->width > pb->height;
    while (curr_param < element->params_count)
    {
        char* text;
        param++;
        text = param->data.text;
        if (!strcmp(text, "invert"))
            pb->invert_fill_direction = true;
        else if (!strcmp(text, "nofill"))
            pb->nofill = true;
        else if (!strcmp(text, "noborder"))
            pb->noborder = true;
        else if (!strcmp(text, "nobar"))
            pb->nobar = true;
        else if (!strcmp(text, "slider"))
        {
            if (curr_param+1 < element->params_count)
            {
                curr_param++;
                param++;
                text = param->data.text;
                pb->slider.__data = skin_find_item(text, SKIN_FIND_IMAGE, wps_data);
            }
            else /* option needs the next param */
                return -1;
        }
        else if (!strcmp(text, "image"))
        {
            if (curr_param+1 < element->params_count)
            {
                curr_param++;
                param++;
                image_filename = param->data.text;
            }
            else /* option needs the next param */
                return -1;
        }
        else if (!strcmp(text, "backdrop"))
        {
            if (curr_param+1 < element->params_count)
            {
                curr_param++;
                param++;
                text = param->data.text;
                pb->backdrop.__data = skin_find_item(text, SKIN_FIND_IMAGE, wps_data);

            }
            else /* option needs the next param */
                return -1;
        }
        else if (!strcmp(text, "vertical"))
        {
            pb->horizontal = false;
            if (isdefault(get_param(element, 3)))
                pb->height = vp->height - pb->y;
        }
        else if (!strcmp(text, "horizontal"))
            pb->horizontal = true;
#ifdef HAVE_TOUCHSCREEN
        else if (!strcmp(text, "notouch"))
            suppress_touchregion = true;
#endif
		else if (token->type == SKIN_TOKEN_SETTING && !strcmp(text, "setting"))
		{
            if (curr_param+1 < element->params_count)
            {
                curr_param++;
                param++;
                text = param->data.text;
#ifndef __PCTOOL__
				if (find_setting_by_cfgname(text, &pb->setting_id) == NULL)
					return WPS_ERROR_INVALID_PARAM;
#endif
			}
		}
        else if (curr_param == 4)
            image_filename = text;

        curr_param++;
    }

    if (image_filename)
    {
        /* noborder is incompatible together with image. There is no border
         * anyway. */
        if (pb->noborder)
            return WPS_ERROR_INVALID_PARAM;

        pb->image.__data = skin_find_item(image_filename, SKIN_FIND_IMAGE, wps_data);
        if (!pb->image.__data) /* load later */
        {
            static const struct skin_tag_callbacks pbimage_callbacks = {
                .move = move_image_load,
            };
            struct gui_img *img = skin_buffer_alloc(sizeof(*img));
            if (!img)
                return WPS_ERROR_INVALID_PARAM;
            /* save a pointer to the filename */
            img->bm.data = (char*)image_filename;
            img->label.__data = image_filename;
            img->x = 0;
            img->y = 0;
            img->num_subimages = 1;
            img->display = -1;
            img->using_preloaded_icons = false;
            img->buflib_handle = -1;
            img->vp.__data = &curr_vp->vp;
            img->loaded = false;
            struct skin_token_list *item = new_skin_token_list_item(NULL, img);
            if (!item)
                return WPS_ERROR_INVALID_PARAM;
            add_to_ll_chain(&wps_data->images.__data, item);
            item->token.__data->callbacks = &pbimage_callbacks;
            pb->image.__data = img;
        }
    }

    if (token->type == SKIN_TOKEN_VOLUME)
        token->type = SKIN_TOKEN_VOLUMEBAR;
    else if (token->type == SKIN_TOKEN_BATTERY_PERCENT)
        token->type = SKIN_TOKEN_BATTERY_PERCENTBAR;
    else if (token->type == SKIN_TOKEN_TUNER_RSSI)
        token->type = SKIN_TOKEN_TUNER_RSSI_BAR;
    else if (token->type == SKIN_TOKEN_PEAKMETER_LEFT)
        token->type = SKIN_TOKEN_PEAKMETER_LEFTBAR;
    else if (token->type == SKIN_TOKEN_PEAKMETER_RIGHT)
        token->type = SKIN_TOKEN_PEAKMETER_RIGHTBAR;
    else if (token->type == SKIN_TOKEN_LIST_NEEDS_SCROLLBAR)
        token->type = SKIN_TOKEN_LIST_SCROLLBAR;
    else if (token->type == SKIN_TOKEN_SETTING)
		token->type = SKIN_TOKEN_SETTINGBAR;
    pb->type = token->type;

#ifdef HAVE_TOUCHSCREEN
    if (!suppress_touchregion &&
        (token->type == SKIN_TOKEN_VOLUMEBAR ||
         token->type == SKIN_TOKEN_PROGRESSBAR ||
         token->type == SKIN_TOKEN_SETTINGBAR))
    {
        static const struct skin_tag_callbacks touchregion_callbacks = {
            .move = move_touchregion,
        };
        struct touchregion *region = skin_buffer_alloc(sizeof(*region));
        struct skin_token_list *item;
        int wpad, hpad;

        if (!region)
            return 0;

        if (token->type == SKIN_TOKEN_VOLUMEBAR)
            region->action = ACTION_TOUCH_VOLUME;
        else if (token->type == SKIN_TOKEN_SETTINGBAR)
            region->action = ACTION_TOUCH_SETTING;
        else
            region->action = ACTION_TOUCH_SCROLLBAR;

        /* try to add some extra space on either end to make pressing the
         * full bar easier. ~5% on either side
         */
        wpad = pb->width * 5 / 100;
        if (wpad > 10)
            wpad = 10;
        hpad = pb->height * 5 / 100;
        if (hpad > 10)
            hpad = 10;

        region->x = pb->x - wpad;
        if (region->x < 0)
            region->x = 0;
        region->width = pb->width + 2 * wpad;
        if (region->x + region->width > curr_vp->vp.x + curr_vp->vp.width)
            region->width = curr_vp->vp.x + curr_vp->vp.width - region->x;

        region->y = pb->y - hpad;
        if (region->y < 0)
            region->y = 0;
        region->height = pb->height + 2 * hpad;
        if (region->y + region->height > curr_vp->vp.y + curr_vp->vp.height)
            region->height = curr_vp->vp.y + curr_vp->vp.height - region->y;

        region->wvp.__data = curr_vp;
        region->reverse_bar = false;
        region->allow_while_locked = false;
        region->press_length = PRESS;
        region->last_press = 0xffff;
        region->armed = false;
        region->bar.__data = pb;

        item = new_skin_token_list_item(NULL, region);
        if (!item)
            return WPS_ERROR_INVALID_PARAM;
        item->token.__data->callbacks = &touchregion_callbacks;
        add_to_ll_chain(&wps_data->touchregions.__data, item);
    }
#endif

    return 0;

#else
    (void)element;
    if (token->type == SKIN_TOKEN_PROGRESSBAR ||
        token->type == SKIN_TOKEN_PLAYER_PROGRESSBAR)
    {
        wps_data->full_line_progressbar =
                        token->type == SKIN_TOKEN_PLAYER_PROGRESSBAR;
    }
    return 0;

#endif
}

#ifdef HAVE_ALBUMART
static void move_albumart(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct skin_albumart *data = token->value.data;

    SAFE_MOVE_BUFLIB_PTR(data->vp, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_albumart_load(struct skin_element* element,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_albumart
    };
    struct dim dimensions;
    int albumart_slot;
    bool swap_for_rtl = lang_is_rtl() && follow_lang_direction;
    struct skin_albumart *aa = skin_buffer_alloc(sizeof(*aa));

    if (!aa)
        return -1;

    token->callbacks = &tag_callbacks;
    token->value.data = aa;
    /* reset albumart info in wps */
    aa->width = -1;
    aa->height = -1;
    aa->xalign = WPS_ALBUMART_ALIGN_CENTER; /* default */
    aa->yalign = WPS_ALBUMART_ALIGN_CENTER; /* default */

    aa->x = get_param(element, 0)->data.number;
    aa->y = get_param(element, 1)->data.number;
    aa->width = get_param(element, 2)->data.number;
    aa->height = get_param(element, 3)->data.number;

    aa->vp.__data = &curr_vp->vp;
    aa->draw_handle = -1;

    /* if we got here, we parsed everything ok .. ! */
    if (aa->width < 0)
        aa->width = 0;

    if (aa->height < 0)
        aa->height = 0;

    if (swap_for_rtl)
        aa->x = (curr_vp->vp.width - aa->width - aa->x);

    aa->state = WPS_ALBUMART_LOAD;
    wps_data->albumart.__data = aa;

    dimensions.width = aa->width;
    dimensions.height = aa->height;

    albumart_slot = playback_claim_aa_slot(&dimensions);

    if (0 <= albumart_slot)
        wps_data->playback_aa_slot = albumart_slot;

    if (element->params_count > 4 && !isdefault(get_param(element, 4)))
    {
        switch (*get_param_text(element, 4))
        {
            case 'l':
            case 'L':
                if (swap_for_rtl)
                    aa->xalign = WPS_ALBUMART_ALIGN_RIGHT;
                else
                    aa->xalign = WPS_ALBUMART_ALIGN_LEFT;
                break;
            case 'c':
            case 'C':
                aa->xalign = WPS_ALBUMART_ALIGN_CENTER;
                break;
            case 'r':
            case 'R':
                if (swap_for_rtl)
                    aa->xalign = WPS_ALBUMART_ALIGN_LEFT;
                else
                    aa->xalign = WPS_ALBUMART_ALIGN_RIGHT;
                break;
        }
    }
    if (element->params_count > 5 && !isdefault(get_param(element, 5)))
    {
        switch (*get_param_text(element, 5))
        {
            case 't':
            case 'T':
                aa->yalign = WPS_ALBUMART_ALIGN_TOP;
                break;
            case 'c':
            case 'C':
                aa->yalign = WPS_ALBUMART_ALIGN_CENTER;
                break;
            case 'b':
            case 'B':
                aa->yalign = WPS_ALBUMART_ALIGN_BOTTOM;
                break;
        }
    }
    return 0;
}

#endif /* HAVE_ALBUMART */
#ifdef HAVE_SKIN_VARIABLES
static void move_skinvar_from_list(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct skin_var *data = token->value.data; 
    
    SAFE_MOVE_BUFLIB_PTR(data->label, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static struct skin_var* find_or_add_var(const char* label,
                                        struct wps_data *data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_skinvar_from_list
    };
    struct skin_var* ret = skin_find_item(label, SKIN_VARIABLE, data);
    if (ret)
        return ret;

    ret = skin_buffer_alloc(sizeof(*ret));
    if (!ret)
        return ret;
    ret->label.__data = label;
    ret->value = 1;
    ret->last_changed = 0xffff;
    struct skin_token_list *item = new_skin_token_list_item(NULL, ret);
    if (!item)
        return NULL;
    add_to_ll_chain(&data->skinvars.__data, item);
    item->token.__data->callbacks = &tag_callbacks;
    return ret;
}

static void move_skinvar(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    switch (token->type)
    {
        case SKIN_TOKEN_VAR_GETVAL:
            // just move the token->value.data at the end
            break;
        case SKIN_TOKEN_VAR_SET:
        {
            struct skin_var_changer *data = token->value.data;
            SAFE_MOVE_BUFLIB_PTR(data->var, diff);
        }
        break;
        case SKIN_TOKEN_VAR_TIMEOUT:
        {
            struct skin_var_lastchange *data = token->value.data;
            SAFE_MOVE_BUFLIB_PTR(data->var, diff);
        }
        break;
        default:
            break;
    }
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_skinvar(  struct skin_element *element,
                           struct wps_token *token,
                           struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_skinvar
    };
    const char* label = get_param_text(element, 0);
    struct skin_var* var = find_or_add_var(label, wps_data);
    if (!var)
        return WPS_ERROR_INVALID_PARAM;
    token->callbacks = &tag_callbacks;
    switch (token->type)
    {
        case SKIN_TOKEN_VAR_GETVAL:
            token->value.data = var;
            return 0;
        case SKIN_TOKEN_VAR_SET:
        {
            struct skin_var_changer *data = skin_buffer_alloc(sizeof(*data));
            if (!data)
                return WPS_ERROR_INVALID_PARAM;
            data->var.__data = var;
            if (!isdefault(get_param(element, 2)))
                data->newval = get_param(element, 2)->data.number;
            else if (strcmp(get_param_text(element, 1), "touch"))
                return WPS_ERROR_INVALID_PARAM;
            data->max = 0;
            if (!strcmp(get_param_text(element, 1), "set"))
                data->direct = true;
            else if (!strcmp(get_param_text(element, 1), "inc"))
            {
                data->direct = false;
            }
            else if (!strcmp(get_param_text(element, 1), "dec"))
            {
                data->direct = false;
                data->newval *= -1;
            }
            else if (!strcmp(get_param_text(element, 1), "touch"))
            {
                data->direct = false;
                data->newval = 0;
            }
            if (element->params_count > 3)
                data->max = get_param(element, 3)->data.number;
            token->value.data = data;
        }
        return 0;
        case SKIN_TOKEN_VAR_TIMEOUT:
        {
            struct skin_var_lastchange *data = skin_buffer_alloc(sizeof(*data));
            if (!data)
                return WPS_ERROR_INVALID_PARAM;
            data->var.__data = var;
            data->timeout = 10;
            if (element->params_count > 1)
                data->timeout = get_param(element, 1)->data.number;
            data->timeout *= TIMEOUT_UNIT;
            token->value.data = data;
        }
        default:
            return 0;
    }
}
#endif /* HAVE_SKIN_VARIABLES */
#ifdef HAVE_TOUCHSCREEN
static void move_lasttouch(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct touchregion_lastpress *data = token->value.data;

    SAFE_MOVE_BUFLIB_PTR(data->region, diff);
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_lasttouch(struct skin_element *element,
                           struct wps_token *token,
                           struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .move = move_lasttouch
    };
    struct touchregion_lastpress *data = skin_buffer_alloc(sizeof(*data));
    int i;
    struct touchregion *region = NULL;
    if (!data)
        return WPS_ERROR_INVALID_PARAM;

    data->timeout = 10;

    for (i=0; i<element->params_count; i++)
    {
        if (get_param(element, i)->type == STRING)
            region = skin_find_item(get_param_text(element, i),
                                          SKIN_FIND_TOUCHREGION, wps_data);
        else if (get_param(element, i)->type == INTEGER ||
                 get_param(element, i)->type == DECIMAL)
            data->timeout = get_param(element, i)->data.number;
    }

    data->region.__data = region;
    data->timeout *= TIMEOUT_UNIT;
    token->callbacks = &tag_callbacks;
    token->value.data = data;
    return 0;
}

struct touchaction {const char* s; int action;};
static const struct touchaction touchactions[] = {
    /* generic actions, convert to screen actions on use */
    {"none", ACTION_TOUCHSCREEN_IGNORE},{"lock", ACTION_TOUCH_SOFTLOCK },
    {"prev", ACTION_STD_PREV },         {"next", ACTION_STD_NEXT },
    {"hotkey", ACTION_STD_HOTKEY},      {"select", ACTION_STD_OK },
    {"menu", ACTION_STD_MENU },         {"cancel", ACTION_STD_CANCEL },
    {"contextmenu", ACTION_STD_CONTEXT},{"quickscreen", ACTION_STD_QUICKSCREEN },

    /* list/tree actions */
    { "resumeplayback", ACTION_TREE_WPS}, /* returns to previous music, WPS/FM */
    /* not really WPS specific, but no equivilant ACTION_STD_* */
    {"voldown", ACTION_WPS_VOLDOWN},    {"volup", ACTION_WPS_VOLUP},
    {"mute", ACTION_TOUCH_MUTE },

    /* generic settings changers */
    {"setting_inc", ACTION_SETTINGS_INC}, {"setting_dec", ACTION_SETTINGS_DEC},
    {"setting_set", ACTION_SETTINGS_SET},

    /* WPS specific actions */
    {"rwd", ACTION_WPS_SEEKBACK },      {"ffwd", ACTION_WPS_SEEKFWD },
    {"wps_prev", ACTION_WPS_SKIPPREV }, {"wps_next", ACTION_WPS_SKIPNEXT },
    {"browse", ACTION_WPS_BROWSE },
    {"play", ACTION_WPS_PLAY },         {"stop", ACTION_WPS_STOP },
    {"shuffle", ACTION_TOUCH_SHUFFLE }, {"repmode", ACTION_TOUCH_REPMODE },
    {"pitch", ACTION_WPS_PITCHSCREEN},  {"trackinfo", ACTION_WPS_ID3SCREEN },
    {"playlist", ACTION_WPS_VIEW_PLAYLIST },
    {"listbookmarks", ACTION_WPS_LIST_BOOKMARKS },
    {"createbookmark", ACTION_WPS_CREATE_BOOKMARK },

#if CONFIG_TUNER
    /* FM screen actions */
    /* Also allow browse, play, stop from WPS codes */
    {"mode", ACTION_FM_MODE },          {"record", ACTION_FM_RECORD },
    {"presets", ACTION_FM_PRESET},
#endif
};

static int touchregion_setup_setting(struct skin_element *element, int param_no,
                                     struct touchregion *region)
{
#ifndef __PCTOOL__
    int p = param_no;
    char *name = get_param_text(element, p++);
    int j;

    region->setting_data.setting = find_setting_by_cfgname(name, &j);
    if (region->setting_data.setting == NULL)
        return WPS_ERROR_INVALID_PARAM;

    if (region->action == ACTION_SETTINGS_SET)
    {
        char* text;
        int temp;
        struct touchsetting *setting = &region->setting_data;
        if (element->params_count < p+1)
            return -1;

        setting->using_text = false;
        text = get_param_text(element, p++);
        switch (settings[j].flags&F_T_MASK)
        {
        case F_T_CUSTOM:
            setting->using_text = true;
            setting->value.text.__data = text;
            break;
        case F_T_INT:
        case F_T_UINT:
            if (settings[j].cfg_vals == NULL)
            {
                setting->value.number = atoi(text);
            }
            else if (cfg_string_to_int(j, &temp, text))
            {
                if (settings[j].flags&F_TABLE_SETTING)
                    setting->value.number =
                        settings[j].table_setting->values[temp];
                else
                    setting->value.number = temp;
            }
            else
                return -1;
            break;
        case F_T_BOOL:
            if (cfg_string_to_int(j, &temp, text))
            {
                setting->value.number = temp;
            }
            else
                return -1;
            break;
        default:
            return -1;
        }
    }
    return p-param_no;
#endif /* __PCTOOL__ */
    return 0;
}

static void move_touchregion(struct skin_element *element, ptrdiff_t diff)
{
    struct wps_token *token = element->data;
    struct touchregion *data = token->value.data;
    
    SAFE_MOVE_BUFLIB_PTR(data->label, diff);
    SAFE_MOVE_BUFLIB_PTR(data->wvp, diff);
    SAFE_MOVE_BUFLIB_PTR(data->bar, diff);
    
    SAFE_MOVE_PTR_CHECK(data->setting_data.value.text.__data, diff, data->setting_data.using_text);
    
    SAFE_MOVE_PTR(token->value.data, diff);
}

static int parse_touchregion(struct skin_element *element,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    static const struct skin_tag_callbacks tag_callbacks = {
        .move = move_touchregion,
    };

    (void)token;
    unsigned i, imax;
    int p;
    struct touchregion *region = NULL;
    const char *action;
    const char pb_string[] = "progressbar";
    const char vol_string[] = "volume";

    /* format: %T([label,], x,y,width,height,action[, ...])
     * if action starts with & the area must be held to happen
     */


    region = skin_buffer_alloc(sizeof(*region));
    if (!region)
        return WPS_ERROR_INVALID_PARAM;

    /* should probably do some bounds checking here with the viewport... but later */
    region->action = ACTION_NONE;

    if (get_param(element, 0)->type == STRING)
    {
        region->label.__data = get_param_text(element, 0);
        p = 1;
        /* "[SI]III[SI]|SS" is the param list. There MUST be 4 numbers
         * followed by at least one string. Verify that here */
        if (element->params_count < 6 ||
            get_param(element, 4)->type != INTEGER)
            return WPS_ERROR_INVALID_PARAM;
    }
    else
    {
        region->label.__data = NULL;
        p = 0;
    }

    region->x = get_param(element, p++)->data.number;
    region->y = get_param(element, p++)->data.number;
    region->width = get_param(element, p++)->data.number;
    region->height = get_param(element, p++)->data.number;
    region->wvp.__data = curr_vp;
    region->armed = false;
    region->reverse_bar = false;
    region->value = 0;
    region->last_press = 0xffff;
    region->press_length = PRESS;
    region->allow_while_locked = false;
    region->bar.__data = NULL;
    action = get_param_text(element, p++);

    /* figure out the action */
    if(!strcmp(pb_string, action))
        region->action = ACTION_TOUCH_SCROLLBAR;
    else if(!strcmp(vol_string, action))
        region->action = ACTION_TOUCH_VOLUME;
    else
    {
        imax = ARRAYLEN(touchactions);
        for (i = 0; i < imax; i++)
        {
            /* try to match with one of our touchregion screens */
            if (!strcmp(touchactions[i].s, action))
            {
                region->action = touchactions[i].action;
                if (region->action == ACTION_SETTINGS_INC ||
                    region->action == ACTION_SETTINGS_DEC ||
                    region->action == ACTION_SETTINGS_SET)
                {
                    int val;
                    if (element->params_count < p+1)
                        return WPS_ERROR_INVALID_PARAM;
                    val = touchregion_setup_setting(element, p, region);
                    if (val < 0)
                        return WPS_ERROR_INVALID_PARAM;
                    p += val;
                }
                break;
            }
        }
        if (region->action == ACTION_NONE)
            return WPS_ERROR_INVALID_PARAM;
    }
    while (p < element->params_count)
    {
        char* param = get_param_text(element, p++);
        if (!strcmp(param, "allow_while_locked"))
            region->allow_while_locked = true;
        else if (!strcmp(param, "reverse_bar"))
            region->reverse_bar = true;
        else if (!strcmp(param, "repeat_press"))
            region->press_length = REPEAT;
        else if (!strcmp(param, "long_press"))
            region->press_length = LONG_PRESS;
    }
    struct skin_token_list *item = new_skin_token_list_item(NULL, region);
    if (!item)
        return WPS_ERROR_INVALID_PARAM;
    add_to_ll_chain(&wps_data->touchregions.__data, item);
    item->token.__data->callbacks = &tag_callbacks;

    if (region->action == ACTION_TOUCH_MUTE)
    {
        region->value = global_settings.volume;
    }


    return 0;
}
#endif

static bool check_feature_tag(const int type)
{
    switch (type)
    {
        case SKIN_TOKEN_RTC_PRESENT:
#if CONFIG_RTC
            return true;
#else
            return false;
#endif
        case SKIN_TOKEN_HAVE_RECORDING:
#ifdef HAVE_RECORDING
            return true;
#else
            return false;
#endif
        case SKIN_TOKEN_HAVE_TUNER:
#if CONFIG_TUNER
            if (radio_hardware_present())
                return true;
#endif
            return false;
        case SKIN_TOKEN_HAVE_TOUCH:
#ifdef HAVE_TOUCHSCREEN
            return true;
#else
            return false;
#endif

#if CONFIG_TUNER
        case SKIN_TOKEN_HAVE_RDS:
#ifdef HAVE_RDS_CAP
            return true;
#else
            return false;
#endif /* HAVE_RDS_CAP */
#endif /* CONFIG_TUNER */
        default: /* not a tag we care about, just don't skip */
            return true;
    }
}

/* This is used to free any buflib allocations before the rest of
 * wps_data is reset.
 * The call to this in settings_apply_skins() is the last chance to do
 * any core_free()'s before wps_data is trashed and those handles lost
 */
void skin_data_free_buflib_allocs(struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP
#ifndef __PCTOOL__
    struct skin_token_list *list = wps_data->images.__data;
    int *font_ids = wps_data->font_ids.__data;
    while (list)
    {
        struct wps_token *token = list->token.__data;
        struct gui_img *img = (struct gui_img*)token->value.data;
        if (img->buflib_handle > 0)
        {
            struct skin_token_list *imglist = list->next.__data;
            core_free(img->buflib_handle);

            while (imglist)
            {
                struct wps_token *freetoken = imglist->token.__data;
                struct gui_img *freeimg = (struct gui_img*)freetoken->value.data;
                if (img->buflib_handle == freeimg->buflib_handle)
                    freeimg->buflib_handle = -1;
                imglist = imglist->next.__data;
            }
        }
        list = list->next.__data;
    }
    wps_data->images.__data = NULL;
    if (font_ids != NULL)
    {
        while (wps_data->font_count > 0)
            font_unload(font_ids[--wps_data->font_count]);
    }
    wps_data->font_ids.__data = NULL;
    if (wps_data->buflib_handle > 0)
        core_free(wps_data->buflib_handle);
    wps_data->buflib_handle = -1;
#endif
#endif
}

/*
 * initial setup of wps_data; does reset everything
 * except fields which need to survive, i.e.
 * Also called if the load fails
 **/
static void skin_data_reset(struct wps_data *wps_data)
{
    skin_data_free_buflib_allocs(wps_data);
#ifdef HAVE_LCD_BITMAP
    wps_data->images.__data = NULL;
#endif
    wps_data->tree = NULL;
#ifdef HAVE_BACKDROP_IMAGE
    if (wps_data->backdrop_id >= 0)
        skin_backdrop_unload(wps_data->backdrop_id);
    backdrop_filename = NULL;
#endif
#ifdef HAVE_TOUCHSCREEN
    wps_data->touchregions.__data = NULL;
#endif
#ifdef HAVE_SKIN_VARIABLES
    wps_data->skinvars.__data = NULL;
#endif
#ifdef HAVE_ALBUMART
    wps_data->albumart.__data = NULL;
    if (wps_data->playback_aa_slot >= 0)
    {
        playback_release_aa_slot(wps_data->playback_aa_slot);
        wps_data->playback_aa_slot = -1;
    }
#endif

#ifdef HAVE_LCD_BITMAP
    wps_data->peak_meter_enabled = false;
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
#else /* HAVE_LCD_CHARCELLS */
    /* progress bars */
    int i;
    for (i = 0; i < 8; i++)
    {
        wps_data->wps_progress_pat[i] = 0;
    }
    wps_data->full_line_progressbar = false;
#endif
    wps_data->wps_loaded = false;
}

#ifdef HAVE_LCD_BITMAP
#ifndef __PCTOOL__

void movepointers_callback(struct skin_element *element, void *data)
{
    struct movepointersdata *movedata = data;
    int i;

    for (i = 0; i < element->children_count; i++)
        SAFE_MOVE_PTR(element->children[i], movedata->diff);
    SAFE_MOVE_PTR(element->children, movedata->diff);

    for (i = 0; i < element->params_count; i++)
    {
        if (element->params[i].type == STRING)
        {
            SAFE_MOVE_PTR(element->params[i].data.text, movedata->diff);
        }
        else if (element->params[i].type == CODE)
        {
            movepointers_callback(element->params[i].data.code, movedata);
            SAFE_MOVE_PTR(element->params[i].data.code, movedata->diff);
        }
    }
    SAFE_MOVE_PTR(element->params, movedata->diff);

    switch (element->type)
    {
        case TAG:
        {
            struct wps_token *token = element->data;
            if (token->callbacks)
            {
                if (token->callbacks->move)
                    token->callbacks->move(element, movedata->diff);
            }
        }
        break;
        case VIEWPORT:
        {
            struct skin_viewport *skinvp = element->data;
            skinvp->callbacks->move(element, movedata->diff);
        }
        break;
        case LINE:
        case LINE_ALTERNATOR:
        case TEXT:
            break;
        case CONDITIONAL:
        {
            struct conditional *cond = element->data;
            struct wps_token *token = cond->token.__data;
            if (token->callbacks)
            {
                struct skin_element temp;
                temp.data = token;
                if (token->callbacks->move)
                    token->callbacks->move(&temp, movedata->diff);
            }
            SAFE_MOVE_BUFLIB_PTR(cond->token, movedata->diff);
        }
        break;
        case UNKNOWN:
        case COMMENT:
            break;
    }

    SAFE_MOVE_PTR(element->next, movedata->diff);
    SAFE_MOVE_PTR(element->data, movedata->diff);
}

void move_token_list_items(struct skin_token_list **list, ptrdiff_t diff)
{
    struct skin_token_list *old, *item = *list;
    struct skin_element element;
    while (item)
    {
        old = item;
        item = item->next.__data;
        if (old->token.__data->callbacks && old->token.__data->callbacks->move)
        {
            element.data = old->token.__data;
            old->token.__data->callbacks->move(&element, diff);
        }
        SAFE_MOVE_BUFLIB_PTR(old->next, diff);
        SAFE_MOVE_BUFLIB_PTR(old->token, diff);
    }
    
    SAFE_MOVE_PTR(*list, diff);
}

static void move_all_lists(struct wps_data *data, ptrdiff_t diff)
{
    move_token_list_items(&data->images.__data, diff);
#ifdef HAVE_TOUCHSCREEN
    move_token_list_items(&data->touchregions.__data, diff);
#endif
#ifdef HAVE_SKIN_VARIABLES
    move_token_list_items(&data->skinvars.__data, diff);
#endif
}

struct wps_data* skin_from_buflib_handle(int handle);
static int currently_loading_handle = -1;
static int buflib_move_callback(int handle, void* current, void* new)
{
    struct wps_data *wps_data = skin_from_buflib_handle(handle);
    struct movepointersdata data = { .diff = new - current};

    if (handle == currently_loading_handle)
        return BUFLIB_CB_CANNOT_MOVE;
        
    skin_traverse_tree(current, movepointers_callback, &data);
    move_all_lists(wps_data, data.diff);
#ifdef HAVE_ALBUMART
    SAFE_MOVE_BUFLIB_PTR(wps_data->albumart, data.diff);
#endif
    SAFE_MOVE_BUFLIB_PTR(wps_data->font_ids, data.diff);
    wps_data->tree = new;
    
    return BUFLIB_CB_OK;
}
static struct buflib_callbacks buflib_ops = {buflib_move_callback, NULL, NULL};
static void lock_handle(int handle)
{
    currently_loading_handle = handle;
}
static void unlock_handle(void)
{
    currently_loading_handle = -1;
}
#endif

static int load_skin_bmp(struct wps_data *wps_data, struct bitmap *bitmap, char* bmpdir)
{
    (void)wps_data; /* only needed for remote targets */
    char img_path[MAX_PATH];
    int fd;
    int handle;
    get_image_filename(bitmap->data, bmpdir,
                       img_path, sizeof(img_path));

    /* load the image */
    int format;
#ifdef HAVE_REMOTE_LCD
    if (curr_screen == SCREEN_REMOTE)
        format = FORMAT_ANY|FORMAT_REMOTE;
    else
#endif
        format = FORMAT_ANY|FORMAT_TRANSPARENT;

    fd = open(img_path, O_RDONLY);
    if (fd < 0)
    {
        DEBUGF("Couldn't open %s\n", img_path);
        return fd;
    }
#ifndef __PCTOOL__
    size_t buf_size = read_bmp_fd(fd, bitmap, 0,
                                    format|FORMAT_RETURN_SIZE, NULL);
    handle = core_alloc_ex(bitmap->data, buf_size, &buflib_ops);
    if (handle <= 0)
    {
        DEBUGF("Not enough skin buffer: need %zd more.\n",
                buf_size - skin_buffer_freespace());
        close(fd);
        return handle;
    }
    _stats->buflib_handles++;
    _stats->images_size += buf_size;
    lseek(fd, 0, SEEK_SET);
    lock_handle(handle);
    bitmap->data = core_get_data(handle);
    int ret = read_bmp_fd(fd, bitmap, buf_size, format, NULL);
    bitmap->data = NULL; /* do this to force a crash later if the
                            caller doesnt call core_get_data() */
    unlock_handle();
    close(fd);
    if (ret > 0)
    {
        /* free unused alpha channel, if any */
        core_shrink(handle, core_get_data(handle), ret);
        return handle;
    }
    else
    {
        /* Abort if we can't load an image */
        DEBUGF("Couldn't load '%s'\n", img_path);
        core_free(handle);
        return -1;
    }
#else /* !__PCTOOL__ */
    close(fd);
    return 1;
#endif
}

static bool load_skin_bitmaps(struct wps_data *wps_data, char *bmpdir)
{
    struct skin_token_list *list;
    bool retval = true; /* return false if a single image failed to load */

    /* regular images */
    list = wps_data->images.__data;
    while (list)
    {
        struct wps_token *token = list->token.__data;
        struct gui_img *img = (struct gui_img*)token->value.data;
        if (!img->loaded)
        {
            if (img->using_preloaded_icons)
            {
                img->loaded = true;
                token->type = SKIN_TOKEN_IMAGE_DISPLAY_LISTICON;
            }
            else
            {
                char path[MAX_PATH];
                int handle;
                strcpy(path, img->bm.data);
                handle = load_skin_bmp(wps_data, &img->bm, bmpdir);
                img->buflib_handle = handle;
                img->loaded = img->buflib_handle >= 0;
                if (img->loaded)
                {
                    struct skin_token_list *imglist = list->next.__data;

                    img->subimage_height = img->bm.height / img->num_subimages;
                    while (imglist)
                    {
                        token = imglist->token.__data;
                        img = (struct gui_img*)token->value.data;
                        if (!strcmp(path, img->bm.data))
                        {
                            img->loaded = true;
                            img->buflib_handle = handle;
                            img->subimage_height = img->bm.height / img->num_subimages;
                        }
                        imglist = imglist->next.__data;
                    }
                }
                else
                    retval = false;
            }
        }
        list = list->next.__data;
    }

#ifdef HAVE_BACKDROP_IMAGE
    wps_data->backdrop_id = skin_backdrop_assign(backdrop_filename, bmpdir, curr_screen);
#endif /* has backdrop support */
    return retval;
}

static bool skin_load_fonts(struct wps_data *data)
{
    /* don't spit out after the first failue to aid debugging */
    int id_array[MAXUSERFONTS];
    int font_count = 0;
    bool success = true;
    struct skin_element *vp_list;
    int font_id;
    /* walk though each viewport and assign its font */
    for(vp_list = data->tree;
        vp_list; vp_list = vp_list->next)
    {
        /* first, find the viewports that have a non-sys/ui-font font */
        struct skin_viewport *skin_vp =
                vp_list->data;
        struct viewport *vp = &skin_vp->vp;

        font_id = skin_vp->parsed_fontid;
        if (font_id == 1)
        {   /* the usual case -> built-in fonts */
            vp->font = screens[curr_screen].getuifont();
            continue;
        }
        else if (font_id <= 0)
        {
            vp->font = FONT_SYSFIXED;
            continue;
        }

        /* now find the corresponding skin_font */
        struct skin_font *font = &skinfonts[font_id-2];
        if (!font->name)
        {
            if (success)
            {
                DEBUGF("font %d not specified\n", font_id);
            }
            success = false;
            continue;
        }

        /* load the font - will handle loading the same font again if
         * multiple viewports use the same */
        if (font->id < 0)
        {
            char path[MAX_PATH];
            snprintf(path, sizeof path, FONT_DIR "/%s", font->name);
#ifndef __PCTOOL__
            font->id = font_load_ex(path, 0, skinfonts[font_id-2].glyphs);

#else
                font->id = font_load(path);
#endif
            //printf("[%d] %s -> %d\n",font_id, font->name, font->id);
            id_array[font_count++] = font->id;
        }

        if (font->id < 0)
        {
            DEBUGF("Unable to load font %d: '%s'\n", font_id, font->name);
            font->name = NULL; /* to stop trying to load it again if we fail */
            success = false;
            continue;
        }

        /* finally, assign the font_id to the viewport */
        vp->font = font->id;
    }
    if (font_count)
    {
        int *font_ids = skin_buffer_alloc(font_count * sizeof(int));
        if (!success || font_ids == NULL)
        {
            while (font_count > 0)
            {
                if(id_array[--font_count] != -1)
                    font_unload(id_array[font_count]);
            }
            data->font_ids.__data = NULL;
            return false;
        }
        memcpy(font_ids, id_array, sizeof(int)*font_count);
        data->font_count = font_count;
        data->font_ids.__data = font_ids;
    }
    else
    {
        data->font_count = 0;
        data->font_ids.__data = NULL;
    }
    return success;
}

#endif /* HAVE_LCD_BITMAP */

static void move_viewport(struct skin_element *element, ptrdiff_t diff)
{
    struct skin_viewport *skin_vp = element->data;
    
    skin_vp->display->scroll_stop_viewport(&skin_vp->vp);
    
    SAFE_MOVE_BUFLIB_PTR_CHECK(skin_vp->label, diff, skin_vp->label.__data != VP_DEFAULT_LABEL);
}

static int parse_viewport(struct skin_element* element, struct wps_data *data)
{
    
    static const struct skin_tag_callbacks tag_callbacks =
    {
        .type = SKIN_TOKEN_VIEWPORT_LOAD, 
        .parse = parse_viewport, 
        .move = move_viewport
    };

    struct skin_viewport *skin_vp = skin_buffer_alloc(sizeof(*skin_vp));
    struct screen *display = &screens[curr_screen];

    if (!skin_vp)
        return CALLBACK_ERROR;

    skin_vp->hidden_flags = 0;
    skin_vp->display = display;
    skin_vp->label.__data = NULL;
    skin_vp->is_infovp = false;
    skin_vp->parsed_fontid = 1;
    skin_vp->callbacks = &tag_callbacks;
    element->data = skin_vp;
    curr_vp = skin_vp;
    curr_viewport_element = element;
    if (!first_viewport)
        first_viewport = element;

    viewport_set_defaults(&skin_vp->vp, curr_screen);

#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
    skin_vp->output_to_backdrop_buffer = false;
#endif
#ifdef HAVE_LCD_COLOR
    skin_vp->start_gradient.start = global_settings.lss_color;
    skin_vp->start_gradient.end = global_settings.lse_color;
    skin_vp->start_gradient.text = global_settings.lst_color;
#endif


    struct skin_tag_parameter *param = get_param(element, 0);
    if (element->params_count == 0) /* default viewport */
    {
        if (!IS_VALID_OFFSET(data->tree)) /* first viewport in the skin */
            data->tree = element;
        skin_vp->label.__data = VP_DEFAULT_LABEL;
        return CALLBACK_OK;
    }

    if (element->params_count == 6)
    {
        if (element->tag->type == SKIN_TOKEN_UIVIEWPORT_LOAD)
        {
            skin_vp->is_infovp = true;
            if (isdefault(param))
            {
                skin_vp->hidden_flags = VP_NEVER_VISIBLE;
                skin_vp->label.__data = VP_DEFAULT_LABEL;
            }
            else
            {
                skin_vp->hidden_flags = VP_NEVER_VISIBLE;
                skin_vp->label.__data = param->data.text;
            }
        }
        else
        {
                skin_vp->hidden_flags = VP_DRAW_HIDEABLE|VP_DRAW_HIDDEN;
                skin_vp->label.__data = param->data.text;
        }
        param++;
    }
    /* x */
    if (!isdefault(param))
    {
        skin_vp->vp.x = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.x += display->lcdwidth;
        else if (param->type == PERCENT)
            skin_vp->vp.x = param->data.number * display->lcdwidth / 1000;
    }
    param++;
    /* y */
    if (!isdefault(param))
    {
        skin_vp->vp.y = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.y += display->lcdheight;
        else if (param->type == PERCENT)
            skin_vp->vp.y = param->data.number * display->lcdheight / 1000;
    }
    param++;
    /* width */
    if (!isdefault(param))
    {
        skin_vp->vp.width = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.width = (skin_vp->vp.width + display->lcdwidth) - skin_vp->vp.x;
        else if (param->type == PERCENT)
            skin_vp->vp.width = param->data.number * display->lcdwidth / 1000;
    }
    else
    {
        skin_vp->vp.width = display->lcdwidth - skin_vp->vp.x;
    }
    param++;
    /* height */
    if (!isdefault(param))
    {
        skin_vp->vp.height = param->data.number;
        if (param->data.number < 0)
            skin_vp->vp.height = (skin_vp->vp.height + display->lcdheight) - skin_vp->vp.y;
        else if (param->type == PERCENT)
            skin_vp->vp.height = param->data.number * display->lcdheight / 1000;
    }
    else
    {
        skin_vp->vp.height = display->lcdheight - skin_vp->vp.y;
    }
    param++;
#ifdef HAVE_LCD_BITMAP
    /* font */
    if (!isdefault(param))
        skin_vp->parsed_fontid = param->data.number;
#endif
    if ((unsigned) skin_vp->vp.x >= (unsigned) display->lcdwidth ||
        skin_vp->vp.width + skin_vp->vp.x > display->lcdwidth ||
        (unsigned) skin_vp->vp.y >= (unsigned) display->lcdheight ||
        skin_vp->vp.height + skin_vp->vp.y > display->lcdheight)
        return CALLBACK_ERROR;

    /* Fix x position for RTL languages */
    if (follow_lang_direction && lang_is_rtl())
        skin_vp->vp.x = display->lcdwidth - skin_vp->vp.x - skin_vp->vp.width;

    return CALLBACK_OK;
}

static int skin_element_callback(struct skin_element* element, void* data)
{
    struct wps_data *wps_data = (struct wps_data *)data;
    struct wps_token *token;
    parse_function function = NULL;

    switch (element->type)
    {
        /* IMPORTANT: element params are shared, so copy them if needed
         *            or use then NOW, dont presume they have a long lifespan
         */
        case TAG:
        {
            token = skin_buffer_alloc(sizeof(*token));
            memset(token, 0, sizeof(*token));
            token->type = element->tag->type;
            token->value.data = NULL;

            if (element->tag->flags&SKIN_RTC_REFRESH)
            {
#if CONFIG_RTC
                curr_line->update_mode |= SKIN_REFRESH_DYNAMIC;
#else
                curr_line->update_mode |= SKIN_REFRESH_STATIC;
#endif
            }
            else
                curr_line->update_mode |= element->tag->flags&SKIN_REFRESH_ALL;

            element->data = token;

            /* Some tags need special handling for the tag, so add them here */
            switch (token->type)
            {
                case SKIN_TOKEN_ALIGN_LANGDIRECTION:
                    follow_lang_direction = 2;
                    break;
                case SKIN_TOKEN_LOGICAL_IF:
                    function = parse_logical_if;
                    break;
                case SKIN_TOKEN_LOGICAL_AND:
                case SKIN_TOKEN_LOGICAL_OR:
                    function = parse_logical_andor;
                    break;
                case SKIN_TOKEN_SUBSTRING:
                    function = parse_substring_tag;
                    break;
                case SKIN_TOKEN_PROGRESSBAR:
                case SKIN_TOKEN_VOLUME:
                case SKIN_TOKEN_BATTERY_PERCENT:
                case SKIN_TOKEN_PLAYER_PROGRESSBAR:
                case SKIN_TOKEN_PEAKMETER_LEFT:
                case SKIN_TOKEN_PEAKMETER_RIGHT:
                case SKIN_TOKEN_LIST_NEEDS_SCROLLBAR:
#ifdef HAVE_RADIO_RSSI
                case SKIN_TOKEN_TUNER_RSSI:
#endif
                    function = parse_progressbar_tag;
                    break;
                case SKIN_TOKEN_SUBLINE_TIMEOUT:
                case SKIN_TOKEN_BUTTON_VOLUME:
                case SKIN_TOKEN_TRACK_STARTING:
                case SKIN_TOKEN_TRACK_ENDING:
                    function = parse_timeout_tag;
                    break;
#ifdef HAVE_LCD_BITMAP
                case SKIN_TOKEN_LIST_ITEM_TEXT:
                case SKIN_TOKEN_LIST_ITEM_ICON:
                    function = parse_listitem;
                    break;
                case SKIN_TOKEN_DISABLE_THEME:
                case SKIN_TOKEN_ENABLE_THEME:
                case SKIN_TOKEN_DRAW_INBUILTBAR:
                    function = parse_statusbar_tags;
                    break;
                case SKIN_TOKEN_LIST_TITLE_TEXT:
#ifndef __PCTOOL__
                    sb_skin_has_title(curr_screen);
#endif
                    break;
#endif
#if (LCD_DEPTH > 1) || (defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1))
                case SKIN_TOKEN_DRAWRECTANGLE:
                    function = parse_drawrectangle;
                    break;
#endif
                case SKIN_TOKEN_FILE_DIRECTORY:
                    token->value.i = get_param(element, 0)->data.number;
                    break;
#ifdef HAVE_BACKDROP_IMAGE
                case SKIN_TOKEN_VIEWPORT_FGCOLOUR:
                case SKIN_TOKEN_VIEWPORT_BGCOLOUR:
                    function = parse_viewportcolour;
                    break;
                case SKIN_TOKEN_IMAGE_BACKDROP:
                    function = parse_image_special;
                    break;
                case SKIN_TOKEN_VIEWPORT_TEXTSTYLE:
                    function = parse_viewporttextstyle;
                    break;
                case SKIN_TOKEN_VIEWPORT_DRAWONBG:
                    curr_vp->output_to_backdrop_buffer = true;
                    backdrop_filename = BACKDROP_BUFFERNAME;
                    wps_data->use_extra_framebuffer = true;
                    break;
#endif
#ifdef HAVE_LCD_COLOR
                case SKIN_TOKEN_VIEWPORT_GRADIENT_SETUP:
                    function = parse_viewport_gradient_setup;
                    break;
#endif
                case SKIN_TOKEN_TRANSLATEDSTRING:
                case SKIN_TOKEN_SETTING:
                    function = parse_setting_and_lang;
                    break;
#ifdef HAVE_LCD_BITMAP
                case SKIN_TOKEN_VIEWPORT_CUSTOMLIST:
                    function = parse_playlistview;
                    break;
                case SKIN_TOKEN_LOAD_FONT:
                    function = parse_font_load;
                    break;
                case SKIN_TOKEN_VIEWPORT_ENABLE:
                case SKIN_TOKEN_UIVIEWPORT_ENABLE:
                    token->callbacks = &move_token_data_tag_callbacks;
                    token->value.data = get_param(element, 0)->data.text;
                    break;
                case SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY:
                case SKIN_TOKEN_IMAGE_DISPLAY_9SEGMENT:
                    function = parse_image_display;
                    break;
                case SKIN_TOKEN_IMAGE_PRELOAD:
                case SKIN_TOKEN_IMAGE_DISPLAY:
                    function = parse_image_load;
                    break;
                case SKIN_TOKEN_LIST_ITEM_CFG:
                    function = parse_listitemviewport;
                    break;
#endif
#ifdef HAVE_TOUCHSCREEN
                case SKIN_TOKEN_TOUCHREGION:
                    function = parse_touchregion;
                    break;
                case SKIN_TOKEN_LASTTOUCH:
                    function = parse_lasttouch;
                    break;
#endif
#ifdef HAVE_ALBUMART
                case SKIN_TOKEN_ALBUMART_DISPLAY:
                    if (wps_data->albumart.__data)
                    {
                        struct skin_albumart *aa = wps_data->albumart.__data;
                        aa->vp.__data = &curr_vp->vp;
                    }
                    break;
                case SKIN_TOKEN_ALBUMART_LOAD:
                    function = parse_albumart_load;
                    break;
#endif
#ifdef HAVE_SKIN_VARIABLES
                case SKIN_TOKEN_VAR_SET:
                case SKIN_TOKEN_VAR_GETVAL:
                case SKIN_TOKEN_VAR_TIMEOUT:
                    function = parse_skinvar;
                    break;
#endif
                default:
                    break;
            }
            if (function)
            {
                if (function(element, token, wps_data) != 0)
                    return CALLBACK_ERROR;
            }
            /* tags that start with 'F', 'I' or 'D' are for the next file */
            if ( *(element->tag->name) == 'I' || *(element->tag->name) == 'F' ||
                 *(element->tag->name) == 'D')
                token->next = true;
            if (follow_lang_direction > 0 )
                follow_lang_direction--;
            break;
        }
        case VIEWPORT:
            return parse_viewport(element, wps_data);
        case LINE:
        {
            curr_line = skin_buffer_alloc(sizeof(*curr_line));
            curr_line->update_mode = SKIN_REFRESH_STATIC;
            element->data = curr_line;
        }
        break;
        case LINE_ALTERNATOR:
        {
            struct line_alternator *alternator = skin_buffer_alloc(sizeof(*alternator));
            alternator->current_line = 0;
#ifndef __PCTOOL__
            alternator->next_change_tick = current_tick;
#endif
            element->data = alternator;
        }
        break;
        case CONDITIONAL:
        {
            struct conditional *conditional = skin_buffer_alloc(sizeof(*conditional));
            conditional->last_value = -1;
            conditional->token.__data = element->data;
            element->data = conditional;
            if (!check_feature_tag(element->tag->type))
            {
                return FEATURE_NOT_AVAILABLE;
            }
            return CALLBACK_OK;
        }
        case TEXT:
            curr_line->update_mode |= SKIN_REFRESH_STATIC;
            break;
        default:
            break;
    }
    return CALLBACK_OK;
}

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool skin_data_load(enum screen_type screen, struct wps_data *wps_data,
                    const char *buf, bool isfile, struct skin_stats *stats)
{
    char *wps_buffer = NULL;
    if (!wps_data || !buf)
        return false;
#ifdef HAVE_LCD_BITMAP
    int i;
    for (i=0;i<MAXUSERFONTS;i++)
    {
        skinfonts[i].id = -1;
        skinfonts[i].name = NULL;
    }
#endif
#ifdef DEBUG_SKIN_ENGINE
    if (isfile && debug_wps)
    {
        DEBUGF("\n=====================\nLoading '%s'\n=====================\n", buf);
    }
#endif

    _stats = stats;
    skin_clear_stats(stats);
    /* get buffer space from the plugin buffer */
    size_t buffersize = 0;
    wps_buffer = (char *)plugin_get_buffer(&buffersize);

    if (!wps_buffer)
        return false;

    skin_data_reset(wps_data);
    wps_data->wps_loaded = false;
    curr_screen = screen;
    curr_line = NULL;
    curr_vp = NULL;
    curr_viewport_element = NULL;
    first_viewport = NULL;

    if (isfile)
    {
        int fd = open_utf8(buf, O_RDONLY);

        if (fd < 0)
            return false;
        /* copy the file's content to the buffer for parsing,
           ensuring that every line ends with a newline char. */
        unsigned int start = 0;
        while(read_line(fd, wps_buffer + start, buffersize - start) > 0)
        {
            start += strlen(wps_buffer + start);
            if (start < buffersize - 1)
            {
                wps_buffer[start++] = '\n';
                wps_buffer[start] = 0;
            }
        }
        close(fd);
        if (start <= 0)
            return false;
        start++;
        skin_buffer = &wps_buffer[start];
        buffersize -= start;
    }
    else
    {
        skin_buffer = wps_buffer;
        wps_buffer = (char*)buf;
    }
    skin_buffer = (void *)(((unsigned long)skin_buffer + 3) & ~3);
    buffersize -= 3;
#ifdef HAVE_BACKDROP_IMAGE
    backdrop_filename = "-";
    wps_data->backdrop_id = -1;
#endif
    /* parse the skin source */
    skin_buffer_init(skin_buffer, buffersize);
    struct skin_element *tree = skin_parse(wps_buffer, skin_element_callback, wps_data);
    wps_data->tree = tree;
    if (!wps_data->tree) {
#ifdef DEBUG_SKIN_ENGINE
        if (isfile && debug_wps)
            skin_error_format_message();
#endif
        skin_data_reset(wps_data);
        return false;
    }

#ifdef HAVE_LCD_BITMAP
    char bmpdir[MAX_PATH];
    if (isfile)
    {
        /* get the bitmap dir */
        char *dot = strrchr(buf, '.');
        strlcpy(bmpdir, buf, dot - buf + 1);
    }
    else
    {
        snprintf(bmpdir, MAX_PATH, "%s", BACKDROP_DIR);
    }
    /* load the bitmaps that were found by the parsing */
    if (!load_skin_bitmaps(wps_data, bmpdir) ||
        !skin_load_fonts(wps_data))
    {
        skin_data_reset(wps_data);
        return false;
    }
#endif
#if defined(HAVE_ALBUMART) && !defined(__PCTOOL__)
    int status = audio_status();
    if (status & AUDIO_STATUS_PLAY)
    {
        /* last_albumart_{width,height} is either both 0 or valid AA dimensions */
        struct skin_albumart *aa = wps_data->albumart.__data;
        if (aa && (aa->state != WPS_ALBUMART_NONE ||
            (((wps_data->last_albumart_height != aa->height) ||
            (wps_data->last_albumart_width != aa->width)))))
        {
            struct mp3entry *id3 = audio_current_track();
            unsigned long elapsed = id3->elapsed;
            unsigned long offset = id3->offset;
            audio_stop();
            if (!(status & AUDIO_STATUS_PAUSE))
                audio_play(elapsed, offset);
        }
    }
#endif
#ifndef __PCTOOL__
    wps_data->buflib_handle = core_alloc(isfile ? buf : "failsafe skin",
                                            skin_buffer_usage());
    if (wps_data->buflib_handle > 0)
    {
        void *dest = core_get_data(wps_data->buflib_handle);
        buflib_move_callback(wps_data->buflib_handle, tree, dest);
        wps_data->wps_loaded = true;
        memcpy(dest, skin_buffer, skin_buffer_usage());
        // SAFETY check
        memset(skin_buffer, 0xff, skin_buffer_usage());
        stats->buflib_handles++;
        stats->tree_size = skin_buffer_usage();
    }
#else
    wps_data->wps_loaded = wps_data->tree != NULL;
#endif

#ifdef HAVE_TOUCHSCREEN
    /* Check if there are any touch regions from the skin and not just
     * auto-created ones for bars */
    struct skin_token_list *regions = wps_data->touchregions.__data;
    bool user_touch_region_found = false;
    while (regions)
    {
        struct wps_token *token = regions->token.__data;
        struct touchregion *r = token->value.data;

        if (r->action != ACTION_TOUCH_SCROLLBAR &&
            r->action != ACTION_TOUCH_VOLUME)
        {
            user_touch_region_found = true;
            break;
        }
        regions = regions->next.__data;
    }
    regions = wps_data->touchregions.__data;
    if (regions && !user_touch_region_found)
        wps_data->touchregions.__data = NULL;
#endif

    skin_buffer = NULL;
    return true;
}
