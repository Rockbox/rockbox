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
#include "lib/pluginlib_actions.h"

/*
 * http://www.lua.org/manual/5.1/manual.html#lua_CFunction
 *
 * In order to communicate properly with Lua, a C function must use the following protocol,
 * which defines the way parameters and results are passed: a C function receives its arguments
 * from Lua in its stack in direct order (the first argument is pushed first). To return values to Lua,
 * a C function just pushes them onto the stack, in direct order (the first result is pushed first),
 * and returns the number of results. Any other value in the stack below the results will be properly
 * discarded by Lua. Like a Lua function, a C function called by Lua can also return many results.
 *
 * When porting new functions, don't forget to check rocklib_aux.pl whether it automatically creates
 * wrappers for the function and if so, add the function names to @forbidden_functions. This is to
 * prevent namespace collisions and adding duplicate wrappers.
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
    int     width;
    int     height;
    size_t  bytes;
    fb_data *data;
    fb_data dummy[1][1];
};

/* some devices need x | y coords shifted to match native format */
static fb_data x_shift = 0;
static fb_data y_shift = 0;
static fb_data xy_mask = 0;

static const fb_data *pixmask = NULL;

static const uint8_t pixmask_h2[8] =
    {0xC0, 0x30, 0x0C, 0x03, 0 ,0 ,0 ,0};
static const uint8_t pixmask_v2[8] =
    {0x03, 0x0C, 0x30, 0xC0, 0 ,0 ,0 ,0};
static const uint16_t pixmask_vi2[8] =
    {0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};
static const uint8_t pixmask_v1[8] =
    {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

#ifdef HAVE_LCD_COLOR
static unsigned invert_rgb(unsigned rgb)
{
    uint8_t r = 0xFFU - RGB_UNPACK_RED((unsigned) rgb);
    uint8_t g = 0xFFU - RGB_UNPACK_GREEN((unsigned) rgb);
    uint8_t b = 0xFFU - RGB_UNPACK_BLUE((unsigned) rgb);

    return LCD_RGBPACK(r, g, b);
}
#endif

/* Internal worker functions for image data array */
static fb_data* data_element(fb_data *data, size_t bytes, int x , int y, int w, int h)
{
    (void) h;
    if (x_shift)
        x = ((x - 1) >> x_shift) + 1;

    if (y_shift)
        y = ((y - 1) >> y_shift) + 1;

    size_t data_address = (w * (y - 1)) + (x - 1);

    if (data_address < bytes)
        return &data[data_address]; /* return element address */

    return NULL; /* data overflow */
}

static fb_data data_set(fb_data *data_element, int x, int y, fb_data newvalue)
{
    fb_data oldvalue = *data_element;
    fb_data mask;
    int bit_n;

    if (y_shift)
    {
        bit_n = (y-1) & xy_mask;
        mask = pixmask[bit_n];
        /*equivalent of:
          newvalue <<=bit_n;newvalue &= mask;newvalue |= oldvalue &= ~mask;
        */
        newvalue = oldvalue ^ ((oldvalue ^ (newvalue << bit_n)) & mask);
    }
    else if (x_shift)
    {
        bit_n = (x-1) & xy_mask;
        mask = pixmask[bit_n];
        newvalue = oldvalue ^ ((oldvalue ^ (newvalue << bit_n)) & mask);
    }

    *data_element = newvalue;

    return oldvalue;
}

static fb_data data_get(fb_data *data_element, int x, int y)
{
    fb_data value = *data_element;
    fb_data mask;
    int bit_n;

    if (y_shift)
    {
        bit_n = (y-1) & xy_mask;
        mask = pixmask[bit_n];
        value &= mask;
        value >>=bit_n;
    }
    else if (x_shift)
    {
        bit_n = (x-1) & xy_mask;
        mask = pixmask[bit_n];
        value &= mask;
        value >>=bit_n;
    }

    return value;
}

/* LUA Interface functions */
static void rli_wrap(lua_State *L, fb_data *src, int width, int height)
{
    int native_width;
    int native_height;

    size_t sz_header = sizeof(struct rocklua_image);
    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, sz_header);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    /* Some devices (1-bit / 2-bit displays) have packed bit formats that need
       to be unpacked in order to work on them at a pixel level.
       The internal formats of these devices do not follow the same paradigm
       for image sizes either
    */
    if (x_shift)
        native_width = ((width - 1) >> x_shift) + 1;
    else
        native_width = width;

    if (y_shift)
        native_height = ((height - 1) >> y_shift) + 1;
    else
        native_height = height;

    a->bytes = ((native_width * native_height)) * sizeof(fb_data);

    a->data = src;

    a->width = width;
    a->height = height;
}

