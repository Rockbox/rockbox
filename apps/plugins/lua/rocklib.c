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
#ifndef ABS
#define ABS(a)(((a) < 0) ? - (a) :(a))
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

struct rli_iter_d
{
    struct rocklua_image *img;
    fb_data *elem;
    int x , y;
    int x1, y1;
    int x2, y2;
    int dx, dy;
};

#ifdef HAVE_LCD_COLOR

#ifndef LCD_BLACK
#define LCD_BLACK LCD_RGBPACK(0, 0, 0)
#endif

static inline unsigned invert_color(unsigned rgb)
{
    uint8_t r = 0xFFU - RGB_UNPACK_RED((unsigned) rgb);
    uint8_t g = 0xFFU - RGB_UNPACK_GREEN((unsigned) rgb);
    uint8_t b = 0xFFU - RGB_UNPACK_BLUE((unsigned) rgb);

    return LCD_RGBPACK(r, g, b);
}
#else

#ifndef LCD_BLACK
#define LCD_BLACK 0x0
#endif

#define invert_color(c) (~c)
#endif /* HAVE_LCD_COLOR */

#if (LCD_DEPTH > 2) /* no native to pixel mapping needed */

#define pix_to_native(x, y, xn, yn) \
    do { } while (0)
#define init_pixmask(x, y, m, p) \
    do { } while (0)
#define pix_to_fb(x, y, o, n) \
    do { } while (0)

#else /* some devices need x | y coords shifted to match native format */

static fb_data x_shift = 0;
static fb_data y_shift = 0;
static fb_data xy_mask = 0;
static const fb_data *pixmask = NULL;

static inline void init_pixmask(fb_data *x_shift, fb_data *y_shift,
                                fb_data *xy_mask, const fb_data **pixmask)
{
/* conversion between packed native formats and individual pixel addressing */

#if(LCD_PIXELFORMAT == VERTICAL_PACKING) && LCD_DEPTH == 1
    static const fb_data pixmask_v1[8] =
        {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    *y_shift = 3U;
    *pixmask = pixmask_v1;
#elif(LCD_PIXELFORMAT == VERTICAL_PACKING) && LCD_DEPTH == 2
    static const fb_data pixmask_v2[8] = {0x03, 0x0C, 0x30, 0xC0, 0, 0, 0, 0};
    *y_shift = 2U;
    *pixmask = pixmask_v2;
#elif(LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    static const fb_data pixmask_vi2[8] =
        {0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};
    *y_shift = 3U;
    *pixmask = pixmask_vi2;
#elif(LCD_PIXELFORMAT == HORIZONTAL_PACKING) && LCD_DEPTH == 2
    static const fb_data pixmask_h2[8] = {0x03, 0x0C, 0x30, 0xC0, 0, 0, 0, 0};
        /* MSB on left */
    *x_shift = 2U;
    *pixmask = pixmask_h2;
#endif

    if(*y_shift)
        *xy_mask = ((1 << (*y_shift)) -1);
    else if(*x_shift)
        *xy_mask = ((1 << (*x_shift)) -1);
}

static inline void pix_to_native(int x, int y, int *x_native, int *y_native)
{
    /* Some devices(1-bit / 2-bit displays) have packed bit formats that need
       to be unpacked in order to work on them at a pixel level.
       The internal formats of these devices do not follow the same paradigm
       for image sizes either; We still display the actual width and height to
       the user but store stride and bytes based on the native values
    */
    if(x_shift)
        *x_native = ((x - 1) >> x_shift) + 1;
    else
        *x_native = x;

    if(y_shift)
        *y_native = ((y - 1) >> y_shift) + 1;
    else
        *y_native = y;
}

static inline fb_data set_masked_pix(fb_data old, fb_data new, fb_data mask, int bit_n)
{
        /*equivalent of:
          newvalue <<=bit_n;newvalue &= mask;newvalue |= oldvalue &= ~mask;
        */
        return old ^ ((old ^ (new << bit_n)) & mask);
}

static void pix_to_fb(int x, int y, fb_data *oldv, fb_data *newv)
{
/* conversion between packed native formats and individual pixel addressing */
    fb_data mask;
    int bit_n;

#if(LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    (void) x;
    const uint16_t greymap_vi2[4] = {0x0000, 0x0001, 0x0100, 0x0101};

    bit_n   = (y - 1) & xy_mask;
    mask    = pixmask[bit_n];
    if(newv)
    {
        *newv = greymap_vi2[*newv &(0x3)]; /* [0-3] => greymap */
        *newv = set_masked_pix(*oldv, *newv, mask, bit_n);
    }
    *oldv  &= mask;
    *oldv >>= (bit_n);

    if(((*oldv) & greymap_vi2[2]) != 0) /* greymap => [0-3] */
        *oldv = ((*oldv) & 0x1U) + 2U; /* 2, 3 */
    else
        *oldv &= 1U; /* 0, 1 */

#elif(LCD_DEPTH <= 2)

    if(y_shift)
        bit_n = (y - 1) & xy_mask;
    else if(x_shift)
        bit_n = xy_mask - ((x - 1) & xy_mask); /*MSB on left*/

    if(x_shift || y_shift)
    {
        mask = pixmask[bit_n];
        bit_n *= LCD_DEPTH;
        if(newv)
            *newv = set_masked_pix(*oldv, *newv, mask, bit_n);

        *oldv  &= mask;
        *oldv >>= (bit_n);
    }
#endif /* LCD_PIXELFORMAT == VERTICAL_INTERLEAVED */
}

#endif /* (LCD_DEPTH > 2) */

/* Internal worker functions for image data array *****************************/
static void bounds_check_xy(lua_State *L, struct rocklua_image *img,
                         int nargx, int x, int nargy, int y)
{
    luaL_argcheck(L, x > 0 && x <= img->width,  nargx, ERR_IDX_RANGE);
    luaL_argcheck(L, y > 0 && y <= img->height, nargy, ERR_IDX_RANGE);
}

static struct rocklua_image* rli_checktype(lua_State *L, int arg)
{
    void *ud = luaL_checkudata(L, arg, ROCKLUA_IMAGE);
    luaL_argcheck(L, ud != NULL, arg, "'" ROCKLUA_IMAGE "' expected");
    return(struct rocklua_image*) ud;
}

static void dim_rlimage(struct rocklua_image *a, int w, int h, int w_native, int h_native)
{
    a->stride = w_native;
    /* apparent w/h is stored but behind the scenes native w/h is used */
    a->width  = w;
    a->height = h;

    a->elems = (size_t)(w_native * h_native);
    a->bytes = a->elems * sizeof(fb_data);
}

static struct rocklua_image * alloc_rlimage_data(lua_State *L, int sz_bytes)
{
    struct rocklua_image *img = (struct rocklua_image *)lua_newuserdata(L, sz_bytes);
    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);
    return img;
}

