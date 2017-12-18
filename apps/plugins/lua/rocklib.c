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
 * Copyright (C) 2017 William Wilgus
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
 * from Lua in its stack in direct order (the first argument is pushed first). To return vals to Lua,
 * a C function just pushes them onto the stack, in direct order (the first result is pushed first),
 * and returns the number of results. Any other val in the stack below the results will be properly
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
#ifndef ABS
#define ABS(a) (((a) < 0) ? -(a) : (a))
#endif

#define ERR_IDX_RANGE "index out of range"
#define ERR_DATA_OVF  "data overflow"
#define ROCKLUA_IMAGE "rb.image"
struct rocklua_image
{
    int     width;
    int     height;
    int     stride;
    size_t  bytes;
    size_t  elems;
    fb_data *data;
    fb_data dummy[1][1];
};

/* some devices need x | y coords shifted to match native format */
static fb_data x_shift = 0;
static fb_data y_shift = 0;
static fb_data xy_mask = 0;
static const fb_data *pixmask = NULL;

static const uint8_t pixmask_h2[8] =
    {0x03, 0x0C, 0x30, 0xC0, 0,0,0,0};
    /*MSB on left*/

static const uint8_t pixmask_v2[8] =
    {0x03, 0x0C, 0x30, 0xC0, 0,0,0,0};

static const uint8_t pixmask_v1[8] =
    {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

static const uint16_t pixmask_vi2[8] =
    {0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};

static const uint16_t greymap_vi2[4] =
    {0x0, 0x001, 0x0100, 0x0101};

#ifdef HAVE_LCD_COLOR
static inline unsigned invert_rgb(unsigned rgb)
{
    uint8_t r = 0xFFU - RGB_UNPACK_RED((unsigned) rgb);
    uint8_t g = 0xFFU - RGB_UNPACK_GREEN((unsigned) rgb);
    uint8_t b = 0xFFU - RGB_UNPACK_BLUE((unsigned) rgb);

    return LCD_RGBPACK(r, g, b);
}
#endif

/* Internal worker functions for image data array */
static inline fb_data* data_element(fb_data *data, size_t elements, 
                                            int x, int y, int stride)
{
    if(y_shift)
        y = ((y - 1) >> y_shift) + 1;
    else if(x_shift)
        x = ((x - 1) >> x_shift) + 1;

    size_t data_address = (stride * (y - 1)) + (x - 1);

    if(data_address >= elements)
        return NULL; /* data overflow */

    return &data[data_address]; /* return element address */
}

static inline fb_data data_set(fb_data *data_element, int x, int y, fb_data new_val)
{
    fb_data old_val = *data_element;
    fb_data mask;
    int bit_n;

#if (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    (void) x;
    /* NOTE old val will be returned in native (unmapped) format */
    /* [0-3] (mapped to)=> {0x0, 0x001, 0x0100, 0x0101} */
    new_val = greymap_vi2[new_val & (0x3)];
    bit_n = (y-1) & xy_mask;
    mask = pixmask[bit_n];
    new_val = old_val ^ ((old_val ^ (new_val << (bit_n))) & mask);
#else
    if(y_shift)
    {
        bit_n = (y-1) & xy_mask;
        mask = pixmask[bit_n];
        new_val = old_val ^ ((old_val ^ (new_val << (LCD_DEPTH * bit_n))) & mask);
    }
    else if(x_shift)
    {
        bit_n = xy_mask - ((x-1) & xy_mask); /*MSB on left*/
        mask = pixmask[bit_n];
        new_val = old_val ^ ((old_val ^ (new_val << (LCD_DEPTH * bit_n))) & mask);
    }
#endif    
    *data_element = new_val;

    return old_val;
}

static inline fb_data data_get(fb_data *data_element, int x, int y)
{
    fb_data val = *data_element;
    fb_data mask;
    int bit_n;

#if (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    (void) x;
    bit_n = (y-1) & xy_mask;
    mask = pixmask[bit_n];
    val &= mask;
    val >>=(bit_n);
    /*{0x0, 0x001, 0x0100, 0x0101} (mapped to)=> [0-3]*/
    if ((val & greymap_vi2[2]) != 0)
        val = (val & 0x1U) + 2U; /* 2, 3 */
    else 
        val &= 1U; /* 0, 1 */
#else
    if(y_shift)
    {
        bit_n = (y-1) & xy_mask;
        mask = pixmask[bit_n];
        val &= mask;
        val >>=(LCD_DEPTH * bit_n);
    }
    else if(x_shift)
    {
        bit_n = xy_mask - ((x-1) & xy_mask); /*MSB on Left*/
        mask = pixmask[bit_n];
        val &= mask;
        val >>=(LCD_DEPTH * bit_n);
    }
#endif

    return val;
}

/* LUA Interface functions */
static void rli_wrap(lua_State *L, fb_data *src, int width, int height)
{
    int w_native;
    int h_native;

    size_t sz_header = sizeof(struct rocklua_image);
    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, sz_header);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    /* Some devices (1-bit / 2-bit displays) have packed bit formats that need
       to be unpacked in order to work on them at a pixel level.
       The internal formats of these devices do not follow the same paradigm
       for image sizes either we still display the actual width and height to
       the user but store stride and bytes based on the native vals
    */
    if(x_shift)
        w_native = ((width - 1) >> x_shift) + 1;
    else
        w_native = width;

    if(y_shift)
        h_native = ((height - 1) >> y_shift) + 1;
    else
        h_native = height;

    a->elems = ((w_native * h_native));
    a->bytes = a->elems * sizeof(fb_data);
    
    a->data = src;

    a->stride = w_native;
    a->width = width;
    a->height = height;
}