static fb_data* rli_alloc(lua_State *L, int width, int height)
{
    int native_width;
    int native_height;

    /* Some devices (1-bit / 2-bit displays) have packed bit formats that need
       to be unpacked in order to work on them at a pixel level.
       The internal formats of these devices do not follow the same paradigm
       for image sizes either
    */
    if (x_shift)
        native_width = ((width - 1) >> x_shift) + 1;
    else
        native_width = width;

    if (y_shift)
        native_height = ((height - 1) >> y_shift) + 1;
    else
        native_height = height;

    size_t sz_header = sizeof(struct rocklua_image);
    size_t nbytes = sz_header + ((native_width*native_height) - 1) * sizeof(fb_data);

    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    a->width  = width;
    a->height = height;

    a->bytes  = ((native_width * native_height)) * sizeof(fb_data);

    a->data   = &a->dummy[0][0];

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

static int rli_tostring(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushfstring(L, ROCKLUA_IMAGE ": %dx%d, %d bytes",
                          a->width, a->height, a->bytes);
    return 1;
}

static int rli_size(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->bytes);
    return 1;
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

static int rli_set(lua_State *L)
{
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));
    fb_data newvalue = FB_SCALARPACK((unsigned)luaL_checknumber(L, 4));

    luaL_argcheck(L, 1 <= x && x <= a->width, 2, "index out of range");
    luaL_argcheck(L, 1 <= y && y <= a->height, 3, "index out of range");

    element =  data_element(a->data, a->bytes, x , y, a->width, a->height);

    luaL_argcheck(L, element != NULL, 1, "data overflow");

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_set(element, x, y, newvalue)));
    return 1;
}

static int rli_get(lua_State *L)
{
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));

    luaL_argcheck(L, 1 <= x && x <= a->width, 2, "index out of range");
    luaL_argcheck(L, 1 <= y && y <= a->height, 3, "index out of range");

    element =  data_element(a->data, a->bytes, x , y, a->width, a->height);

    luaL_argcheck(L, element != NULL, 1, "data overflow");

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(element, x, y)));

    return 1;
}

static int rli_clear(lua_State *L)
{
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    fb_data newvalue = (unsigned)luaL_optnumber(L, 2, 0);
    int x1 = luaL_optnumber(L, 3, 0);
    int y1 = luaL_optnumber(L, 4, 0);
    int x2 = luaL_optnumber(L, 5, 0);
    int y2 = luaL_optnumber(L, 6, 0);

    lua_settop(L, 1);/* dump everything from stack but the image pointer */

    if (!x1 || !x2 || !y1 || !y2) /* whole image */
    {
        newvalue = FB_SCALARPACK(newvalue);
        memset(a->data, newvalue, a->bytes);
    }
    else
    {
        luaL_argcheck(L, 1 <= x1 && x1 <= a->width, 3, "index out of range");
        luaL_argcheck(L, 1 <= y1 && y1 <= a->height, 4, "index out of range");
        luaL_argcheck(L, 1 <= x2 && x2 <= a->width, 5, "index out of range");
        luaL_argcheck(L, 1 <= y2 && y2 <= a->height, 6, "index out of range");

        for (int y = y1; y <= y2; y++)
        {
            for (int x = x1; x <= x2; x++)
            {
                element =  data_element(a->data, a->bytes, x , y, a->width, a->height);

                luaL_argcheck(L, element != NULL, 1, "data overflow");

                data_set(element, x, y, FB_SCALARPACK((newvalue)));
            }
        }
    }
    return 0;
}