static void rli_wrap(lua_State *L, fb_data *src, int width, int height)
{
    int w_native = width;
    int h_native = height;

    pix_to_native(width, height, &w_native, &h_native);

    size_t sz_header = sizeof(struct rocklua_image);

    struct rocklua_image *a = alloc_rlimage_data(L, sz_header);

    dim_rlimage(a, width, height, w_native, h_native);

    a->data = src;
}

static fb_data* rli_alloc(lua_State *L, int width, int height)
{
    int w_native = width;
    int h_native = height;

    pix_to_native(width, height, &w_native, &h_native);

    size_t sz_header = sizeof(struct rocklua_image);
    size_t nbytes = sz_header + ((w_native * h_native) - 1) * sizeof(fb_data);

    struct rocklua_image *a = alloc_rlimage_data(L, nbytes);

    dim_rlimage(a, width, height, w_native, h_native);

    a->data = &a->dummy[0][0];

    return a->data;
}

static inline fb_data* data_element(fb_data *data,
                                  size_t elements,
                                     int x, int y,
                                       int stride)
{
    pix_to_native(x, y, &x, &y);

    size_t data_address = (stride * (y - 1)) + (x - 1);

    if(data_address >= elements || x > stride)
        return NULL; /* data overflow */

    return &data[data_address]; /* return element address */
}

static inline fb_data data_set(fb_data *data_elem, int x, int y, fb_data new_val)
{
    (void) x; /* supress warnings when (LCD_DEPTH > 2) */
    (void) y;

    if(!data_elem)
        return 0;

    fb_data old_val = *data_elem;

    pix_to_fb(x, y, &old_val, &new_val);

    *data_elem = new_val;

    return old_val;
}

static inline fb_data data_get(fb_data *data_elem, int x, int y)
{
    (void) x;  /* supress warnings when (LCD_DEPTH > 2) */
    (void) y;

    if(!data_elem)
        return 0;

    fb_data val = *data_elem;

    pix_to_fb(x, y, &val, NULL);

    return val;
}