static fb_data* rli_alloc(lua_State *L, int width, int height)
{
    int w_native;
    int h_native;

    /* Some devices (1-bit / 2-bit displays) have packed bit formats that need
       to be unpacked in order to work on them at a pixel level.
       The internal formats of these devices do not follow the same paradigm
       for image sizes either we still display the actual width and height to
       the user but store stride and bytes based on the native values
    */
    if(x_shift)
        w_native = ((width - 1) >> x_shift) + 1;
    else
        w_native = width;

    if(y_shift)
        h_native = ((height - 1) >> y_shift) + 1;
    else
        h_native = height;

    size_t sz_header = sizeof(struct rocklua_image);
    size_t nbytes = sz_header + ((w_native * h_native) - 1) * sizeof(fb_data);

    struct rocklua_image *a = (struct rocklua_image *)lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    a->stride = w_native;
    /* apparent w/h is stored but behind the scenes native w/h is used */
    a->width  = width;
    a->height = height;

    a->elems = ((w_native * h_native));
    a->bytes = a->elems * sizeof(fb_data);

    a->data   = &a->dummy[0][0];

    return a->data;
}

static int rli_new(lua_State *L)
{
    int width = luaL_checkint(L, 1);
    int height = luaL_checkint(L, 2);

    luaL_argcheck(L, width > 0,  1, ERR_IDX_RANGE);
    luaL_argcheck(L, height > 0, 2, ERR_IDX_RANGE);

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

    switch(luaL_optnumber(L, 2, 0))
    {
        default:
        case 0:
        {
            lua_pushfstring(L, 
            ROCKLUA_IMAGE ": %dx%d, %d elements, %d bytes, %d-bit depth, %d pack",
            a->width, a->height, a->elems, a->bytes, LCD_DEPTH, LCD_PIXELFORMAT);
            break;
        }        
        case 1: { lua_pushfstring(L, ROCKLUA_IMAGE   ); break; }
        case 2: { lua_pushfstring(L, "%d", a->width  ); break; }
        case 3: { lua_pushfstring(L, "%d", a->height ); break; }
        case 4: { lua_pushfstring(L, "%d", a->elems  ); break; }
        case 5: { lua_pushfstring(L, "%d", a->bytes  ); break; }
        case 6: { lua_pushfstring(L, "%d", LCD_DEPTH ); break; }
        case 7: { lua_pushfstring(L, "%d", LCD_PIXELFORMAT); break; }
        case 8: { lua_pushfstring(L, "%p", a->data); break; }
    }  

    return 1;
}