static int rli_invert(lua_State *L)
{
    fb_data *element = NULL;
    unsigned value;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x1 = luaL_optnumber(L, 2, 1);
    int y1 = luaL_optnumber(L, 3, 1);
    int x2 = luaL_optnumber(L, 4, a->width);
    int y2 = luaL_optnumber(L, 5, a->height);

    lua_settop(L, 1);/* dump everything from stack but the image pointer */

    luaL_argcheck(L, 1 <= x1 && x1 <= a->width, 2, "index out of range");
    luaL_argcheck(L, 1 <= y1 && y1 <= a->height, 3, "index out of range");
    luaL_argcheck(L, 1 <= x2 && x2 <= a->width, 2, "index out of range");
    luaL_argcheck(L, 1 <= y2 && y2 <= a->height, 3, "index out of range");

    for (int y = y1; y <= y2; y++)
    {
        for (int x = x1; x <= x2; x++)
        {
            element =  data_element(a->data, a->bytes, x , y, a->width, a->height);

            luaL_argcheck(L, element != NULL, 1, "data overflow");

            value = FB_UNPACK_SCALAR_LCD(data_get(element, x, y));
            #if defined(HAVE_LCD_COLOR)
            value = invert_rgb(value);
            #else
            value = ~value;
            #endif
            data_set(element, x, y, FB_SCALARPACK((value)));
        }
    }

    return 0;
}


static int rli_copy(lua_State *L)
{
    /*(dest*, src*, d_x1, d_y1, s_x2, s_y2, width, height, [operation], clr) */
    fb_data *dest_element = NULL;
    fb_data *src_element = NULL;
    unsigned srcvalue;
    unsigned destvalue;
    int x = 0;
    int y = 0;

    struct rocklua_image *d = rli_checktype(L, 1); /*dest*/
    struct rocklua_image *s = rli_checktype(L, 2); /*src*/
    int d_x1 = luaL_optnumber(L, 3, 0);
    int d_y1 = luaL_optnumber(L, 4, 0);
    int s_x2 = luaL_optnumber(L, 5, 0);
    int s_y2 = luaL_optnumber(L, 6, 0);
    int w  = luaL_optnumber(L, 7, 0);
    int h  = luaL_optnumber(L, 8, 0);
    const int op = luaL_optnumber(L, 9, 0);
    const unsigned clr = luaL_optnumber(L, 10, 0);
    lua_settop(L, 2); /* dump everything from stack but the image pointers */

    if (!d_x1 || !s_x2 || !d_y1 || !s_y2) //whole image
    {
        luaL_argcheck(L, s->bytes == d->bytes && s->width == d-> width,
                                             2, "image size mismatch");
        memcpy(d->data, s->data, d->bytes);
    }

    if (w == 0)
        w = MIN(d->width - d_x1, s->width - s_x2 );
    if (h == 0)
        h = MIN(d->height - d_y1, s->height - s_y2 );

    while ((y + d_y1 <= d->height && y <= h) &&
          (y + s_y2 <= s->height && y <= h))
    {

        while ((x + d_x1 <= d->width && x <= w) &&
              (x + s_x2 <= s->width && x <= w))
        {
            dest_element = data_element(d->data, d->bytes,
                                        x + d_x1 , y + d_y1,
                                        d->width, d->height);

            src_element = data_element(s->data, s->bytes,
                                       x + s_x2 , y + s_y2,
                                       s->width, s->height);

            luaL_argcheck(L, src_element != NULL && dest_element != NULL,
                             1, "image data overflow");

            srcvalue = FB_UNPACK_SCALAR_LCD(data_get(src_element, x + s_x2, y + s_y2));
            destvalue = FB_UNPACK_SCALAR_LCD(data_get(dest_element, x + d_x1, y + d_y1));

            switch (op)
            {
                default:
                case 0:
                    destvalue = srcvalue; //copy
                    break;
                case 1:
                    destvalue |= srcvalue; //or
                    break;
                case 2:
                    destvalue ^= srcvalue; //xor
                    break;
                case 3:
                    destvalue = ~(destvalue | srcvalue); //nor
                    break;
                case 4:
                    destvalue &= srcvalue; //and
                    break;
                case 5:
                    destvalue = ~(destvalue & srcvalue); //nand
                    break;
                case 6:
                    destvalue = ~srcvalue; //not
                    break;
#ifdef HAVE_LCD_COLOR
                case 7:
                    if (srcvalue != clr)
                        destvalue = srcvalue;
                    break;
                case 8:
                    if (srcvalue == clr)
                        destvalue = srcvalue;
                    break;
                case 9:
                    if (srcvalue > clr)
                        destvalue = srcvalue;
                    break;
                case 10:
                    if (srcvalue < clr)
                        destvalue = srcvalue;
                    break;
                case 11:
                    if (destvalue != clr)
                        destvalue = srcvalue;
                    break;
                case 12:
                    if (destvalue == clr)
                        destvalue = srcvalue;
                    break;
                case 13:
                    if (destvalue > clr)
                        destvalue = srcvalue;
                    break;
                case 14:
                    if (destvalue < clr)
                        destvalue = srcvalue;
                    break;
#endif
            } /*switch op*/

            data_set(dest_element, x + d_x1, y + d_y1, FB_SCALARPACK(destvalue));

            x++;
        }
        x = 0;
        y++;

    }
    return 0;
}