static bool init_rli_iter(struct rli_iter_d *d, struct rocklua_image *img,
                                                int x1, int y1,
                                                int x2, int y2,
                                                int dx, int dy,
                                            bool swx, bool swy)
{
    int swap;
    if(swx)
    {
        swap = x1;
        x1 = x2;
        x2 = swap;
    }
    if(swy)
    {
        swap = y1;
        y1 = y2;
        y2 = swap;
    }

    /* stepping in the correct direction ? */
    if((x1 > x2 && dx > 0) || (x1 < x2 && dx < 0))
        dx = -dx;
    if((y1 > y2 && dy > 0) || (y1 < y2 && dy < 0))
        dy = -dy;

    d->img = img;
    d->x   = x1;
    d->y   = y1;
    d->x1  = x1;
    d->y1  = y1;
    d->x2  = x2;
    d->y2  = y2;
    d->dx  = dx;
    d->dy  = dy;
    d->elem = data_element(img->data, img->elems, d->x, d->y, img->stride);
    return(d->dx != 0 || d->dy != 0);
}

static bool next_rli_iter(struct rli_iter_d *d)
{
    /* steps to the next point(x, y) by delta x/y, stores pointer to element
       returns true if x & y haven't reached x2 & y2
       if limit reached - element set to NULL, deltas set to 0 & false returned
    */
    struct rocklua_image *img = d->img;
    bool ret = true;
    if((d->dx > 0 && d->x < d->x2) || (d->dx < 0 && d->x > d->x2))
    {
        d->x += d->dx;
        d->elem =  data_element(img->data, img->elems, d->x, d->y, img->stride);
    }
    else if((d->dy > 0 && d->y < d->y2) || (d->dy < 0 && d->y > d->y2))
    {
        d->x = d->x1; /* Reset x*/
        d->y += d->dy;
        d->elem =  data_element(img->data, img->elems, d->x, d->y, img->stride);
    }
    else
    {
        ret = false;
        d->elem = NULL;
        d->dx = 0;
        d->dy = 0;
    }

    return ret;
}

static int d_line(struct rocklua_image *img, int x1, int y1, int x2, int y2,
                                              fb_data clr, bool clip)
{
    /* Bresenham midpoint line algorithm */
    fb_data *element;

    int d_a = x2 - x1; /* delta of x direction called 'a' for now */
    int d_b = y2 - y1; /* delta of y direction called 'b' for now */

    int s_a = 1; /* step of a direction */
    int s_b = 1; /* step of b direction */

    int swap;
    int d_err;
    int numpixels;

    int *a1 = &x1; /* pointer to the x var 'a' */
    int *b1 = &y1; /* pointer to the y var 'b' */

    if(d_a < 0) /* instead of negative delta we will switch step instead */
    {
        d_a = -d_a;
        s_a = -s_a;
    }

    if(d_b < 0) /* instead of negative delta we will switch step instead */
    {
        d_b = -d_b;
        s_b = -s_b;
    }

    x2 += s_a; /* add 1 extra point to make the whole line */
    y2 += s_b; /* add 1 extra point */

    if(d_b > d_a) /*if deltaY('b') > deltaX('a') swap their identities */
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

    d_err = ((d_b << 1) - d_a) >> 1; /* preload err of 1 step (px centered) */
    numpixels = d_a + 1;
    d_a -= d_b; /* 'a' - 'b' v See below v */

    for(;numpixels > 0; numpixels--)
    {
        element = data_element(img->data, img->elems, x1, y1, img->stride);
        if(clip || element)
            data_set(element, x1, y1, clr);
        else
            return 1;

        if(d_err >= 0) /* 0 is our target midpoint(exact point on the line) */
        {
            *b1 += s_b; /* whichever axis is in 'b' stepped(-1 or 1) */
            d_err -= d_a;
        }
        else
            d_err += d_b; /* 'a' - 'b' allows to only add 'b' when d_err < 0 */

        *a1 += s_a; /* whichever axis is in 'a' stepped(-1 or 1) */
    }

    return 0;
}

