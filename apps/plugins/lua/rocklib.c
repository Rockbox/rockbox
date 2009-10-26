/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
 * Copyright (C) 2009 Maurus Cuelenaere
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

#define lrocklib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "rocklib.h"
#include "lib/helper.h"

/*
 * http://www.lua.org/manual/5.1/manual.html#lua_CFunction
 *
 * In order to communicate properly with Lua, a C function must use the following protocol,
 * which defines the way parameters and results are passed: a C function receives its arguments
 * from Lua in its stack in direct order (the first argument is pushed first). To return values to Lua,
 * a C function just pushes them onto the stack, in direct order (the first result is pushed first),
 * and returns the number of results. Any other value in the stack below the results will be properly
 * discarded by Lua. Like a Lua function, a C function called by Lua can also return many results. 
 */


/*
 * -----------------------------
 *
 *  Rockbox Lua image wrapper
 *
 * -----------------------------
 */

#define ROCKLUA_IMAGE "rb.image"
struct rocklua_image
{
    int width;
    int height;
    fb_data *data;
    fb_data dummy[1][1];
};

static void rli_wrap(lua_State *L, fb_data *src, int width, int height)
{
    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, sizeof(struct rocklua_image));

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    a->width = width;
    a->height = height;
    a->data = src;
}

static fb_data* rli_alloc(lua_State *L, int width, int height)
{
    size_t nbytes = sizeof(struct rocklua_image) + ((width*height) - 1) * sizeof(fb_data);
    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    a->width = width;
    a->height = height;
    a->data = &a->dummy[0][0];

    return a->data;
}

static int rli_new(lua_State *L)
{
    int width = luaL_checkint(L, 1);
    int height = luaL_checkint(L, 2);

    rli_alloc(L, width, height);

    return 1;
}

static struct rocklua_image* rli_checktype(lua_State *L, int arg)
{
    void *ud = luaL_checkudata(L, arg, ROCKLUA_IMAGE);
    luaL_argcheck(L, ud != NULL, arg, "'" ROCKLUA_IMAGE "' expected");
    return (struct rocklua_image*) ud;
}

static int rli_width(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->width);
    return 1;
}

static int rli_height(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->height);
    return 1;
}

static fb_data* rli_element(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);

    luaL_argcheck(L, 1 <= x && x <= a->width, 2,
                    "index out of range");
    luaL_argcheck(L, 1 <= y && y <= a->height, 3,
                    "index out of range");

    /* return element address */
    return &a->data[a->width * (y - 1) + (x - 1)];
}

static int rli_set(lua_State *L)
{
    fb_data newvalue = (fb_data) luaL_checknumber(L, 4);
    *rli_element(L) = newvalue;
    return 0;
}

static int rli_get(lua_State *L)
{
    lua_pushnumber(L, *rli_element(L));
    return 1;
}

static int rli_tostring(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushfstring(L, ROCKLUA_IMAGE ": %dx%d", a->width, a->height);
    return 1;
}

static const struct luaL_reg rli_lib [] =
{
    {"__tostring", rli_tostring},
    {"set", rli_set},
    {"get", rli_get},
    {"width", rli_width},
    {"height", rli_height},
    {NULL, NULL}
};

static inline void rli_init(lua_State *L)
{
    luaL_newmetatable(L, ROCKLUA_IMAGE);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  /* pushes the metatable */
    lua_settable(L, -3);  /* metatable.__index = metatable */

    luaL_register(L, NULL, rli_lib);
}

/*
 * -----------------------------
 *
 *  Rockbox wrappers start here!
 *
 * -----------------------------
 */

#define RB_WRAP(M) static int rock_##M(lua_State *L)
#define SIMPLE_VOID_WRAPPER(func) RB_WRAP(func) { (void)L; func(); return 0; }

/* Helper function for opt_viewport */
static void check_tablevalue(lua_State *L, const char* key, int tablepos, void* res, bool unsigned_val)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    if(!lua_isnoneornil(L, -1))
    {
        if(unsigned_val)
            *(unsigned*)res = luaL_checkint(L, -1);
        else
            *(int*)res = luaL_checkint(L, -1);
    }

    lua_pop(L, 1); /* Pop the value off the stack */
}

static struct viewport* opt_viewport(lua_State *L, int narg, struct viewport* alt)
{
    if(lua_isnoneornil(L, narg))
        return alt;

    int tablepos = lua_gettop(L);
    struct viewport *vp;