static int rli_size(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->elems);
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
    /*(dst*, [x1, y1, val]) */
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));
    fb_data new_val = FB_SCALARPACK((unsigned)luaL_checknumber(L, 4));

    luaL_argcheck(L, x > 0 && x <= a->width,  2, ERR_IDX_RANGE);
    luaL_argcheck(L, y > 0 && y <= a->height, 3, ERR_IDX_RANGE);

    element =  data_element(a->data, a->elems, x, y, a->stride);

    luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_set(element, x, y, new_val)));

    /* returns old val */
    return 1;
}

static int rli_get(lua_State *L)
{
    /*val = (src*, x1, y1) */
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));

    luaL_argcheck(L, x > 0 && x <= a->width,  2, ERR_IDX_RANGE);
    luaL_argcheck(L, y > 0 && y <= a->height, 3, ERR_IDX_RANGE);

    element =  data_element(a->data, a->elems, x, y, a->stride);

    luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(element, x, y)));

    return 1;
}

static int d_line(struct rocklua_image *a, int x1, int y1, int x2, int y2, fb_data clr)
{
    fb_data *element;

    int d_a = x2 - x1;
    int d_b = y2 - y1;

    int s_a = 1;
    int s_b = 1;

    int swap;
    int d_err;
    int numpixels;

    int *a1 = &x1;
    int *b1 = &y1;

    if (d_a < 0) {
        d_a = -d_a;  
        s_a = -s_a;
    }

    if (d_b < 0) 
    { 
        d_b = -d_b;
        s_b = -s_b; 
    }

    x2 += s_a;
    y2 += s_b;

    if (d_b > d_a) 
    {
        a1 = &y1;
        b1 = &x1;
        swap = d_a;
        d_a = d_b;
        d_b = swap;
        swap = s_a;
        s_a = s_b;
        s_b = swap;
    }

    d_err = ((d_b << 1) - d_a) >> 1;
    numpixels = d_a + 1;
    d_a -= d_b;
    for(;numpixels > 0; numpixels--)
    {
        element = data_element(a->data, a->elems, x1, y1, a->stride);
        if (element)
            data_set(element, x1, y1, clr);
        else
            return 1;

        if (d_err >= 0)
        {
            *b1 += s_b;
            d_err -= d_a;
        }
        else
            d_err += d_b;

        *a1 += s_a;          
    }

    return 0;
}

static int rli_line(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);

    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_optnumber(L, 4, x1);
    int y2 = luaL_optnumber(L, 5, y1);
    luaL_argcheck(L, x1 > 0 && x1 <= a->width,  2, ERR_IDX_RANGE);
    luaL_argcheck(L, y1 > 0 && y1 <= a->height, 3, ERR_IDX_RANGE);
    luaL_argcheck(L, x2 > 0 && x2 <= a->width,  4, ERR_IDX_RANGE);
    luaL_argcheck(L, y2 > 0 && y2 <= a->height, 5, ERR_IDX_RANGE);
#ifdef HAVE_LCD_COLOR
    fb_data clr =
            FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, LCD_RGBPACK(0, 0, 0)));
#else
    fb_data clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, 0));
#endif
    luaL_argcheck(L, d_line(a, x1, y1, x2, y2, clr) == 0, 1, ERR_DATA_OVF "1");;
    return 0;
}