static int d_ellipse_rect(struct rocklua_image *img, int x1, int y1, int x2, int y2,
                                   fb_data *clr, fb_data *fillclr, bool clip)
{
    /* NOTE! clr and fillclr passed as pointers unlike the other functions. */
    /* Rasterizing algorithm derivative of work by Alois Zing */
#if LCD_WIDTH > 1024 || LCD_HEIGHT > 1024
    /* Prevents overflow on large screens */
    double dx, dy, err, e2;
#else
    long dx, dy, err, e2;
#endif
    fb_data *element1;
    fb_data *element2;
    fb_data *element3;
    fb_data *element4;
    int ret = 0;

    int a = ABS(x2 - x1); /* diameter */
    int b = ABS(y2 - y1); /* diameter */

    int b1 = (b & 1);
    b = b - (1 - b1);

    int a2 = (a * a);
    int b2 = (b * b);

    if(!clr || a == 0 || b == 0)
        return 0; /* not an error but nothing to display */

    dx = ((1 - a) * b2) >> 1; /* error increment */
    dy = (b1 * a2) >> 1;      /* error increment */

    err = dx + dy + b1 * a2; /* error of 1.step */

    if(x1 > x2) /* if called with swapped points .. exchange them */
    {
        x1 = x2;
        x2 += a;
    }

    if(y1 > y2) /* if called with swapped points .. exchange them */
        y1 = y2;

    y1 += (b + 1) >> 1;
    y2 = y1 - b1;

    do {
        element1 = data_element(img->data, img->elems, x2, y1, img->stride);
        element2 = data_element(img->data, img->elems, x1, y1, img->stride);
        element3 = data_element(img->data, img->elems, x1, y2, img->stride);
        element4 = data_element(img->data, img->elems, x2, y2, img->stride);
        if(clip || (element1 && element2 && element3 && element4))
        {
            data_set(element1, x2, y1, *clr); /*   I. Quadrant +x +y */
            data_set(element2, x1, y1, *clr); /*  II. Quadrant -x +y */
            data_set(element3, x1, y2, *clr); /* III. Quadrant -x -y */
            data_set(element4, x2, y2, *clr); /*  IV. Quadrant +x -y */
            if(fillclr && x1 != x2)
            {
                ret |= d_line(img, x1 + 1, y1, x2 - 1, y1, *fillclr, clip); /* I. II.*/
                ret |= d_line(img, x1 + 1, y2, x2 - 1, y2, *fillclr, clip); /* III.IV.*/
            }
        }
        else
            return 1; /* ERROR */

        e2 = err;

        if(e2 <= (dy >> 1)) /* target midpoint - y step */
        {
            y1++;
            y2--;
            dy += a2;
            err += dy;
        }

        if(e2 >= (dx >> 1) || err > (dy >> 1)) /* target midpoint - x step */
        {
            x1++;
            x2--;
            dx += b2;
            err += dx;
        }

    } while(x1 <= x2);

    while (y1 - y2 < b) /* for early stop of flat ellipse a=1 finish tip */
    {
        element1 = data_element(img->data, img->elems, x2 + 1, y1, img->stride);
        element2 = data_element(img->data, img->elems, x1 - 1, y1, img->stride);
        element3 = data_element(img->data, img->elems, x1 - 1, y2, img->stride);
        element4 = data_element(img->data, img->elems, x2 + 1, y2, img->stride);
        if (clip || (element1 && element2 && element3 && element4))
        {
            data_set(element1, x2 + 1, y1, *clr);
            data_set(element2, x1 - 1, y1, *clr);
            data_set(element3, x1 - 1, y2, *clr);
            data_set(element4, x2 + 1, y2, *clr);
        }
        else
            return 2; /* ERROR */

        y1++;
        y2--;
    }

    return ret;
}

/* User defined pixel manipulations through rli_copy */
static int custom_op(lua_State *L, struct rli_iter_d *ds, struct rli_iter_d *ss, int op, fb_data *c)
{
    (void) c;
    int ret;
    fb_data dst = data_get(ds->elem, ds->x, ds->y);
    fb_data src = data_get(ss->elem, ss->x, ss->y);
    lua_rawgeti(L, LUA_REGISTRYINDEX, op);
    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(dst));
    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(src));
    lua_pushnumber(L, ds->x);
    lua_pushnumber(L, ds->y);
    lua_pushnumber(L, ss->x);
    lua_pushnumber(L, ss->y);
    lua_call(L, 6, 2); /* calling custom function w/ 6 vars 2 ret*/

    ret = lua_isnoneornil(L, -2);

    dst = FB_SCALARPACK((unsigned)luaL_optnumber(L, -2, dst));
    data_set(ds->elem, ds->x, ds->y, dst);

    if(lua_isnoneornil(L, -1) == 0)
    {
        src = FB_SCALARPACK((unsigned)luaL_optnumber(L, -1, src));
        data_set(ss->elem, ss->x, ss->y, src);
    }

    lua_pop(L, 2);
    return ret;
}