    lua_getfield(L, tablepos, "vp");  /* get table['vp'] */
    if(lua_isnoneornil(L, -1))
    {
        lua_pop(L, 1); /* Pop nil off stack */

        vp = (struct viewport*) lua_newuserdata(L, sizeof(struct viewport)); /* Allocate memory and push it as udata on the stack */
        memset(vp, 0, sizeof(struct viewport)); /* Init viewport values to 0 */
        lua_setfield(L, tablepos, "vp"); /* table['vp'] = vp (pops value off the stack) */
    }
    else
    {
        vp = (struct viewport*) lua_touserdata(L, -1); /* Reuse viewport struct */
        lua_pop(L, 1); /* We don't need the value on stack */
    }

    luaL_checktype(L, narg, LUA_TTABLE);

    check_tablevalue(L, "x", tablepos, &vp->x, false);
    check_tablevalue(L, "y", tablepos, &vp->y, false);
    check_tablevalue(L, "width", tablepos, &vp->width, false);
    check_tablevalue(L, "height", tablepos, &vp->height, false);
#ifdef HAVE_LCD_BITMAP
    check_tablevalue(L, "font", tablepos, &vp->font, false);
    check_tablevalue(L, "drawmode", tablepos, &vp->drawmode, false);
#endif
#if LCD_DEPTH > 1
    check_tablevalue(L, "fg_pattern", tablepos, &vp->fg_pattern, true);
    check_tablevalue(L, "bg_pattern", tablepos, &vp->bg_pattern, true);
#ifdef HAVE_LCD_COLOR
    check_tablevalue(L, "lss_pattern", tablepos, &vp->lss_pattern, true);
    check_tablevalue(L, "lse_pattern", tablepos, &vp->lse_pattern, true);
    check_tablevalue(L, "lst_pattern", tablepos, &vp->lst_pattern, true);
#endif
#endif

    return vp;
}

RB_WRAP(set_viewport)
{
    struct viewport *vp = opt_viewport(L, 1, NULL);
    int screen = luaL_optint(L, 2, SCREEN_MAIN);
    rb->screens[screen]->set_viewport(vp);
    return 0;
}

RB_WRAP(clear_viewport)
{
    int screen = luaL_optint(L, 1, SCREEN_MAIN);
    rb->screens[screen]->clear_viewport();
    return 0;
}

#ifdef HAVE_LCD_BITMAP
RB_WRAP(lcd_framebuffer)
{
    rli_wrap(L, rb->lcd_framebuffer, LCD_WIDTH, LCD_HEIGHT);
    return 1;
}

RB_WRAP(lcd_mono_bitmap_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int src_x = luaL_checkint(L, 2);
    int src_y = luaL_checkint(L, 3);
    int stride = luaL_checkint(L, 4);
    int x = luaL_checkint(L, 5);
    int y = luaL_checkint(L, 6);
    int width = luaL_checkint(L, 7);
    int height = luaL_checkint(L, 8);
    int screen = luaL_optint(L, 9, SCREEN_MAIN);

    rb->screens[screen]->mono_bitmap_part((const unsigned char *)src->data, src_x, src_y, stride, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_mono_bitmap)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    int screen = luaL_optint(L, 6, SCREEN_MAIN);

    rb->screens[screen]->mono_bitmap((const unsigned char *)src->data, x, y, width, height);
    return 0;
}

#if LCD_DEPTH > 1
RB_WRAP(lcd_bitmap_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int src_x = luaL_checkint(L, 2);
    int src_y = luaL_checkint(L, 3);
    int stride = luaL_checkint(L, 4);
    int x = luaL_checkint(L, 5);
    int y = luaL_checkint(L, 6);
    int width = luaL_checkint(L, 7);
    int height = luaL_checkint(L, 8);
    int screen = luaL_optint(L, 9, SCREEN_MAIN);

    rb->screens[screen]->bitmap_part(src->data, src_x, src_y, stride, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_bitmap)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    int screen = luaL_optint(L, 6, SCREEN_MAIN);

    rb->screens[screen]->bitmap(src->data, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_get_backdrop)
{
    fb_data* backdrop = rb->lcd_get_backdrop();
    if(backdrop == NULL)
        return 0;
    else
    {
        rli_wrap(L, backdrop, LCD_WIDTH, LCD_HEIGHT);
        return 1;
    }
}
#endif /* LCD_DEPTH > 1 */

#if LCD_DEPTH == 16
RB_WRAP(lcd_bitmap_transparent_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int src_x = luaL_checkint(L, 2);
    int src_y = luaL_checkint(L, 3);
    int stride = luaL_checkint(L, 4);
    int x = luaL_checkint(L, 5);
    int y = luaL_checkint(L, 6);
    int width = luaL_checkint(L, 7);
    int height = luaL_checkint(L, 8);
    int screen = luaL_optint(L, 9, SCREEN_MAIN);

    rb->screens[screen]->transparent_bitmap_part(src->data, src_x, src_y, stride, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_bitmap_transparent)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);
    int screen = luaL_optint(L, 6, SCREEN_MAIN);

    rb->screens[screen]->transparent_bitmap(src->data, x, y, width, height);
    return 0;
}
#endif /* LCD_DEPTH == 16 */

