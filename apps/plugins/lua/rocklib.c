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

/* Helper function for opt_viewport */
static void check_tablevalue(lua_State *L, const char* key, int tablepos, void* res, bool unsigned_val)
{
    lua_pushstring(L, key); /* Push the key on the stack */
    lua_gettable(L, tablepos); /* Find table[key] (pops key off the stack) */

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

    lua_pushliteral(L, "vp"); /* push 'vp' on the stack */
    struct viewport *vp = (struct viewport*)lua_newuserdata(L, sizeof(struct viewport)); /* allocate memory and push it as udata on the stack */
    memset(vp, 0, sizeof(struct viewport)); /* Init viewport values to 0 */
    lua_settable(L, tablepos); /* table['vp'] = vp (pops key & value off the stack) */

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
    check_tablevalue(L, "lst_pattern", tablepos, &vp->lse_pattern, true);
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
    int screen = luaL_optint(L, 2, SCREEN_MAIN);
    rb->screens[screen]->clear_viewport();
    return 0;
}

RB_WRAP(splash)
{
    int ticks = luaL_checkint(L, 1);
    const char *s = luaL_checkstring(L, 2);
    rb->splash(ticks, s);
    return 0;
}

RB_WRAP(lcd_update)
{
    (void)L;
    rb->lcd_update();
    return 0;
}

RB_WRAP(lcd_update_rect)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);
    rb->lcd_update_rect(x, y, width, height);
    return 0;
}

RB_WRAP(lcd_clear_display)
{
    (void)L;
    rb->lcd_clear_display();
    return 0;
}

RB_WRAP(lcd_putsxy)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_putsxy(x, y, string);
    return 0;
}

RB_WRAP(lcd_puts)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_puts(x, y, string);
    return 0;
}

RB_WRAP(lcd_puts_scroll)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    const char* string = luaL_checkstring(L, 3);
    rb->lcd_puts_scroll(x, y, string);
    return 0;
}

RB_WRAP(lcd_stop_scroll)
{
    (void)L;
    rb->lcd_stop_scroll();
    return 0;
}

#ifdef HAVE_LCD_BITMAP
RB_WRAP(lcd_framebuffer)
{
    rli_wrap(L, rb->lcd_framebuffer, LCD_WIDTH, LCD_HEIGHT);
    return 1;
}

RB_WRAP(lcd_set_drawmode)
{
    int drawmode = luaL_checkint(L, 1);
    rb->lcd_set_drawmode(drawmode);
    return 0;
}

RB_WRAP(lcd_get_drawmode)
{
    int result = rb->lcd_get_drawmode();
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(lcd_setfont)
{
    int font = luaL_checkint(L, 1);
    rb->lcd_setfont(font);
    return 0;
}

RB_WRAP(lcd_drawpixel)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);

    rb->lcd_drawpixel(x, y);
    return 0;
}

RB_WRAP(lcd_drawline)
{
    int x1 = luaL_checkint(L, 1);
    int y1 = luaL_checkint(L, 2);
    int x2 = luaL_checkint(L, 3);
    int y2 = luaL_checkint(L, 4);

    rb->lcd_drawline(x1, y1, x2, y2);
    return 0;
}

RB_WRAP(lcd_hline)
{
    int x1 = luaL_checkint(L, 1);
    int x2 = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);

    rb->lcd_hline(x1, x2, y);
    return 0;
}

RB_WRAP(lcd_vline)
{
    int x = luaL_checkint(L, 1);
    int y1 = luaL_checkint(L, 2);
    int y2 = luaL_checkint(L, 3);

    rb->lcd_vline(x, y1, y2);
    return 0;
}

RB_WRAP(lcd_drawrect)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);

    rb->lcd_drawrect(x, y, width, height);
    return 0;
}

RB_WRAP(lcd_fillrect)
{
    int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
    int width = luaL_checkint(L, 3);
    int height = luaL_checkint(L, 4);

    rb->lcd_fillrect(x, y, width, height);
    return 0;
}

#if LCD_DEPTH > 1
RB_WRAP(lcd_set_foreground)
{
    unsigned foreground = luaL_checkint(L, 1);
    rb->lcd_set_foreground(foreground);
    return 0;
}