/* Pre defined pixel manipulations through rli_copy */
static int blit_op(lua_State *L, struct rli_iter_d *ds, struct rli_iter_d *ss, int op, fb_data *c)
{
    (void) L;
    fb_data clr = *c;
    fb_data dst = data_get(ds->elem, ds->x, ds->y);
    fb_data src = data_get(ss->elem, ss->x, ss->y);
    switch(op)
    {
        default:
        case 0:  { dst  =  src;         break; }/*copyS*/
        case 1:  { dst |= src;          break; }/*DorS  */
        case 2:  { dst ^= src;          break; }/*DxorS */
        case 3:  { dst  = ~(dst | src); break; }/*nDorS */
        case 4:  { dst  = dst | (~src); break; }/*DornS*/
        case 5:  { dst &= src;          break; }/*DandS */
        case 6:  { dst  = (~dst) & src; break; }/*nDandS*/
        case 7:  { dst  = ~src;         break; }/*notS */
        /* mask blits */
        case 8:  { if(src != 0) { dst = clr; } break; }/*Sand*/
        case 9:  { if(src == 0) { dst = clr; } break; }/*Snot*/

        case 10:  { dst = src | clr;             break; }/*SorC*/
        case 11:  { dst = src ^ clr;             break; }/*SxorC*/
        case 12:  { dst = ~(src | clr);          break; }/*nSorC*/
        case 13:  { dst = src | (~clr);          break; }/*SornC*/
        case 14:  { dst = src & clr;             break; }/*SandC*/
        case 15:  { dst = (~src) & clr;          break; }/*nSandC*/
        case 16:  { dst |= (~src) | clr;         break; }/*DornSorC*/
        case 17:  { dst ^= (src & (dst ^ clr));  break; }/*DxorSandDxorC*/

        case 18: { if(src != clr) { dst = src; } break; }
        case 19: { if(src == clr) { dst = src; } break; }
        case 20: { if(src > clr)  { dst = src; } break; }
        case 21: { if(src < clr)  { dst = src; } break; }
        case 22: { if(dst != clr) { dst = src; } break; }
        case 23: { if(dst == clr) { dst = src; } break; }
        case 24: { if(dst > clr)  { dst = src; } break; }
        case 25: { if(dst < clr)  { dst = src; } break; }
        case 26: { if(dst != src) { dst = clr; } break; }
        case 27: { if(dst == src) { dst = clr; } break; }
        case 28: { if(dst > src)  { dst = clr; } break; }
        case 29: { if(dst < src)  { dst = clr; } break; }

        /* src isn't actually needed for these blits */
        case 30:  { dst =  clr;         break; }/*copyC*/
        case 31:  { dst |= clr;         break; }/*DorC  */
        case 32:  { dst ^= clr;         break; }/*DxorC */
        case 33:  { dst = ~(dst | clr); break; }/*nDorC */
        case 34:  { dst = dst | (~clr); break; }/*DornC*/
        case 35:  { dst &= clr;         break; }/*DandC */
        case 36:  { dst = (~dst) & clr; break; }/*nDandC*/
        case 37:  { dst = ~clr;         break; }/*notC */
    }/*switch op*/
    data_set(ds->elem, ds->x, ds->y, dst);
    return 0;
}

/* Standard transformation of rli_copy */
static int copy_op(lua_State *L, struct rli_iter_d *ds, struct rli_iter_d *ss, int op, fb_data *c)
{
    (void) L;
    (void) c;
    (void) op;
    data_set(ds->elem, ds->x, ds->y, data_get(ss->elem, ss->x, ss->y));
    return 0;
}

/* LUA Interface functions ****************************************************/
static int rli_new(lua_State *L)
{   /* [width, height] */
    int width  = luaL_optint(L, 1, LCD_WIDTH);
    int height = luaL_optint(L, 2, LCD_HEIGHT);

    luaL_argcheck(L, width  > 0, 1, ERR_IDX_RANGE);
    luaL_argcheck(L, height > 0, 2, ERR_IDX_RANGE);

    rli_alloc(L, width, height);

    return 1;
}

static int rli_ellipse_rect(lua_State *L)
{
    /* (dst*, x1, y1, x2, y2, [clr, fillclr, clip]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_checkint(L, 4);
    int y2 = luaL_checkint(L, 5);
    fb_data clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, LCD_BLACK));
    fb_data fillclr; /* fill color is index 7 */
    bool clip = luaL_optboolean(L, 8, false);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    if(!lua_isnoneornil(L, 7))
    {
        fillclr = FB_SCALARPACK((unsigned) luaL_checkint(L, 7));
        luaL_argcheck(L, d_ellipse_rect(a, x1, y1, x2, y2, &clr, &fillclr, clip) == 0,
                      1, ERR_DATA_OVF);
    }
    else /* border is the same as the fill */
    {
        luaL_argcheck(L, d_ellipse_rect(a, x1, y1, x2, y2, &clr, NULL, clip) == 0,
                      1, ERR_DATA_OVF);
    }

    return 0;
}