static int d_ellipse_rect(struct rocklua_image *rli,
                    int x1, int y1,
                    int x2, int y2,
                    fb_data *clr, fb_data *fillclr)
{
    /* NOTE! clr and fillclr passed as pointers unlike the other functions. */
    /* Rasterizing algorithm derivative of work by Alois Zing */
    fb_data *element1;
    fb_data *element2;
    fb_data *element3;
    fb_data *element4;

    int a = ABS(x2 - x1); /* diameter */
    int b = ABS(y2 - y1); /* diameter */

    if(!clr || a == 0 || b == 0)
        return 0; /* not an error but nothing to display */

    int b1 = (b & 1);
    b = b - (1 - b1);

    int a2 = a * a;
    int b2 = b * b;

    long dx = ((1 - a) * b2) >> 1; /* error increment */
    long dy = (b1 * a2) >> 1;      /* error increment */

    long err = (dx + dy + b1 * a2) << 1; /* error of 1.step */
    long e2;

    if (x1 > x2) /* if called with swapped points .. exchange them */
    {
        x1 = x2;
        x2 += a;
    }

    if (y1 > y2) /* if called with swapped points .. exchange them */
    {
        y1 = y2;
    }

    y1 += (b + 1) >> 1;
    y2 = y1 - b1;

    do {
        element1 = data_element(rli->data, rli->elems, x2, y1, rli->stride);
        element2 = data_element(rli->data, rli->elems, x1, y1, rli->stride);
        element3 = data_element(rli->data, rli->elems, x1, y2, rli->stride);
        element4 = data_element(rli->data, rli->elems, x2, y2, rli->stride);
        if (element1 && element2 && element3 && element4)
        {
            data_set(element1, x2, y1, *clr); /*   I. Quadrant +x +y */
            data_set(element2, x1, y1, *clr); /*  II. Quadrant -x +y */
            data_set(element3, x1, y2, *clr); /* III. Quadrant -x -y */
            data_set(element4, x2, y2, *clr); /*  IV. Quadrant +x -y */
            if(fillclr && x1 != x2)
            {            
                d_line(rli, x1 + 1, y1, x2 - 1, y1, *fillclr); /* I. II. Quad */
                d_line(rli, x1 + 1, y2, x2 - 1, y2, *fillclr); /* III. IV. Quad */
            }
        }
        else
            return 1; /* ERROR */

        e2 = err;

        if (e2 <= dy) /* y step */
        {
            y1++;
            y2--;
            dy += a2;
            err += dy + dy;
        }

        if (e2 >= dx || err > dy) /* x step */
        {
            x1++;
            x2--;
            dx += b2;
            err += dx + dx;
        }

    } while (x1 <= x2);

    while (y1 - y2 < b) /* for early stop of flat ellipse a=1 finish tip */
    {
        element1 = data_element(rli->data, rli->elems, x2 + 1, y1, rli->stride);
        element2 = data_element(rli->data, rli->elems, x1 - 1, y1, rli->stride);
        element3 = data_element(rli->data, rli->elems, x1 - 1, y2, rli->stride);
        element4 = data_element(rli->data, rli->elems, x2 + 1, y2, rli->stride);
        if (element1 && element2 && element3 && element4)
        {
            data_set(element1, x2, y1, *clr);
            data_set(element2, x1, y1, *clr);
            data_set(element3, x1, y2, *clr);
            data_set(element4, x2, y2, *clr);
        }
        else
            return 2; /* ERROR */

        y1++;
        y2--;
    }

    return 0;
}

static int rli_ellipse_rect(lua_State *L)
{
    /* (dst*, x1, y1, x2, y2, [clr, fillclr]) */
    struct rocklua_image *a = rli_checktype(L, 1);
    fb_data clr;
    fb_data fillclr;
    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_checkint(L, 4);
    int y2 = luaL_checkint(L, 5);


    luaL_argcheck(L, x1 > 0 && x1 <= a->width,  2, ERR_IDX_RANGE);
    luaL_argcheck(L, y1 > 0 && y1 <= a->height, 3, ERR_IDX_RANGE);
    luaL_argcheck(L, x2 > 0 && x2 <= a->width,  4, ERR_IDX_RANGE);
    luaL_argcheck(L, y2 > 0 && y2 <= a->height, 5, ERR_IDX_RANGE);
#ifdef HAVE_LCD_COLOR
    clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, LCD_RGBPACK(0, 0, 0)));