#endif /* defined(LCD_BITMAP) */

RB_WRAP(current_tick)
{
    lua_pushinteger(L, *rb->current_tick);
    return 1;
}

#ifdef HAVE_TOUCHSCREEN
RB_WRAP(action_get_touchscreen_press)
{
    short x, y;
    int result = rb->action_get_touchscreen_press(&x, &y);

    lua_pushinteger(L, result);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 3;
}
#endif

RB_WRAP(kbd_input)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);

    const char *input = luaL_optstring(L, 1, NULL);
    char *buffer = luaL_prepbuffer(&b);

    if(input != NULL)
        rb->strlcpy(buffer, input, LUAL_BUFFERSIZE);
    else
        buffer[0] = '\0';

    if(!rb->kbd_input(buffer, LUAL_BUFFERSIZE))
    {
        luaL_addsize(&b, strlen(buffer));
        luaL_pushresult(&b);
    }
    else
        lua_pushnil(L);

    return 1;
}

#ifdef HAVE_TOUCHSCREEN
RB_WRAP(touchscreen_set_mode)
{
    enum touchscreen_mode mode = luaL_checkint(L, 1);
    rb->touchscreen_set_mode(mode);
    return 0;
}
#endif

RB_WRAP(font_getstringsize)
{
    const unsigned char* str = luaL_checkstring(L, 1);
    int fontnumber = luaL_checkint(L, 2);
    int w, h;

    int result = rb->font_getstringsize(str, &w, &h, fontnumber);
    lua_pushinteger(L, result);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);

    return 3;
}