RB_WRAP(lcd_get_foreground)
{
    unsigned result = rb->lcd_get_foreground();
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(lcd_set_background)
{
    unsigned background = luaL_checkint(L, 1);
    rb->lcd_set_background(background);
    return 0;
}

RB_WRAP(lcd_get_background)
{
    unsigned result = rb->lcd_get_background();
    lua_pushinteger(L, result);
    return 1;
}

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

    rb->lcd_bitmap_part(src->data, src_x, src_y, stride, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_bitmap)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);

    rb->lcd_bitmap(src->data, x, y, width, height);
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

    rb->lcd_bitmap_transparent_part(src->data, src_x, src_y, stride, x, y, width, height);
    return 0;
}

RB_WRAP(lcd_bitmap_transparent)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);
    int width = luaL_checkint(L, 4);
    int height = luaL_checkint(L, 5);

    rb->lcd_bitmap_transparent(src->data, x, y, width, height);
    return 0;
}
#endif /* LCD_DEPTH == 16 */

#endif /* defined(LCD_BITMAP) */

RB_WRAP(yield)
{
    (void)L;
    rb->yield();
    return 0;
}

RB_WRAP(sleep)
{
    int ticks = luaL_checkint(L, 1);
    rb->sleep(ticks);
    return 0;
}

RB_WRAP(current_tick)
{
    lua_pushinteger(L, *rb->current_tick);
    return 1;
}