#else
    clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, 0));
#endif  

    if (!lua_isnoneornil(L, 7))
    {
        fillclr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 7, clr));
        luaL_argcheck(L, d_ellipse_rect(a, x1, y1, x2, y2, &clr, &fillclr) == 0,
                      1, ERR_DATA_OVF "1");
    }
    else
    {
        luaL_argcheck(L, d_ellipse_rect(a, x1, y1, x2, y2, &clr, NULL) == 0,
                      1, ERR_DATA_OVF "1");
    }
    
    return 0;
}

static int rli_clear(lua_State *L)
{
    /* (dst*, [color, x1, y1, x2, y2]) */
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
#ifdef HAVE_LCD_COLOR
    fb_data new_val =
            FB_SCALARPACK((unsigned) luaL_optnumber(L, 2, LCD_RGBPACK(0, 0, 0)));
#else
    fb_data new_val = FB_SCALARPACK((unsigned) luaL_optnumber(L, 2, 0));
#endif
    int x1 = luaL_optnumber(L, 3, 1);
    int y1 = luaL_optnumber(L, 4, 1);
    int x2 = luaL_optnumber(L, 5, a->width);
    int y2 = luaL_optnumber(L, 6, a->height);

    luaL_argcheck(L, x1 > 0 && x1 <= a->width,  3, ERR_IDX_RANGE);
    luaL_argcheck(L, y1 > 0 && y1 <= a->height, 4, ERR_IDX_RANGE);
    luaL_argcheck(L, x2 > 0 && x2 <= a->width,  5, ERR_IDX_RANGE);
    luaL_argcheck(L, y2 > 0 && y2 <= a->height, 6, ERR_IDX_RANGE);

    for(int y = y1; y <= y2; y++)
    {
        for(int x = x1; x <= x2; x++)
        {
            element = data_element(a->data, a->elems, x, y, a->stride);

            luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

            data_set(element, x, y, new_val);
        }
    }
    return 0;
}

static int rli_invert(lua_State *L)
{
    /*(dst*, [x1, y1, x2, y2]) */
    fb_data *element = NULL;
    unsigned val;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x1 = luaL_optnumber(L, 2, 1);
    int y1 = luaL_optnumber(L, 3, 1);
    int x2 = luaL_optnumber(L, 4, a->width);
    int y2 = luaL_optnumber(L, 5, a->height);

    luaL_argcheck(L, x1 > 0 && x1 <= a->width,  2, ERR_IDX_RANGE);
    luaL_argcheck(L, y1 > 0 && y1 <= a->height, 3, ERR_IDX_RANGE);
    luaL_argcheck(L, x2 > 0 && x2 <= a->width,  4, ERR_IDX_RANGE);
    luaL_argcheck(L, y2 > 0 && y2 <= a->height, 5, ERR_IDX_RANGE);

    for(int y = y1; y <= y2; y++)
    {
        for(int x = x1; x <= x2; x++)
        {
            element = data_element(a->data, a->elems, x, y, a->stride);

            luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

            val = FB_UNPACK_SCALAR_LCD(data_get(element, x, y));
        #if defined(HAVE_LCD_COLOR)
            val = invert_rgb(val);
        #else
            val = ~val;
        #endif
            data_set(element, x, y, FB_SCALARPACK((val)));
        }
    }

    return 0;
}