static const struct luaL_reg rli_lib [] =
{
    {"__tostring", rli_tostring},
    {"size",   rli_size},
    {"width",  rli_width},
    {"height", rli_height},
    {"set",    rli_set},
    {"get",    rli_get},
    {"copy",   rli_copy},
    {"clear",  rli_clear},
    {"invert", rli_invert},
    {NULL, NULL}
};

static inline void rli_init(lua_State *L)
{

    if (LCD_PIXELFORMAT == HORIZONTAL_PACKING)
    {
        if (LCD_DEPTH == 2)
        {
            x_shift = 2U;
            pixmask = (fb_data *) pixmask_h2;
        }
    }
    else if (LCD_PIXELFORMAT == VERTICAL_PACKING)
    {
        if (LCD_DEPTH == 1)
        {
            y_shift = 3U;

            pixmask = (fb_data *) pixmask_v1;
        }
        else if (LCD_DEPTH == 2)
        {
            y_shift = 2U;
            pixmask = (fb_data *) pixmask_v2;
        }
    }
    else if (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    {
        if (LCD_DEPTH == 2)
        {
            y_shift = 3U;
            pixmask = (fb_data *) pixmask_vi2;
        }
    }

    if (y_shift)
        xy_mask = ((1 << y_shift) -1);
    else if (x_shift)
        xy_mask = ((1 << x_shift) -1);

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

#define RB_WRAP(M) static int rock_##M(lua_State UNUSED_ATTR *L)
#define SIMPLE_VOID_WRAPPER(func) RB_WRAP(func) { (void)L; func(); return 0; }

/* Helper function for opt_viewport */
static void check_tablevalue(lua_State *L, const char* key, int tablepos, void* res, bool unsigned_val)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    if (!lua_isnoneornil(L, -1))
    {
        if (unsigned_val)
            *(unsigned*)res = luaL_checkint(L, -1);
        else
            *(int*)res = luaL_checkint(L, -1);
    }

    lua_pop(L, 1); /* Pop the value off the stack */
}

static struct viewport* opt_viewport(lua_State *L, int narg, struct viewport* alt)
{
    if (lua_isnoneornil(L, narg))
        return alt;

    int tablepos = lua_gettop(L);
    struct viewport *vp;

    lua_getfield(L, tablepos, "vp");  /* get table['vp'] */
    if (lua_isnoneornil(L, -1))
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
    if (backdrop == NULL)
        lua_pushnil(L);
    else
        rli_wrap(L, backdrop, LCD_WIDTH, LCD_HEIGHT);

    return 1;
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

    if (input != NULL)
        rb->strlcpy(buffer, input, LUAL_BUFFERSIZE);
    else
        buffer[0] = '\0';

    if (!rb->kbd_input(buffer, LUAL_BUFFERSIZE))
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
RB_WRAP(touchscreen_get_mode)
{
    lua_pushinteger(L, rb->touchscreen_get_mode());
    return 1;
}
#endif

RB_WRAP(font_getstringsize)
{
    const unsigned char* str = luaL_checkstring(L, 1);
    int fontnumber = luaL_checkint(L, 2);
    int w, h;

    if (fontnumber == FONT_UI)
        fontnumber = rb->global_status->font_id[SCREEN_MAIN];
    else
        fontnumber = FONT_SYSFIXED;

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

    if (dither)
        format |= FORMAT_DITHER;

    if (transparent)
        format |= FORMAT_TRANSPARENT;

    int result = rb->read_bmp_file(filename, &bm, 0, format | FORMAT_RETURN_SIZE, NULL);

    if (result > 0)
    {
        bm.data = (unsigned char*) rli_alloc(L, bm.width, bm.height);
        if (rb->read_bmp_file(filename, &bm, result, format, NULL) < 0)
        {
            /* Error occured, drop newly allocated image from stack */
            lua_pop(L, 1);
            lua_pushnil(L);
        }
    }
    else
        lua_pushnil(L);

    return 1;
}

RB_WRAP(current_path)
{
    const char *current_path = get_current_path(L, 1);
    if (current_path != NULL)
        lua_pushstring(L, current_path);
    else
        lua_pushnil(L);

    return 1;
}

static void fill_text_message(lua_State *L, struct text_message * message,
                              int pos)
{
    int i;
    luaL_checktype(L, pos, LUA_TTABLE);
    int n = luaL_getn(L, pos);
    const char **lines = (const char**) tlsf_malloc(n * sizeof(const char*));
    if (lines == NULL)
        luaL_error(L, "Can't allocate %d bytes!", n * sizeof(const char*));
    for (i=1; i<=n; i++)
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
    if (!lua_isnoneornil(L, 2))
        fill_text_message(L, (yes = &yes_message), 2);
    if (!lua_isnoneornil(L, 3))
        fill_text_message(L, (no = &no_message), 3);

    enum yesno_res result = rb->gui_syncyesno_run(&main_message, yes, no);

    tlsf_free(main_message.message_lines);
    if (yes)
        tlsf_free(yes_message.message_lines);
    if (no)
        tlsf_free(no_message.message_lines);

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
    items = (const char**) tlsf_malloc(n * sizeof(const char*));
    if (items == NULL)
        luaL_error(L, "Can't allocate %d bytes!", n * sizeof(const char*));
    for (i=1; i<=n; i++)
    {
        lua_rawgeti(L, 2, i); /* Push item on the stack */
        items[i-1] = luaL_checkstring(L, -1);
        lua_pop(L, 1); /* Pop it */
    }

    menu.strings = items;
    menu.flags |= MENU_ITEM_COUNT(n);
    menu_desc.desc = (unsigned char*) title;

    int result = rb->do_menu(&menu, &start_selected, NULL, false);

    tlsf_free(items);

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(playlist_sync)
{
    /* just pass NULL to work with the current playlist */
    rb->playlist_sync(NULL);
    return 1;
}

RB_WRAP(playlist_remove_all_tracks)
{
    /* just pass NULL to work with the current playlist */
    int result = rb->playlist_remove_all_tracks(NULL);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(playlist_insert_track)
{
    const char *filename;
    int position, queue, sync;

    /* for now don't take a playlist_info pointer, but just pass NULL to use
	   the current playlist. If this changes later, all the other
	   parameters can be shifted back. */
    filename = luaL_checkstring(L, 1); /* only required parameter */
    position = luaL_optint(L, 2, PLAYLIST_INSERT);
    queue = lua_toboolean(L, 3); /* default to false */
    sync = lua_toboolean(L, 4); /* default to false */

    int result = rb->playlist_insert_track(NULL, filename, position,
        queue, sync);

    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(playlist_insert_directory)
{
    const char* dirname;
    int position, queue, recurse;

    /* just like in playlist_insert_track, only works with current playlist. */
    dirname = luaL_checkstring(L, 1); /* only required parameter */
    position = luaL_optint(L, 2, PLAYLIST_INSERT);
    queue = lua_toboolean(L, 3); /* default to false */
    recurse = lua_toboolean(L, 4); /* default to false */

    int result = rb->playlist_insert_directory(NULL, dirname, position,
        queue, recurse);

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

RB_WRAP(get_plugin_action)
{
    static const struct button_mapping *m1[] = { pla_main_ctx };
    int timeout = luaL_checkint(L, 1);
    int btn;
#ifdef HAVE_REMOTE_LCD
    static const struct button_mapping *m2[] = { pla_main_ctx, pla_remote_ctx };
    bool with_remote = luaL_optint(L, 2, 0);
    if (with_remote)
        btn = pluginlib_getaction(timeout, m2, 2);
    else
#endif
        btn = pluginlib_getaction(timeout, m1, 1);

    lua_pushinteger(L, btn);
    return 1;
}

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
    R(touchscreen_get_mode),
#endif
    R(kbd_input),

    R(font_getstringsize),
    R(read_bmp_file),
    R(set_viewport),
    R(clear_viewport),
    R(current_path),
    R(gui_syncyesno_run),
    R(playlist_sync),
    R(playlist_remove_all_tracks),
    R(playlist_insert_track),
    R(playlist_insert_directory),
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
    R(get_plugin_action),

    {"new_image", rli_new},

    {NULL, NULL}
};
#undef  R
extern const luaL_Reg rocklib_aux[];


#define RB_CONSTANT(x)        lua_pushinteger(L, x); lua_setfield(L, -2, #x);
#define RB_STRING_CONSTANT(x) lua_pushstring(L, x); lua_setfield(L, -2, #x);
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
    RB_CONSTANT(LCD_DEPTH);

    RB_CONSTANT(FONT_SYSFIXED);
    RB_CONSTANT(FONT_UI);

    RB_CONSTANT(PLAYLIST_PREPEND);
    RB_CONSTANT(PLAYLIST_INSERT);
    RB_CONSTANT(PLAYLIST_INSERT_LAST);
    RB_CONSTANT(PLAYLIST_INSERT_FIRST);
    RB_CONSTANT(PLAYLIST_INSERT_SHUFFLED);
    RB_CONSTANT(PLAYLIST_REPLACE);
    RB_CONSTANT(PLAYLIST_INSERT_LAST_SHUFFLED);

#ifdef HAVE_TOUCHSCREEN
    RB_CONSTANT(TOUCHSCREEN_POINT);
    RB_CONSTANT(TOUCHSCREEN_BUTTON);
#endif

    RB_CONSTANT(SCREEN_MAIN);
#ifdef HAVE_REMOTE_LCD
    RB_CONSTANT(SCREEN_REMOTE);
#endif

    /* some useful paths constants */
    RB_STRING_CONSTANT(ROCKBOX_DIR);
    RB_STRING_CONSTANT(HOME_DIR);
    RB_STRING_CONSTANT(PLUGIN_DIR);
    RB_STRING_CONSTANT(PLUGIN_APPS_DATA_DIR);
    RB_STRING_CONSTANT(PLUGIN_GAMES_DATA_DIR);
    RB_STRING_CONSTANT(PLUGIN_DATA_DIR);
    RB_STRING_CONSTANT(VIEWERS_DATA_DIR);

    rli_init(L);

    return 1;
}