static int rli_line(lua_State *L)
{
    /* (dst*, x1, y1, [x2, y2, clr, clip]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_optint(L, 4, x1);
    int y2 = luaL_optint(L, 5, y1);
    fb_data clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 6, LCD_BLACK));
    bool clip = luaL_optboolean(L, 7, false);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    luaL_argcheck(L, d_line(a, x1, y1, x2, y2, clr, clip) == 0, 1, ERR_DATA_OVF);
    return 0;
}

static int rli_iterator(lua_State *L) {

    struct rli_iter_d *ds = (struct rli_iter_d *)lua_touserdata(L, lua_upvalueindex(1));

    if(ds->dx == 0 && ds->dy == 0)
        return 0; /* nothing left to do */

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(ds->elem, ds->x, ds->y)));
    lua_pushinteger(L, ds->x);
    lua_pushinteger(L, ds->y);

    /* load the next element */
    next_rli_iter(ds);

    return 3;
}

static int rli_iterator_factory(lua_State *L) {
    struct rli_iter_d *ds;

    struct rocklua_image *a = rli_checktype(L, 1); /*image we wish to iterate*/
    int x1  = luaL_optint(L, 2, 1);
    int y1  = luaL_optint(L, 3, 1);
    int x2  = luaL_optint(L, 4, a->width  - x1 + 1);
    int y2  = luaL_optint(L, 5, a->height - y1 + 1);
    int dx  = luaL_optint(L, 6, 1);
    int dy  = luaL_optint(L, 7, 1);

    ds = (struct rli_iter_d *) lua_newuserdata(L, sizeof(struct rli_iter_d));

    init_rli_iter(ds, a, x1, y1, x2, y2, dx, dy, false, false);
    /* returns the iter function with embedded iter data(up values) */
    lua_pushcclosure(L, &rli_iterator, 1);

    return 1;
}
static int rli_invert(lua_State *L) /* also marshal */
{
    /* (dst*, [x1, y1, x2, y2, clip, function]) */
    struct rli_iter_d ds;
    bool d_act;
    unsigned val;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x1 = luaL_optint(L, 2, 1);
    int y1 = luaL_optint(L, 3, 1);
    int x2 = luaL_optint(L, 4, a->width);
    int y2 = luaL_optint(L, 5, a->height);
    int dx = luaL_optint(L, 6, 1);
    int dy = luaL_optint(L, 7, 1);
    bool clip    = luaL_optboolean(L, 8, false);
    bool c_funct = lua_isfunction(L, 9);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    d_act = init_rli_iter(&ds, a, x1, y1, x2, y2, dx, dy, false, false);

    do
    {
        luaL_argcheck(L, clip || (ds.elem != NULL), 1, ERR_DATA_OVF);

        val = FB_UNPACK_SCALAR_LCD(data_get(ds.elem, ds.x, ds.y));

        if(c_funct) /* custom function*/
            {
                luaL_argcheck(L, lua_checkstack (L, 4), 9, ERR_DATA_OVF);
                lua_pushvalue(L, 9); /* lua function */
                lua_pushinteger(L, val);
                lua_pushinteger(L, ds.x);
                lua_pushinteger(L, ds.y);
                lua_call(L, 3, 1); /* calling custom function w/ 3 vars 1 ret*/

                if(lua_isnoneornil(L, -1)) /* exit marshal early */
                    return 0;

                val = FB_SCALARPACK((unsigned)lua_tonumber(L, -1));
                lua_pop(L, 1);
            }
            else
                val = invert_color(val);

            data_set(ds.elem, ds.x, ds.y, FB_SCALARPACK((val)));

            d_act = next_rli_iter(&ds);

    } while(d_act);

    return 0;
}

static int rli_clear(lua_State *L)
{
    /* (dst*, [color, x1, y1, x2, y2, clip]) */
    struct rli_iter_d ds;
    bool d_act;

    struct rocklua_image *a = rli_checktype(L, 1);
    fb_data clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 2, LCD_BLACK));
    int x1 = luaL_optint(L, 3, 1);
    int y1 = luaL_optint(L, 4, 1);
    int x2 = luaL_optint(L, 5, a->width);
    int y2 = luaL_optint(L, 6, a->height);
    bool clip = luaL_optboolean(L, 7, false);

    if(!clip)
    {
        bounds_check_xy(L, a, 3, x1, 4, y1);
        bounds_check_xy(L, a, 5, x2, 6, y2);
    }

    d_act = init_rli_iter(&ds, a, x1, y1, x2, y2, 1, 1, false, false);
    do
    {
        luaL_argcheck(L, clip || (ds.elem != NULL), 1, ERR_DATA_OVF);

        data_set(ds.elem, ds.x, ds.y, clr);

        d_act = next_rli_iter(&ds);

    } while(d_act);

    return 0;
}