static int rli_blit(lua_State *L)
{
    /*(dst*, src*, [d_x1, d_y1, s_x2, s_y2, x_off, y_off, operation, clr]) */
    fb_data *dst_element;
    fb_data *src_element;
    fb_data src_val;
    fb_data dst_val;
    int x, y, dst_x, dst_y, src_x, src_y, w, h;

    struct rocklua_image *d = rli_checktype(L, 1); /*dst*/
    struct rocklua_image *s = rli_checktype(L, 2); /*src*/

    int d_x1     = luaL_optnumber(L, 3, 1);
    int d_y1     = luaL_optnumber(L, 4, 1);
    int s_x2     = luaL_optnumber(L, 5, 1);
    int s_y2     = luaL_optnumber(L, 6, 1);
    int x_off    = luaL_optnumber(L, 7, 0);
    int y_off    = luaL_optnumber(L, 8, 0);
    const int op = luaL_optnumber(L, 9, 0);

#ifdef HAVE_LCD_COLOR
    fb_data clr = FB_SCALARPACK((unsigned)luaL_optnumber(L, 10, TRANSPARENT_COLOR));
#else
    fb_data clr = FB_SCALARPACK((unsigned)luaL_optnumber(L, 10, 0));
#endif

    luaL_argcheck(L, d_x1 > 0 && d_x1 <= d->width,  3, ERR_IDX_RANGE);
    luaL_argcheck(L, d_y1 > 0 && d_y1 <= d->height, 4, ERR_IDX_RANGE);
    luaL_argcheck(L, s_x2 > 0 && s_x2 <= s->width,  5, ERR_IDX_RANGE);
    luaL_argcheck(L, s_y2 > 0 && s_y2 <= s->height, 6, ERR_IDX_RANGE);

    w = MIN(d->width  - d_x1, s->width  - s_x2 );
    h = MIN(d->height - d_y1, s->height - s_y2 );

    if(!lua_isnoneornil(L, 7))
        w = MIN(w, ABS(x_off));
    if(!lua_isnoneornil(L, 8))
        h = MIN(h, ABS(y_off));

    if (x_off < 0)
        d_x1 += w;
    if (y_off < 0)    
        d_y1 += h;

    y = h;
    do
    {
        x = w;
        do
        {
            dst_x = d_x1 + ((x_off < 0)? -x : x);
            dst_y = d_y1 + ((y_off < 0)? -y : y);
            
            src_x = s_x2 + x;
            src_y = s_y2 + y;

            dst_element = data_element(d->data, d->elems,
                                            dst_x, dst_y,
                                              d->stride);

            src_element = data_element(s->data, s->elems,
                                            src_x, src_y,
                                              s->stride);

            luaL_argcheck(L, src_element != NULL && dst_element != NULL,
                                                       1, ERR_DATA_OVF);

            src_val =  data_get(src_element, src_x, src_y);

            dst_val = data_get(dst_element, dst_x, dst_y);

            switch (op)
            {
                case -1:/* allow user to define their own blit operation */
                {
                    if (!lua_isnoneornil(L, 11))
                    {
                        lua_pushvalue(L, 11); /* lua function */
                        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(dst_val));
                        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(src_val));
                        lua_pushnumber(L, x);
                        lua_pushnumber(L, y);

                        luaL_argcheck(L, lua_pcall(L, 4, 1, 0) == 0, 11, 
                                       "error calling custom function");

                        dst_val = FB_SCALARPACK((unsigned)lua_tointeger(L, -1));
                        lua_pop(L, 1);
                        break;
                    }
                }
                default:
                case 0:  { dst_val =  src_val;               break; }/*copy*/
                case 1:  { dst_val |= src_val;               break; }/*or  */
                case 2:  { dst_val ^= src_val;               break; }/*xor */
                case 3:  { dst_val = ~(dst_val | src_val);   break; }/*nor */
                case 4:  { dst_val &= src_val;               break; }/*and */
                case 5:  { dst_val = ~(dst_val & src_val);   break; }/*nand*/
                case 6:  { dst_val = ~src_val;               break; }/*not */
                case 7:  { if(src_val != 0) { dst_val = clr; } break; }
                case 8:  { if(src_val == 0) { dst_val = clr; } break; }
#if defined(HAVE_LCD_COLOR) || (LCD_DEPTH > 1)
                case 9:  { if(src_val != clr) { dst_val = src_val; } break; }
                case 10: { if(src_val == clr) { dst_val = src_val; } break; }
                case 11: { if(src_val > clr)  { dst_val = src_val; } break; }
                case 12: { if(src_val < clr)  { dst_val = src_val; } break; }
                case 13: { if(dst_val != clr) { dst_val = src_val; } break; }
                case 14: { if(dst_val == clr) { dst_val = src_val; } break; }
                case 15: { if(dst_val > clr)  { dst_val = src_val; } break; }
                case 16: { if(dst_val < clr)  { dst_val = src_val; } break; }
                case 17: { if(dst_val != src_val) { dst_val = clr; } break; }
                case 18: { if(dst_val == src_val) { dst_val = clr; } break; }
                case 19: { if(dst_val > src_val)  { dst_val = clr; } break; }
                case 20: { if(dst_val < src_val)  { dst_val = clr; } break; }
#endif
            } /*switch op*/

            data_set(dst_element, dst_x, dst_y, dst_val);

        } while(x--);

    } while(y--);

    return 0;
}