RB_WRAP(button_get)
{
    bool block = luaL_checkboolean(L, 1);
    long result = rb->button_get(block);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(button_get_w_tmo)
{
    int ticks = luaL_checkint(L, 1);
    long result = rb->button_get_w_tmo(ticks);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(button_status)
{
    int result = rb->button_status();
    lua_pushinteger(L, result);
    return 1;
}

#ifdef HAVE_BUTTON_DATA
RB_WRAP(button_get_data)
{
    int result = rb->button_get_data();
    lua_pushinteger(L, result);
    return 1;
}
#endif

#ifdef HAS_BUTTON_HOLD
RB_WRAP(button_hold)
{
    bool result = rb->button_hold();
    lua_pushboolean(L, result);
    return 1;
}
#endif

RB_WRAP(get_action)
{
    int context = luaL_checkint(L, 1);
    int timeout = luaL_checkint(L, 2);
    int result = rb->get_action(context, timeout);
    lua_pushinteger(L, result);
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

RB_WRAP(action_userabort)
{
    int timeout = luaL_checkint(L, 1);
    bool result = rb->action_userabort(timeout);
    lua_pushboolean(L, result);
    return 1;
}

RB_WRAP(kbd_input)
{
    luaL_Buffer b;
    luaL_buffinit(L, &b);

    char *buffer = luaL_prepbuffer(&b);
    buffer[0] = '\0';
    rb->kbd_input(buffer, LUAL_BUFFERSIZE);
    luaL_addsize(&b, strlen(buffer));

    luaL_pushresult(&b);
    return 1;
}

RB_WRAP(backlight_on)
{
    (void)L;
    rb->backlight_on();
    return 0;
}

RB_WRAP(backlight_off)
{
    (void)L;
    rb->backlight_off();
    return 0;
}

RB_WRAP(backlight_set_timeout)
{
    int val = luaL_checkint(L, 1);
    rb->backlight_set_timeout(val);
    return 0;
}

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
RB_WRAP(backlight_set_brightness)
{
    int val = luaL_checkint(L, 1);
    rb->backlight_set_brightness(val);
    return 0;
}
#endif

RB_WRAP(open)
{
    const char* pathname = luaL_checkstring(L, 1);
    int flags = luaL_checkint(L, 2);
    int result = rb->open(pathname, flags);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(close)
{
    int fd = luaL_checkint(L, 1);
    int result = rb->close(fd);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(read)
{
    size_t len, n, result = 0;
    luaL_Buffer b;

    int fd = luaL_checkint(L, 1);
    size_t count = luaL_checkint(L, 2);

    luaL_buffinit(L, &b);
    len = LUAL_BUFFERSIZE;
    do
    {
        char *p = luaL_prepbuffer(&b);

        if (len > count)
            len = count;

        n = rb->read(fd, p, len);

        luaL_addsize(&b, n);
        count -= n;
        result += n;
    } while (count > 0 && n == len);
    luaL_pushresult(&b); /* close buffer */

    lua_pushinteger(L, result);
    return 2;
}

RB_WRAP(lseek)
{
    int fd = luaL_checkint(L, 1);
    off_t offset = luaL_checkint(L, 2);
    int whence = luaL_checkint(L, 3);
    off_t result = rb->lseek(fd, offset, whence);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(creat)
{
    const char* pathname = luaL_checkstring(L, 1);
    int result = rb->creat(pathname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(write)
{
    size_t count;
    int fd = luaL_checkint(L, 1);
    void* buf = (void*)luaL_checklstring(L, 2, &count);
    ssize_t result = rb->write(fd, buf, count);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(remove)
{
    const char* pathname = luaL_checkstring(L, 1);
    int result = rb->remove(pathname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(rename)
{
    const char* path = luaL_checkstring(L, 1);
    const char* newname = luaL_checkstring(L, 2);
    int result = rb->rename(path, newname);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(ftruncate)
{
    int fd = luaL_checkint(L, 1);
    off_t length = luaL_checkint(L, 2);
    int result = rb->ftruncate(fd, length);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(filesize)
{
    int fd = luaL_checkint(L, 1);
    off_t result = rb->filesize(fd);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(file_exists)
{
    const char* path = luaL_checkstring(L, 1);
    bool result = rb->file_exists(path);
    lua_pushboolean(L, result);
    return 1;
}

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

#define R(NAME) {#NAME, rock_##NAME}
static const luaL_Reg rocklib[] =
{
    /* Graphics */
    R(lcd_clear_display),
    R(lcd_update),
    R(lcd_update_rect),
    R(lcd_puts),
    R(lcd_putsxy),
    R(lcd_puts_scroll),
    R(lcd_stop_scroll),
    R(splash),
#ifdef HAVE_LCD_BITMAP
    R(lcd_framebuffer),
    R(lcd_set_drawmode),
    R(lcd_get_drawmode),
    R(lcd_setfont),
    R(lcd_drawline),
    R(lcd_drawpixel),
    R(lcd_hline),
    R(lcd_vline),
    R(lcd_drawrect),
    R(lcd_fillrect),
#if LCD_DEPTH > 1
    R(lcd_set_foreground),
    R(lcd_get_foreground),
    R(lcd_set_background),
    R(lcd_get_background),
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

    /* File handling */
    R(open),
    R(close),
    R(read),
    R(lseek),
    R(creat),
    R(write),
    R(remove),
    R(rename),
    R(ftruncate),
    R(filesize),
    R(file_exists),

    /* Kernel */
    R(sleep),
    R(yield),
    R(current_tick),

    /* Buttons */
    R(button_get),
    R(button_get_w_tmo),
    R(button_status),
#ifdef HAVE_BUTTON_DATA
    R(button_get_data),
#endif
#ifdef HAS_BUTTON_HOLD
    R(button_hold),
#endif
    R(get_action),
    R(action_userabort),
#ifdef HAVE_TOUCHSCREEN
    R(action_get_touchscreen_press),
#endif
    R(kbd_input),

    /* Hardware */
    R(backlight_on),
    R(backlight_off),
    R(backlight_set_timeout),
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    R(backlight_set_brightness),
#endif

    R(font_getstringsize),
    R(read_bmp_file),
    R(set_viewport),
    R(clear_viewport),

    {"new_image", rli_new},

    {NULL, NULL}
};
#undef  R

#define RB_CONSTANT(x) lua_pushinteger(L, x); lua_setfield(L, -2, #x);
/*
 ** Open Rockbox library
 */
LUALIB_API int luaopen_rock(lua_State *L)
{
    luaL_register(L, LUA_ROCKLIBNAME, rocklib);

    RB_CONSTANT(HZ);

    RB_CONSTANT(LCD_WIDTH);
    RB_CONSTANT(LCD_HEIGHT);

    RB_CONSTANT(O_RDONLY);
    RB_CONSTANT(O_WRONLY);
    RB_CONSTANT(O_RDWR);
    RB_CONSTANT(O_CREAT);
    RB_CONSTANT(O_APPEND);
    RB_CONSTANT(O_TRUNC);
    RB_CONSTANT(SEEK_SET);
    RB_CONSTANT(SEEK_CUR);
    RB_CONSTANT(SEEK_END);
    
    RB_CONSTANT(FONT_SYSFIXED);
    RB_CONSTANT(FONT_UI);

    rli_init(L);

    return 1;
}