static int rli_copy(lua_State *L)
{
    /* (dst*, src*, [d_x, d_y, s_x, s_y, x_off, y_off, clip, [op, funct/clr]]) */
    struct rli_iter_d ds;
    bool d_act;

    struct rli_iter_d ss;
    bool s_act;

    int w, h;
    fb_data clr = 0;

    typedef int (*trans)(lua_State *, struct rli_iter_d *, struct rli_iter_d *, int, fb_data *);
    trans transform = copy_op; /* copy is the default transformation */

    int lf_ref = LUA_NOREF; /* de-ref'd without consequence */

    bool d_swx = false; /* dest swap x */
    bool d_swy = false; /* dest swap y */
    bool s_swx = false; /* src  swap x */
    bool s_swy = false; /* src  swap y */

    struct rocklua_image *d = rli_checktype(L, 1); /*dst*/
    struct rocklua_image *s = rli_checktype(L, 2); /*src*/

    /* copy whole image if possible */
    if(s->bytes == d->bytes && s->width == d-> width && lua_gettop(L) < 3)
    {
        memcpy(d->data, s->data, d->bytes);
        return 0;
    }

    int d_x = luaL_optint(L, 3, 1);
    int d_y = luaL_optint(L, 4, 1);
    int s_x = luaL_optint(L, 5, 1);
    int s_y = luaL_optint(L, 6, 1);
    w = MIN(d->width - d_x, s->width - s_x);
    h = MIN(d->height - d_y, s->height - s_y);
    int x_off = luaL_optint(L, 7, w);
    int y_off = luaL_optint(L, 8, h);
    bool clip = luaL_optboolean(L, 9, false);

    int op = luaL_optint(L, 10, 0);

    if (!lua_isfunction(L, 11))
    {
        luaL_argcheck(L, op != 0xFF, 11, "function expected"); /* op = 0xFF */
        clr = FB_SCALARPACK((unsigned)luaL_optnumber(L, 11, LCD_BLACK));
    }

    if (op == 0xFF) /* custom function specified.. */
    {
        luaL_argcheck(L, lua_checkstack (L, 1), 11, ERR_DATA_OVF);
        transform = custom_op;
        lua_pushvalue(L, 11); /* lua function */
        lf_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_settop(L, 0); /* clear C stack for use by custom function */
        op = lf_ref;
    }
    else if (op != 0)
        transform = blit_op;

    if(!clip) /* Out of bounds is not allowed */
    {
        bounds_check_xy(L, d, 3, d_x, 4, d_y);
        bounds_check_xy(L, s, 5, s_x, 6, s_y);
    }
    else if (w < 0 || h < 0)
    {   /* not an error but nothing to display */
        luaL_unref(L, LUA_REGISTRYINDEX, lf_ref); /* de-reference custom function */
        return 0;
    }

    w = MIN(w, ABS(x_off));
    h = MIN(h, ABS(y_off));

    d_swx = (x_off < 0);
    d_swy = (y_off < 0);

    if(s->data == d->data) /* if src == dest need to care about fill direction*/
    {
        if(d_x > s_x)
        {
            d_swx = !d_swx;
            s_swx = !s_swx;
        }
        if(d_y > s_y)
        {
            d_swy = !d_swy;
            s_swy = !s_swy;
        }
    }

    d_act = init_rli_iter(&ds, d, d_x, d_y, d_x + w, d_y + h, 1, 1, d_swx, d_swy);

    s_act = init_rli_iter(&ss, s, s_x, s_y, s_x + w, s_y + h, 1, 1, s_swx, s_swy);

    do
    {
        luaL_argcheck(L, clip || (ss.elem != NULL), 2, ERR_DATA_OVF);
        luaL_argcheck(L, clip || (ds.elem != NULL), 1, ERR_DATA_OVF);

        if(transform(L, &ds, &ss, op, &clr))
            break; /* Custom op can quit early */

        d_act = next_rli_iter(&ds);
        s_act = next_rli_iter(&ss);
    } while(d_act && s_act);

    luaL_unref(L, LUA_REGISTRYINDEX, lf_ref); /* de-reference custom function */
    return 0;
}