static int rli_copy(lua_State *L)
{
    /*(dst*, src*, [d_x1, d_y1, s_x2, s_y2, x_off, y_off]) */
    fb_data *dst_element;
    fb_data *src_element;
    int x, y, dst_x, dst_y, src_x, src_y, w, h;

    struct rocklua_image *d = rli_checktype(L, 1); /*dst*/
    struct rocklua_image *s = rli_checktype(L, 2); /*src*/

    /* copy whole image if possible */
    if(s->bytes == d->bytes && s->width == d-> width && lua_gettop(L) < 3)
    {
        memcpy(d->data, s->data, d->bytes);
        return 0;
    }

    int d_x1  = luaL_optnumber(L, 3, 1);
    int d_y1  = luaL_optnumber(L, 4, 1);
    int s_x2  = luaL_optnumber(L, 5, 1);
    int s_y2  = luaL_optnumber(L, 6, 1);
    int x_off = luaL_optnumber(L, 7, 0);
    int y_off = luaL_optnumber(L, 8, 0);

    luaL_argcheck(L, d_x1 > 0 && d_x1 <= d->width,  3, ERR_IDX_RANGE);
    luaL_argcheck(L, d_y1 > 0 && d_y1 <= d->height, 4, ERR_IDX_RANGE);
    luaL_argcheck(L, s_x2 > 0 && s_x2 <= s->width,  5, ERR_IDX_RANGE);
    luaL_argcheck(L, s_y2 > 0 && s_y2 <= s->height, 6, ERR_IDX_RANGE);

    w = MIN(d->width - d_x1, s->width - s_x2 );
    h = MIN(d->height - d_y1, s->height - s_y2 );

    if(!lua_isnoneornil(L, 7))
        w = MIN(w, ABS(x_off));
    if(!lua_isnoneornil(L, 8))
        h = MIN(h, ABS(y_off));

    if (x_off < 0)
        d_x1 += w;
    if (y_off < 0)    
        d_y1 += h;

    y = h;
    do
    {
        x = w;
        do
        {
            dst_x = d_x1 + ((x_off < 0)? -x : x);
            dst_y = d_y1 + ((y_off < 0)? -y : y);

            src_x = s_x2 + x;
            src_y = s_y2 + y;

            dst_element = data_element(d->data, d->elems,
                                            dst_x, dst_y, 
                                              d->stride);

            src_element = data_element(s->data, s->elems,
                                            src_x, src_y,
                                              s->stride);

            luaL_argcheck(L, src_element != NULL && dst_element != NULL,
                          1, ERR_DATA_OVF);

            data_set(dst_element, dst_x, dst_y,
                     data_get(src_element, src_x, src_y));

        } while(x--);

    } while(y--);

    return 0;
}