#ifdef HAVE_LCD_COLOR
RB_WRAP(lcd_rgbpack)
{
    int r = luaL_checkint(L, 1);
    int g = luaL_checkint(L, 2);
    int b = luaL_checkint(L, 3);
    int result = LCD_RGBPACK(r, g, b);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(lcd_rgbunpack)
{
    int rgb = luaL_checkint(L, 1);
    lua_pushinteger(L, RGB_UNPACK_RED(rgb));
    lua_pushinteger(L, RGB_UNPACK_GREEN(rgb));
    lua_pushinteger(L, RGB_UNPACK_BLUE(rgb));
    return 3;
}
#endif

RB_WRAP(read_bmp_file)
{
    struct bitmap bm;
    const char* filename = luaL_checkstring(L, 1);
    bool dither = luaL_optboolean(L, 2, true);
    bool transparent = luaL_optboolean(L, 3, false);
    int format = FORMAT_NATIVE;

    if(dither)
        format |= FORMAT_DITHER;

    if(transparent)
        format |= FORMAT_TRANSPARENT;

    int result = rb->read_bmp_file(filename, &bm, 0, format | FORMAT_RETURN_SIZE, NULL);

    if(result > 0)
    {
        bm.data = (unsigned char*) rli_alloc(L, bm.width, bm.height);
        rb->read_bmp_file(filename, &bm, result, format, NULL);

        return 1;
    }

    return 0;
}

RB_WRAP(current_path)
{
    const char *current_path = get_current_path(L, 1);
    if(current_path != NULL)
    {
        lua_pushstring(L, current_path);
        return 1;
    }
    else
        return 0;
}

static void fill_text_message(lua_State *L, struct text_message * message,
                              int pos)
{
    int i;
    luaL_checktype(L, pos, LUA_TTABLE);
    int n = luaL_getn(L, pos);
    const char **lines = (const char**) dlmalloc(n * sizeof(const char*));
    if(lines == NULL)
        luaL_error(L, "Can't allocate %d bytes!", n * sizeof(const char*));
    for(i=1; i<=n; i++)
    {
        lua_rawgeti(L, pos, i);
        lines[i-1] = luaL_checkstring(L, -1);
        lua_pop(L, 1);
    }
    message->message_lines = lines;
    message->nb_lines = n;
}

RB_WRAP(gui_syncyesno_run)
{
    struct text_message main_message, yes_message, no_message;
    struct text_message *yes = NULL, *no = NULL;

    fill_text_message(L, &main_message, 1);
    if(!lua_isnoneornil(L, 2))
        fill_text_message(L, (yes = &yes_message), 2);
    if(!lua_isnoneornil(L, 3))
        fill_text_message(L, (no = &no_message), 3);

    enum yesno_res result = rb->gui_syncyesno_run(&main_message, yes, no);

    dlfree(main_message.message_lines);
    if(yes)
        dlfree(yes_message.message_lines);
    if(no)
        dlfree(no_message.message_lines);

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(do_menu)
{
    struct menu_callback_with_desc menu_desc = {NULL, NULL, Icon_NOICON};
    struct menu_item_ex menu = {MT_RETURN_ID | MENU_HAS_DESC, {.strings = NULL},
                                {.callback_and_desc = &menu_desc}};
    int i, n, start_selected;
    const char **items, *title;

    title = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    start_selected = luaL_optint(L, 3, 0);

    n = luaL_getn(L, 2);
    items = (const char**) dlmalloc(n * sizeof(const char*));
    if(items == NULL)
        luaL_error(L, "Can't allocate %d bytes!", n * sizeof(const char*));
    for(i=1; i<=n; i++)
    {
        lua_rawgeti(L, 2, i); /* Push item on the stack */
        items[i-1] = luaL_checkstring(L, -1);
        lua_pop(L, 1); /* Pop it */
    }

    menu.strings = items;
    menu.flags |= MENU_ITEM_COUNT(n);
    menu_desc.desc = (unsigned char*) title;

    int result = rb->do_menu(&menu, &start_selected, NULL, false);

    dlfree(items);

    lua_pushinteger(L, result);
    return 1;
}

SIMPLE_VOID_WRAPPER(backlight_force_on);
SIMPLE_VOID_WRAPPER(backlight_use_settings);
#ifdef HAVE_REMOTE_LCD
SIMPLE_VOID_WRAPPER(remote_backlight_force_on);
SIMPLE_VOID_WRAPPER(remote_backlight_use_settings);
#endif
#ifdef HAVE_BUTTON_LIGHT
SIMPLE_VOID_WRAPPER(buttonlight_force_on);
SIMPLE_VOID_WRAPPER(buttonlight_use_settings);
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
RB_WRAP(backlight_brightness_set)
{
    int brightness = luaL_checkint(L, 1);
    backlight_brightness_set(brightness);

    return 0;
}
SIMPLE_VOID_WRAPPER(backlight_brightness_use_setting);
#endif

#define R(NAME) {#NAME, rock_##NAME}
static const luaL_Reg rocklib[] =
{
    /* Graphics */
#ifdef HAVE_LCD_BITMAP
    R(lcd_framebuffer),
    R(lcd_mono_bitmap_part),
    R(lcd_mono_bitmap),
#if LCD_DEPTH > 1
    R(lcd_get_backdrop),
    R(lcd_bitmap_part),
    R(lcd_bitmap),
#endif
#if LCD_DEPTH == 16
    R(lcd_bitmap_transparent_part),
    R(lcd_bitmap_transparent),
#endif
#endif
#ifdef HAVE_LCD_COLOR
    R(lcd_rgbpack),
    R(lcd_rgbunpack),
#endif

    /* Kernel */
    R(current_tick),

    /* Buttons */
#ifdef HAVE_TOUCHSCREEN
    R(action_get_touchscreen_press),
    R(touchscreen_set_mode),
#endif
    R(kbd_input),

    R(font_getstringsize),
    R(read_bmp_file),
    R(set_viewport),
    R(clear_viewport),
    R(current_path),
    R(gui_syncyesno_run),
    R(do_menu),

    /* Backlight helper */
    R(backlight_force_on),
    R(backlight_use_settings),
#ifdef HAVE_REMOTE_LCD
    R(remote_backlight_force_on),
    R(remote_backlight_use_settings),
#endif
#ifdef HAVE_BUTTON_LIGHT
    R(buttonlight_force_on),
    R(buttonlight_use_settings),
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    R(backlight_brightness_set),
    R(backlight_brightness_use_setting),
#endif

    {"new_image", rli_new},

    {NULL, NULL}
};
#undef  R
extern const luaL_Reg rocklib_aux[];


#define RB_CONSTANT(x) lua_pushinteger(L, x); lua_setfield(L, -2, #x);
/*
 ** Open Rockbox library
 */
LUALIB_API int luaopen_rock(lua_State *L)
{
    luaL_register(L, LUA_ROCKLIBNAME, rocklib);
    luaL_register(L, LUA_ROCKLIBNAME, rocklib_aux);

    RB_CONSTANT(HZ);

    RB_CONSTANT(LCD_WIDTH);
    RB_CONSTANT(LCD_HEIGHT);

    RB_CONSTANT(FONT_SYSFIXED);
    RB_CONSTANT(FONT_UI);

#ifdef HAVE_TOUCHSCREEN
    RB_CONSTANT(TOUCHSCREEN_POINT);
    RB_CONSTANT(TOUCHSCREEN_BUTTON);
#endif

    RB_CONSTANT(SCREEN_MAIN);
#ifdef HAVE_REMOTE_LCD
    RB_CONSTANT(SCREEN_REMOTE);
#endif

    rli_init(L);

    return 1;
}