static int rli_get(lua_State *L)
{
    /*clr = (src*, x1, y1, clip) */
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));
    bool clip = luaL_optboolean(L, 4, false);

    if(!clip)
        bounds_check_xy(L, a, 2, x, 3, y);
    else if(x < 1 || x > a->width || y < 1 || y > a->height)
    {
        lua_pushnil(L);
        return 1;
    }

    element =  data_element(a->data, a->elems, x, y, a->stride);

    luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(element, x, y)));

    return 1;
}

static int rli_set(lua_State *L)
{
    /* (dst*, [x1, y1, clr, clip]) */
    fb_data *element = NULL;

    struct rocklua_image *a = rli_checktype(L, 1);
    int x = (luaL_checkint(L, 2));
    int y = (luaL_checkint(L, 3));
    fb_data clr = FB_SCALARPACK((unsigned)luaL_optnumber(L, 4, LCD_BLACK));
    bool clip = luaL_optboolean(L, 5, false);

    if(!clip)
        bounds_check_xy(L, a, 2, x, 3, y);
    else if(x < 1 || x > a->width || y < 1 || y > a->height)
    {
        lua_pushnil(L);
        return 1;
    }

    element = data_element(a->data, a->elems, x, y, a->stride);

    luaL_argcheck(L, element != NULL, 1, ERR_DATA_OVF);

    if(!lua_isnoneornil(L, 4))
        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_set(element, x, y, clr)));
    else /* act like get if color is nil*/
        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(element, x, y)));

    /* returns old value */
    return 1;
}

static int rli_height(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->height);
    return 1;
}

static int rli_width(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->width);
    return 1;
}

static int rli_equal(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    struct rocklua_image *b = rli_checktype(L, 2);
    lua_pushboolean(L, a->data == b->data);
    return 1;
}

static int rli_size(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->elems);
    return 1;
}

static int rli_raw(lua_State *L)
{
    /*val = (src*, index, [new_val]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    size_t i = (unsigned)(luaL_checkint(L, 2));

    fb_data val;

    luaL_argcheck(L, i > 0 && i <= (a->elems), 2, ERR_IDX_RANGE);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(a->data[i-1]));

    if(!lua_isnoneornil(L, 3))
    {
        val = FB_SCALARPACK((unsigned) luaL_checkint(L, 3));
        a->data[i-1] = val;
    }

    return 1;
}

static int rli_tostring(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);

    switch(luaL_optint(L, 2, 0))
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

/* Rli Image methods exported to lua */
static const struct luaL_reg rli_lib [] =
{
    {"__tostring", rli_tostring},
    {"_data",      rli_raw},
    {"__len",      rli_size},
    {"__eq",       rli_equal},
    {"width",      rli_width},
    {"height",     rli_height},
    {"set",        rli_set},
    {"get",        rli_get},
    {"copy",       rli_copy},
    {"clear",      rli_clear},
    {"invert",     rli_invert},
    {"marshal",    rli_invert},
    {"points",     rli_iterator_factory},
    {"draw_line",  rli_line},
    {"draw_ellipse_rect", rli_ellipse_rect},
    {NULL, NULL}
};

static inline void rli_init(lua_State *L)
{
#ifdef HAVE_LCD_BITMAP
    /* some devices need x | y coords shifted to match native format
       conversion between packed native formats and individual pixel addressing */
    init_pixmask(&x_shift, &y_shift, &xy_mask, &pixmask);

    luaL_newmetatable(L, ROCKLUA_IMAGE);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  /* pushes the metatable */
    lua_settable(L, -3);  /* metatable.__index = metatable */

    luaL_register(L, NULL, rli_lib);
#else
    /* no need for image lib */
    lua_pushnil(L);
#endif /* HAVE_LCD_BITMAP */
}

/*
 * -----------------------------
 *
 *  Rockbox wrappers start here!
 *
 * -----------------------------
 */

#define RB_WRAP(M) static int rock_##M(lua_State UNUSED_ATTR *L)
#define SIMPLE_VOID_WRAPPER(func) RB_WRAP(func) {(void)L; func(); return 0; }

/* Helper function for opt_viewport */
static void check_tablevalue(lua_State *L, const char* key, int tablepos, void* res, bool is_unsigned)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    if(!lua_isnoneornil(L, -1))
    {
        if(is_unsigned)
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
        lua_setfield(L, tablepos, "vp"); /* table['vp'] = vp(pops value off the stack) */
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