static int rli_data(lua_State *L)
{
    /*val = (src*, index, [new_val]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    size_t i = (unsigned) (luaL_checkint(L, 2));
    int v = luaL_optnumber(L, 3, -1);
    fb_data val;

    luaL_argcheck(L, 1U <= i && i <= (a->elems), 2, ERR_IDX_RANGE);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(a->data[i-1]));

    if (v >= 0)
    {
        val = FB_SCALARPACK((unsigned) luaL_checkint(L, 3));
        a->data[i-1] = val;
    }
    
    return 1;
}

static const struct luaL_reg rli_lib [] =
{
    {"_data", rli_data},
    {"__tostring", rli_tostring},
    {"__len",  rli_size},
    {"width",  rli_width},
    {"height", rli_height},
    {"set",    rli_set},
    {"get",    rli_get},
    {"blit",   rli_blit},
    {"copy",   rli_copy},
    {"clear",  rli_clear},
    {"invert", rli_invert},
    {"draw_line", rli_line},
    {"draw_ellipse_rect", rli_ellipse_rect},
    {NULL, NULL}
};

static inline void rli_init(lua_State *L)
{

    if(LCD_PIXELFORMAT == HORIZONTAL_PACKING)
    {
        if(LCD_DEPTH == 2)
        {
            x_shift = 2U;
            pixmask = (fb_data *) pixmask_h2;
            /* MSB on left*/
        }
    }
    else if(LCD_PIXELFORMAT == VERTICAL_PACKING)
    {
        if(LCD_DEPTH == 1)
        {
            y_shift = 3U;
            pixmask = (fb_data *) pixmask_v1;
        }
        else if(LCD_DEPTH == 2)
        {
            y_shift = 2U;
            pixmask = (fb_data *) pixmask_v2;
        }
    }
    else if(LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    {
        if(LCD_DEPTH == 2)
        {
            y_shift = 3U;
            pixmask = (fb_data *) pixmask_vi2;
        }
    }

    if(y_shift)
        xy_mask = ((1 << y_shift) -1);
    else if(x_shift)
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
static void check_tableval(lua_State *L, const char* key, int tablepos, void* res, bool unsigned_val)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    if(!lua_isnoneornil(L, -1))
    {
        if(unsigned_val)
            *(unsigned*)res = luaL_checkint(L, -1);
        else
            *(int*)res = luaL_checkint(L, -1);
    }

    lua_pop(L, 1); /* Pop the val off the stack */
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
        memset(vp, 0, sizeof(struct viewport)); /* Init viewport vals to 0 */
        lua_setfield(L, tablepos, "vp"); /* table['vp'] = vp (pops val off the stack) */
    }
    else
    {
        vp = (struct viewport*) lua_touserdata(L, -1); /* Reuse viewport struct */
        lua_pop(L, 1); /* We don't need the val on stack */
    }

    luaL_checktype(L, narg, LUA_TTABLE);

    check_tableval(L, "x", tablepos, &vp->x, false);
    check_tableval(L, "y", tablepos, &vp->y, false);
    check_tableval(L, "width", tablepos, &vp->width, false);
    check_tableval(L, "height", tablepos, &vp->height, false);
#ifdef HAVE_LCD_BITMAP
    check_tableval(L, "font", tablepos, &vp->font, false);
    check_tableval(L, "drawmode", tablepos, &vp->drawmode, false);
#endif
#if LCD_DEPTH > 1
    check_tableval(L, "fg_pattern", tablepos, &vp->fg_pattern, true);
    check_tableval(L, "bg_pattern", tablepos, &vp->bg_pattern, true);
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

    if(fontnumber == FONT_UI)
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

    if(dither)
        format |= FORMAT_DITHER;

    if(transparent)
        format |= FORMAT_TRANSPARENT;

    int result = rb->read_bmp_file(filename, &bm, 0, format | FORMAT_RETURN_SIZE, NULL);

    if(result > 0)
    {
        bm.data = (unsigned char*) rli_alloc(L, bm.width, bm.height);
        if(rb->read_bmp_file(filename, &bm, result, format, NULL) < 0)
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
    if(current_path != NULL)
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

    tlsf_free(main_message.message_lines);
    if(yes)
        tlsf_free(yes_message.message_lines);
    if(no)
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
    if(with_remote)
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
