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
 * -----------------------------------------------------------------------------
 *
 *  Rockbox Lua image wrapper
 *
 *  Some devices(1-bit / 2-bit displays) have packed bit formats that
 *  need to be unpacked in order to work on them at a pixel level.
 *
 *  The internal formats of these devices do not follow the same paradigm
 *  for image sizes either; We still display the actual width and height to
 *  the user but store stride based on the native values
 *
 *  Conversion between native addressing and per pixel addressing
 *  incurs extra overhead but it is much faster to do it
 *  on the 'C' side rather than in lua.
 *
 * -----------------------------------------------------------------------------
 */

#ifdef HAVE_LCD_BITMAP
#define RLI_EXTENDED
#endif

#define ROCKLUA_IMAGE "rb.image"
#define ERR_IDX_RANGE "index out of range"
#define ERR_DATA_OVF  "data overflow"

/* mark for RLI to LUA Interface functions (luaState *L) is the only argument */
#define RLI_LUA static int

#ifndef ABS
#define ABS(a)(((a) < 0) ? - (a) :(a))
#endif

#ifndef LCD_BLACK
#define LCD_BLACK 0x0
#endif

struct rocklua_image
{
    int     width;
    int     height;
    int     stride;
    size_t  elems;
    fb_data *data;
    fb_data dummy[1];
};

/* holds iterator data for rlimages */
struct rli_iter_d
{
    struct rocklua_image *img;
    fb_data *elem;
    int x , y;
    int x1, y1;
    int x2, y2;
    int dx, dy;
};

/* __tostring information enums */
enum rli_info {RLI_INFO_ALL = 0, RLI_INFO_TYPE, RLI_INFO_WIDTH,
               RLI_INFO_HEIGHT, RLI_INFO_ELEMS, RLI_INFO_BYTES,
               RLI_INFO_DEPTH, RLI_INFO_FORMAT, RLI_INFO_ADDRESS};

#ifdef HAVE_LCD_COLOR

static inline fb_data invert_color(fb_data rgb)
{
    uint8_t r = 0xFFU - FB_UNPACK_RED(rgb);
    uint8_t g = 0xFFU - FB_UNPACK_RED(rgb);
    uint8_t b = 0xFFU - FB_UNPACK_RED(rgb);

    return FB_RGBPACK(r, g, b);
}
#else /* !HAVE_LCD_COLOR */

#define invert_color(c) (~c)

#endif /* HAVE_LCD_COLOR */


#if (LCD_DEPTH > 2) /* no native to pixel mapping needed */

#define pixel_to_fb(x, y, o, n) {(void) x; (void) y; do { } while (0);}
#define pixel_to_native(x, y, xn, yn) {*xn = x; *yn = y;}
#define init_pixelmask(x, y, m, p) do { } while (0)


#else /* some devices need x | y coords shifted to match native format */

static fb_data x_shift = FB_SCALARPACK(0);
static fb_data y_shift = FB_SCALARPACK(0);
static fb_data xy_mask = FB_SCALARPACK(0);
static const fb_data *pixelmask = NULL;

/* conversion between packed native formats and individual pixel addressing */
static inline void init_pixelmask(fb_data *x_shift, fb_data *y_shift,
                                  fb_data *xy_mask, const fb_data **pixelmask)
{

#if(LCD_PIXELFORMAT == VERTICAL_PACKING) && LCD_DEPTH == 1
    static const fb_data pixelmask_v1[8] =
               {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    *pixelmask = pixelmask_v1;

    (void) x_shift;
    *y_shift = 3U;
    *xy_mask = ((1 << (*y_shift)) - 1);
#elif(LCD_PIXELFORMAT == VERTICAL_PACKING) && LCD_DEPTH == 2
    static const fb_data pixelmask_v2[4] = {0x03, 0x0C, 0x30, 0xC0};
    *pixelmask = pixelmask_v2;

    (void) x_shift;
    *y_shift = 2U;
    *xy_mask = ((1 << (*y_shift)) - 1);
#elif(LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    static const fb_data pixelmask_vi2[8] =
               {0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};
    *pixelmask = pixelmask_vi2;

    (void) x_shift;
    *y_shift = 3U;
    *xy_mask = ((1 << (*y_shift)) - 1);
#elif(LCD_PIXELFORMAT == HORIZONTAL_PACKING) && LCD_DEPTH == 2
    /* MSB on left */
    static const fb_data pixelmask_h2[4] = {0x03, 0x0C, 0x30, 0xC0};
    *pixelmask = pixelmask_h2;

    (void) y_shift;
    *x_shift = 2U;
    *xy_mask = ((1 << (*x_shift)) - 1);
#else
    #warning Unknown Pixel Format
#endif /* LCD_PIXELFORMAT */

} /* init_pixelmask */

static inline void pixel_to_native(int x, int y, int *x_native, int *y_native)
{
    *x_native = ((x - 1) >> x_shift) + 1;
    *y_native = ((y - 1) >> y_shift) + 1;
} /* pixel_to_native */

static inline fb_data set_masked_pixel(fb_data old,
                                       fb_data new,
                                       fb_data mask,
                                       int bit_n)
{
    /*equivalent of: (old & (~mask)) | ((new << bit_n) & mask);*/
    return old ^ ((old ^ (new << bit_n)) & mask);

} /* set_masked_pixel */

static inline fb_data get_masked_pixel(fb_data val, fb_data mask, int bit_n)
{
    val = val & mask;
    return val >> bit_n;
} /* get_masked_pixel */

/* conversion between packed native formats and individual pixel addressing */
static void pixel_to_fb(int x, int y, fb_data *oldv, fb_data *newv)
{
    fb_data mask;
    int bit_n;

#if(LCD_PIXELFORMAT == VERTICAL_INTERLEAVED) && LCD_DEPTH == 2
    (void) x;
    const uint16_t greymap_vi2[4] = {0x0000, 0x0001, 0x0100, 0x0101};

    bit_n = (y - 1) & xy_mask;
    mask  = pixelmask[bit_n];

    *newv = greymap_vi2[*newv &(0x3)]; /* [0-3] => greymap */
    *newv = set_masked_pixel(*oldv, *newv, mask, bit_n);

    *oldv = get_masked_pixel(*oldv, mask, bit_n);

    if((*oldv) > 1) /* greymap => [0-3] */
        *oldv = ((*oldv) & 0x1U) + 2U; /* 2, 3 */
    else
        *oldv &= 1U; /* 0, 1 */

#elif(LCD_DEPTH <= 2)
    if(y_shift)
        bit_n = (y - 1) & xy_mask;
    else if(x_shift)
        bit_n = xy_mask - ((x - 1) & xy_mask); /*MSB on left*/

    if(y_shift || x_shift)
    {
        mask   = pixelmask[bit_n];
        bit_n *= LCD_DEPTH;

        *newv = set_masked_pixel(*oldv, *newv, mask, bit_n);

        *oldv = get_masked_pixel(*oldv, mask, bit_n);
    }
#else
    #error Unknown Pixel Format
#endif /* LCD_PIXELFORMAT == VERTICAL_INTERLEAVED && LCD_DEPTH == 2 */
} /* pixel_to_fb */

#endif /* (LCD_DEPTH > 2) no native to pixel mapping needed */

/* Internal worker functions for image data array *****************************/

static inline void swap_int(bool swap, int *v1, int *v2)
{
    if(swap)
    {
        int val = *v1;
        *v1 = *v2;
        *v2 = val;
    }
} /* swap_int */

/* Throws error if x or y are out of bounds notifies user which narg indice
   the out of bound variable originated */
static void bounds_check_xy(lua_State *L, struct rocklua_image *img,
                         int nargx, int x, int nargy, int y)
{
    luaL_argcheck(L, x <= img->width && x > 0,  nargx, ERR_IDX_RANGE);
    luaL_argcheck(L, y <= img->height && y > 0, nargy, ERR_IDX_RANGE);
} /* bounds_check_xy */

static struct rocklua_image* rli_checktype(lua_State *L, int arg)
{
    void *ud = luaL_checkudata(L, arg, ROCKLUA_IMAGE);

    luaL_argcheck(L, ud != NULL, arg, "'" ROCKLUA_IMAGE "' expected");

    return (struct rocklua_image*) ud;
} /* rli_checktype */

static struct rocklua_image * alloc_rlimage(lua_State *L, bool alloc_data,
                                            int width, int height)
{
    /* rliimage is pushed on the stack it is up to you to pop it */
    struct rocklua_image *img;

    const size_t sz_header = sizeof(struct rocklua_image);
    size_t sz_data = 0;
    size_t n_elems;

    int w_native;
    int h_native;

    pixel_to_native(width, height, &w_native, &h_native);

    n_elems = (size_t)(w_native * h_native);

    if(alloc_data) /* if this a new image we need space for image data */
        sz_data = n_elems * sizeof(fb_data);

    /* newuserdata pushes the userdata onto the stack */
    img = (struct rocklua_image *) lua_newuserdata(L, sz_header + sz_data);

    luaL_getmetatable(L, ROCKLUA_IMAGE);
    lua_setmetatable(L, -2);

    /* apparent w/h is stored but behind the scenes native w/h is used */
    img->width  = width;
    img->height = height;
    img->stride = w_native;
    img->elems  = n_elems;

    return img;
} /* alloc_rlimage */

static inline void rli_wrap(lua_State *L, fb_data *src, int width, int height)
{
    /* rliimage is pushed on the stack it is up to you to pop it */
    struct rocklua_image *a = alloc_rlimage(L, false, width, height);

    a->data = src;
} /* rli_wrap */

static inline fb_data* rli_alloc(lua_State *L, int width, int height)
{
    /* rliimage is pushed on the stack it is up to you to pop it */
    struct rocklua_image *a = alloc_rlimage(L, true, width, height);

    a->data = &a->dummy[0]; /* ref to beginning of alloc'd img data */

    return a->data;
} /* rli_alloc */

static inline fb_data data_setget(fb_data *elem, int x, int y, fb_data *val)
{
    fb_data old_val = FB_SCALARPACK(0);
    fb_data new_val;

    if(elem)
    {
        old_val = *elem;

        if(val)
        {
            new_val = *val;
            pixel_to_fb(x, y, &old_val, &new_val);
            *elem = new_val;
        }
        else
            pixel_to_fb(x, y, &old_val, &new_val);
    }

    return old_val;
} /* data_setget */

static inline fb_data data_set(fb_data *elem, int x, int y, fb_data *new_val)
{
    /* get and set share the same underlying function */
    return data_setget(elem, x, y, new_val);
} /* data_set */

static inline fb_data data_get(fb_data *elem, int x, int y)
{
    /* get and set share the same underlying function */
    return data_setget(elem, x, y, NULL);
} /* data_get */

static fb_data* rli_get_element(struct rocklua_image* img, int x, int y)
{
    int stride      = img->stride;
    size_t elements = img->elems;
    fb_data *data   = img->data;

    pixel_to_native(x, y, &x, &y);

    /* row major address */
    size_t data_address = (stride * (y - 1)) + (x - 1);

    /* x needs bound between 0 and stride otherwise overflow to prev/next y */
    if(x <= 0 || x > stride || data_address >= elements)
        return NULL; /* data overflow */

    return &data[data_address]; /* return element address */
} /* rli_get_element */

/* Lua to C Interface for pixel set and get functions */
static int rli_setget(lua_State *L, bool is_get)
{
    /*(set) (dst*, [x1, y1, clr, clip]) */
    /*(get) (dst*, [x1, y1, clip]) */
    struct rocklua_image *a = rli_checktype(L, 1);
    int x = luaL_checkint(L, 2);
    int y = luaL_checkint(L, 3);

    fb_data clr = FB_SCALARPACK(0); /* Arg 4 is color if set element */
    fb_data *p_clr = &clr;

    int clip_narg;

    if(is_get) /* get element */
    {
        p_clr = NULL;
        clip_narg = 4;
    }
    else /* set element */
    {
        clr = FB_SCALARPACK((unsigned) luaL_checknumber(L, 4));
        clip_narg = 5;
    }

    fb_data *element = rli_get_element(a, x, y);

    if(!element)
    {
        if(!luaL_optboolean(L, clip_narg, false)) /* Error if !clip */
            bounds_check_xy(L, a, 2, x, 3, y);

        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_setget(element, x, y, p_clr)));

    /* returns old value */
    return 1;
} /* rli_setget */

#ifdef RLI_EXTENDED
static bool init_rli_iter(struct rli_iter_d *d,
                          struct rocklua_image *img,
                          int x1, int y1,
                          int x2, int y2,
                          int dx, int dy,
                          bool swx, bool swy)
{

    swap_int((swx), &x1, &x2);
    swap_int((swy), &y1, &y2);

    /* stepping in the correct x direction ? */
    if((dx > 0 && x1 > x2) || (dx < 0 && x1 < x2))
        dx = -dx;

    /* stepping in the correct y direction ? */
    if((dy > 0 && y1 > y2) || (dy < 0 && y1 < y2))
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
    d->elem = rli_get_element(img, d->x, d->y);

    return(dx != 0 || dy != 0);
} /* init_rli_iter */

/* steps to the next point(x, y) by delta x/y, stores pointer to element
   returns true if x & y haven't reached x2 & y2
   if limit reached - element set to NULL, deltas set to 0 & false returned
*/
static bool next_rli_iter(struct rli_iter_d *d)
{
    if((d->dx > 0 && d->x < d->x2) || (d->dx < 0 && d->x > d->x2))
        d->x += d->dx;
    else if((d->dy > 0 && d->y < d->y2) || (d->dy < 0 && d->y > d->y2))
    {
        d->x = d->x1; /* Reset x*/
        d->y += d->dy;
    }
    else
    {
        d->elem = NULL;
        d->dx = 0;
        d->dy = 0;
        return false;
    }

    d->elem = rli_get_element(d->img, d->x, d->y);

    return true;
} /* next_rli_iter */

static int d_line(struct rocklua_image *img,
                  int x1, int y1,
                  int x2, int y2,
                  fb_data *clr,
                  bool clip)
{
    /* NOTE! clr passed as pointer */
    /* Bresenham midpoint line algorithm */
    fb_data *element;

    int r_a = x2 - x1; /* range of x direction called 'a' for now */
    int r_b = y2 - y1; /* range of y direction called 'b' for now */

    int s_a = 1; /* step of a direction */
    int s_b = 1; /* step of b direction */

    int d_err;
    int numpixels;

    int *a1 = &x1; /* pointer to the x var 'a' */
    int *b1 = &y1; /* pointer to the y var 'b' */

    if(r_a < 0) /* instead of negative range we will switch step instead */
    {
        r_a = -r_a;
        s_a = -s_a;
    }

    if(r_b < 0) /* instead of negative range we will switch step instead */
    {
        r_b = -r_b;
        s_b = -s_b;
    }

    x2 += s_a; /* add 1 extra point to make the whole line */
    y2 += s_b; /* add 1 extra point */

    if(r_b > r_a) /*if rangeY('b') > rangeX('a') swap their identities */
    {
        a1 = &y1;
        b1 = &x1;
        swap_int((true), &r_a, &r_b);
        swap_int((true), &s_a, &s_b);
    }

    d_err = ((r_b << 1) - r_a) >> 1; /* preload err of 1 step (px centered) */

    numpixels = r_a + 1;

    r_a -= r_b; /* pre-subtract 'a' - 'b' */

    for(;numpixels > 0; numpixels--)
    {
        element = rli_get_element(img, x1, y1);
        if(element || clip)
            data_set(element, x1, y1, clr);
        else
            return numpixels + 1; /* Error */

        if(d_err >= 0) /* 0 is our target midpoint(exact point on the line) */
        {
            *b1 += s_b; /* whichever axis is in 'b' stepped(-1 or +1) */
            d_err -= r_a;
        }
        else
            d_err += r_b; /* only add 'b' when d_err < 0 */

        *a1 += s_a; /* whichever axis is in 'a' stepped(-1 or +1) */
    }

    return 0;
} /* d_line */

/* ellipse worker function */
static int d_ellipse_elements(struct rocklua_image * img,
                              int x1, int y1,
                              int x2, int y2,
                              int sx, int sy,
                              fb_data *clr,
                              fb_data *fillclr,
                              bool clip)
{
    int ret = 0;
    fb_data *element1, *element2, *element3, *element4;

    if(fillclr && x1 - sx != x2 + sx)
    {
        ret |= d_line(img, x1, y1, x2, y1, fillclr, clip); /* I. II.*/
        ret |= d_line(img, x1, y2, x2, y2, fillclr, clip); /* III.IV.*/
    }

    x1 -= sx; /* shift x & y */
    y1 -= sy;
    x2 += sx;
    y2 += sy;

    element1 = rli_get_element(img, x2, y1);
    element2 = rli_get_element(img, x1, y1);
    element3 = rli_get_element(img, x1, y2);
    element4 = rli_get_element(img, x2, y2);

    if(clip || (element1 && element2 && element3 && element4))
    {
        data_set(element1, x2, y1, clr); /*   I. Quadrant +x +y */
        data_set(element2, x1, y1, clr); /*  II. Quadrant -x +y */
        data_set(element3, x1, y2, clr); /* III. Quadrant -x -y */
        data_set(element4, x2, y2, clr); /*  IV. Quadrant +x -y */
    }
    else
        ret = 2; /* ERROR */

    return ret;
} /* d_ellipse_elements */

static int d_ellipse(struct rocklua_image *img,
                          int x1, int y1,
                          int x2, int y2,
                          fb_data *clr,
                          fb_data *fillclr,
                          bool clip)
{
    /* NOTE! clr and fillclr passed as pointers */
    /* Rasterizing algorithm derivative of work by Alois Zing */
#if LCD_WIDTH > 1024 || LCD_HEIGHT > 1024
    /* Prevents overflow on large screens */
    double dx, dy, err, e2;
#else
    long dx, dy, err, e2;
#endif

    int ret = 0;

    int a = ABS(x2 - x1); /* diameter */
    int b = ABS(y2 - y1); /* diameter */

    if(a == 0 || b == 0 || !clr)
        return ret; /* not an error but nothing to display */

    int b1 = (b & 1);
    b = b - (1 - b1);

    int a2 = (a * a);
    int b2 = (b * b);

    dx = ((1 - a) * b2) >> 1; /* error increment */
    dy = (b1 * a2) >> 1;      /* error increment */

    err = dx + dy + b1 * a2; /* error of 1.step */

    /* if called with swapped points .. exchange them */
    swap_int((x1 > x2), &x1, &x2);
    swap_int((y1 > y2), &y1, &y2);

    y1 += (b + 1) >> 1;
    y2 = y1 - b1;

    do
    {
        ret = d_ellipse_elements(img, x1, y1, x2, y2, 0, 0, clr, fillclr, clip);

        e2 = err;

        /* using division because you can't use bit shift on doubles.. */
        if(e2 <= (dy / 2)) /* target midpoint - y step */
        {
            y1++;
            y2--;
            dy  += a2;
            err += dy;
        }

        if(e2 >= (dx / 2) || err > (dy / 2)) /* target midpoint - x step */
        {
            x1++;
            x2--;
            dx  += b2;
            err += dx;
        }

    } while(ret == 0 && x1 <= x2);

    while (ret == 0 && y1 - y2 < b) /* early stop of flat ellipse a=1 finish tip */
    {
        ret = d_ellipse_elements(img, x1, y1, x2, y2, 1, 0, clr, fillclr, clip);

        y1++;
        y2--;
    }

    return ret;
} /* d_ellipse */

/* Lua to C Interface for line and ellipse */
static int rli_line_ellipse(lua_State *L, bool is_ellipse)
{
    struct rocklua_image *a = rli_checktype(L, 1);

    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_optint(L, 4, x1);
    int y2 = luaL_optint(L, 5, y1);

    fb_data clr = FB_SCALARPACK((unsigned) luaL_checknumber(L, 6));

    fb_data fillclr; /* fill color is index 7 if is_ellipse */
    fb_data *p_fillclr = NULL;

    bool clip;
    int clip_narg;

    if(is_ellipse)
        clip_narg = 8;
    else
        clip_narg = 7;

    clip = luaL_optboolean(L, clip_narg, false);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    if(is_ellipse)
    {
        if(!lua_isnoneornil(L, 7))
        {
            fillclr = FB_SCALARPACK((unsigned) luaL_checkint(L, 7));
            p_fillclr = &fillclr;
        }

        luaL_argcheck(L, d_ellipse(a, x1, y1, x2, y2, &clr, p_fillclr, clip) == 0,
                      1, ERR_DATA_OVF);
    }
    else
        luaL_argcheck(L, d_line(a, x1, y1, x2, y2, &clr, clip) == 0,
                      1, ERR_DATA_OVF);

    return 0;
} /* rli_line_ellipse */

/* Pushes lua function from Stack at position narg to top of stack
   and puts a reference in the global registry for later use       */
static inline int register_luafunc(lua_State *L, int narg_funct)
{
        lua_pushvalue(L, narg_funct); /* lua function */
        int lf_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_settop(L, 0); /* clear C stack for use by lua function */
        return lf_ref;
} /* register_luafunc */

/* User defined pixel manipulations through rli_copy, rli_marshal */
static int custom_transform(lua_State *L,
                            struct rli_iter_d *ds,
                            struct rli_iter_d *ss,
                            int op,
                            fb_data *color)
{
    (void) color;

    fb_data dst;
    fb_data src;

    unsigned int params = 3;
    int ret = 0;

    lua_rawgeti(L, LUA_REGISTRYINDEX, op);

    dst = data_get(ds->elem, ds->x, ds->y);
    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(dst));
    lua_pushnumber(L, ds->x);
    lua_pushnumber(L, ds->y);

    if(ss) /* Allows src to be omitted */
    {
        params  += 3;
        src = data_get(ss->elem, ss->x, ss->y);
        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(src));
        lua_pushnumber(L, ss->x);
        lua_pushnumber(L, ss->y);
    }

    lua_call(L, params, 2); /* call custom function w/ n-params & 2 ret */

    if(!lua_isnoneornil(L, -2))
    {
        ret = 1;
        dst = FB_SCALARPACK((unsigned) luaL_checknumber(L, -2));
        data_set(ds->elem, ds->x, ds->y, &dst);
    }

    if(!lua_isnoneornil(L, -1) && ss)
    {
        ret |= 2;
        src = FB_SCALARPACK((unsigned) luaL_checknumber(L, -1));
        data_set(ss->elem, ss->x, ss->y, &src);
    }

    lua_pop(L, 2);
    return ret; /* 0 signals iterator to stop */
} /* custom_transform */

/* Pre defined pixel manipulations through rli_copy */
static int blit_transform(lua_State *L,
                          struct rli_iter_d *ds,
                          struct rli_iter_d *ss,
                          int op,
                          fb_data *color)
{
    (void) L;
    unsigned clr = FB_UNPACK_SCALAR_LCD(*color);
    unsigned dst = FB_UNPACK_SCALAR_LCD(data_get(ds->elem, ds->x, ds->y));
    unsigned src;

    /* Reuse 0 - 7 for src / clr blits*/
    if(op >= 30 && op <= 37)
    {
        op -= 30;
        src = clr;
    }
    else
        src = FB_UNPACK_SCALAR_LCD(data_get(ss->elem, ss->x, ss->y));

    switch(op)
    {
        default:
        /* case 30: */
        case 0:  { dst = src;          break; }/* copyS/C  */
        /* case 31: */
        case 1:  { dst = src | dst;    break; }/* DorS/C   */
        /* case 32: */
        case 2:  { dst = src ^ dst;    break; }/* DxorS/C  */
        /* case 33: */
        case 3:  { dst = ~(src | dst); break; }/* nDorS/C  */
        /* case 34: */
        case 4:  { dst = (~src) | dst; break; }/* DornS/C  */
        /* case 35: */
        case 5:  { dst = src & dst;    break; }/* DandS/C  */
        /* case 36: */
        case 6:  { dst = src & (~dst); break; }/* nDandS/C */
        /* case 37: */
        case 7:  { dst = ~src;         break; }/* notS/C   */

        /* mask blits */
        case 8:  { if(src != 0) { dst = clr; } break; }/* Sand */
        case 9:  { if(src == 0) { dst = clr; } break; }/* Snot */

        case 10:  { dst  = src | clr;           break; }/* SorC  */
        case 11:  { dst  = src ^ clr;           break; }/* SxorC */
        case 12:  { dst  = ~(src | clr);        break; }/* nSorC */
        case 13:  { dst  = src | (~clr);        break; }/* SornC */
        case 14:  { dst  = src & clr;           break; }/* SandC */
        case 15:  { dst  = (~src) & clr;        break; }/* nSandC */
        case 16:  { dst |= (~src) | clr;        break; }/* DornSorC */
        case 17:  { dst ^= (src & (dst ^ clr)); break; }/* DxorSandDxorC */

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

    }/*switch op*/
    fb_data data = FB_SCALARPACK(dst);
    data_set(ds->elem, ds->x, ds->y, &data);
    return 1;
} /* blit_transform */

static int invert_transform(lua_State *L,
                            struct rli_iter_d *ds,
                            struct rli_iter_d *ss,
                            int op,
                            fb_data *color)
{
    (void) L;
    (void) color;
    (void) op;
    (void) ss;

    fb_data val = invert_color(data_get(ds->elem, ds->x, ds->y));
    data_set(ds->elem, ds->x, ds->y, &val);

    return 1;
} /* invert_transform */
#endif /* RLI_EXTENDED */

/* RLI to LUA Interface functions *********************************************/
RLI_LUA rli_new(lua_State *L)
{   /* [width, height] */
    int width  = luaL_optint(L, 1, LCD_WIDTH);
    int height = luaL_optint(L, 2, LCD_HEIGHT);

    luaL_argcheck(L, width  > 0, 1, ERR_IDX_RANGE);
    luaL_argcheck(L, height > 0, 2, ERR_IDX_RANGE);

    rli_alloc(L, width, height);

    return 1;
}

RLI_LUA rli_set(lua_State *L)
{
    /*(set) (dst*, [x1, y1, clr, clip]) */
    /* get and set share the same underlying function */
    return rli_setget(L, false);
}

RLI_LUA rli_get(lua_State *L)
{
    /*(get) (dst*, [x1, y1, clip]) */
    /* get and set share the same underlying function */
    return rli_setget(L, true);
}

RLI_LUA rli_height(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->height);
    return 1;
}

RLI_LUA rli_width(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->width);
    return 1;
}

RLI_LUA rli_equal(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    struct rocklua_image *b = rli_checktype(L, 2);
    lua_pushboolean(L, a->data == b->data);
    return 1;
}

RLI_LUA rli_size(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushnumber(L, a->elems);
    return 1;
}

RLI_LUA rli_raw(lua_State *L)
{
    /*val = (img*, index, [new_val]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    size_t i = (unsigned) luaL_checkint(L, 2);

    fb_data val;

    luaL_argcheck(L, i > 0 && i <= (a->elems), 2, ERR_IDX_RANGE);

    lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(a->data[i-1]));

    if(!lua_isnoneornil(L, 3))
    {
        val = FB_SCALARPACK((unsigned) luaL_checknumber(L, 3));
        a->data[i-1] = val;
    }

    return 1;
}

RLI_LUA rli_tostring(lua_State *L)
{
    /* (img, [infoitem]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    int item     = (unsigned) luaL_optint(L, 2, 0);
    size_t bytes = a->elems * sizeof(fb_data);

    switch(item)
    {
        default:
        case RLI_INFO_ALL:
        {
            lua_pushfstring(L,
            ROCKLUA_IMAGE ": %dx%d, %d elements, %d bytes, %d-bit depth, %d pack",
            a->width, a->height, a->elems, bytes, LCD_DEPTH, LCD_PIXELFORMAT);
            break;
        }
        case RLI_INFO_TYPE:    { lua_pushfstring(L, ROCKLUA_IMAGE   ); break; }
        case RLI_INFO_WIDTH:   { lua_pushfstring(L, "%d", a->width  ); break; }
        case RLI_INFO_HEIGHT:  { lua_pushfstring(L, "%d", a->height ); break; }
        case RLI_INFO_ELEMS:   { lua_pushfstring(L, "%d", a->elems  ); break; }
        case RLI_INFO_BYTES:   { lua_pushfstring(L, "%d", bytes     ); break; }
        case RLI_INFO_DEPTH:   { lua_pushfstring(L, "%d", LCD_DEPTH ); break; }
        case RLI_INFO_FORMAT:  { lua_pushfstring(L, "%d", LCD_PIXELFORMAT); break; }
        case RLI_INFO_ADDRESS: { lua_pushfstring(L, "%p", a->data); break; }
    }

    return 1;
}

#ifdef RLI_EXTENDED
RLI_LUA rli_ellipse(lua_State *L)
{
    /* (dst*, x1, y1, x2, y2, [clr, fillclr, clip]) */
    /* line and ellipse share the same init function */
    return rli_line_ellipse(L, true);
}

RLI_LUA rli_line(lua_State *L)
{
    /* (dst*, x1, y1, [x2, y2, clr, clip]) */
    /* line and ellipse share the same init function */
    return rli_line_ellipse(L, false);
}

RLI_LUA rli_iterator(lua_State *L) {
    /* see rli_iterator_factory */
    struct rli_iter_d *ds;
    ds = (struct rli_iter_d *) lua_touserdata(L, lua_upvalueindex(1));

    if(ds->dx != 0 || ds->dy != 0)
    {
        lua_pushnumber(L, FB_UNPACK_SCALAR_LCD(data_get(ds->elem, ds->x, ds->y)));

        lua_pushinteger(L, ds->x);
        lua_pushinteger(L, ds->y);

        next_rli_iter(ds); /* load next element */

        return 3;
    }
    return 0; /* nothing left to do */
}

RLI_LUA rli_iterator_factory(lua_State *L) {
    /* (src*, [x1, y1, x2, y2, dx, dy]) */
    struct rocklua_image *a = rli_checktype(L, 1); /*image we wish to iterate*/

    struct rli_iter_d *ds;

    int x1  = luaL_optint(L, 2, 1);
    int y1  = luaL_optint(L, 3, 1);
    int x2  = luaL_optint(L, 4, a->width  - x1 + 1);
    int y2  = luaL_optint(L, 5, a->height - y1 + 1);
    int dx  = luaL_optint(L, 6, 1);
    int dy  = luaL_optint(L, 7, 1);

    /* create new iter + pushed onto stack */
    ds = (struct rli_iter_d *) lua_newuserdata(L, sizeof(struct rli_iter_d));

    init_rli_iter(ds, a, x1, y1, x2, y2, dx, dy, false, false);

    /* returns the iter function with embedded iter data(up values) */
    lua_pushcclosure(L, &rli_iterator, 1);

    return 1;
}

RLI_LUA rli_marshal(lua_State *L) /* also invert */
{
    /* (img*, [x1, y1, x2, y2, dx, dy, clip, function]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    struct rli_iter_d ds;

    int x1 = luaL_optint(L, 2, 1);
    int y1 = luaL_optint(L, 3, 1);
    int x2 = luaL_optint(L, 4, a->width);
    int y2 = luaL_optint(L, 5, a->height);
    int dx = luaL_optint(L, 6, 1);
    int dy = luaL_optint(L, 7, 1);
    bool clip    = luaL_optboolean(L, 8, false);

    int lf_ref = LUA_NOREF; /* de-ref'd without consequence */

    int (*rli_trans)(lua_State *, struct rli_iter_d *, struct rli_iter_d *, int, fb_data *);
    rli_trans = invert_transform; /* default transformation */

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    init_rli_iter(&ds, a, x1, y1, x2, y2, dx, dy, false, false);

    if(lua_isfunction(L, 9)) /* custom function */
    {
        rli_trans = custom_transform;
        lf_ref = register_luafunc(L, 9);
    }

    do
    {
        luaL_argcheck(L, clip || (ds.elem != NULL), 1, ERR_DATA_OVF);

        if(!(*rli_trans)(L, &ds, NULL, lf_ref, NULL))
            break; /* Custom op can quit early */

    } while(next_rli_iter(&ds));

    luaL_unref(L, LUA_REGISTRYINDEX, lf_ref); /* de-reference custom function */
    return 0;
}

RLI_LUA rli_copy(lua_State *L)
{
    /* (dst*, src*, [d_x, d_y, s_x, s_y, x_off, y_off, clip, [op, funct/clr]]) */
    struct rocklua_image *d = rli_checktype(L, 1); /*dst*/
    struct rocklua_image *s = rli_checktype(L, 2); /*src*/

    struct rli_iter_d ds; /*dst*/
    struct rli_iter_d ss; /*src*/

    /* copy whole image if possible */
    if(s->elems == d->elems && s->width == d->width && lua_gettop(L) < 3)
    {
        memcpy(d->data, s->data, d->elems * sizeof(fb_data));
        return 0;
    }

    int d_x = luaL_optint(L, 3, 1);
    int d_y = luaL_optint(L, 4, 1);
    int s_x = luaL_optint(L, 5, 1);
    int s_y = luaL_optint(L, 6, 1);

    int w = MIN(d->width - d_x, s->width - s_x);
    int h = MIN(d->height - d_y, s->height - s_y);

    int x_off = luaL_optint(L, 7, w);
    int y_off = luaL_optint(L, 8, h);

    bool clip = luaL_optboolean(L, 9, false);
    int op = luaL_optint(L, 10, 0);
    fb_data clr = FB_SCALARPACK(0); /* 11 is custom function | color */

    bool d_swx = (x_off < 0); /* dest swap */
    bool d_swy = (y_off < 0);
    bool s_swx = false; /* src  swap */
    bool s_swy = false;

    int (*rli_trans)(lua_State *, struct rli_iter_d *, struct rli_iter_d *, int, fb_data *);
    rli_trans = blit_transform; /* default transformation */

    int lf_ref = LUA_NOREF; /* de-ref'd without consequence */

    if(!clip) /* Out of bounds is not allowed */
    {
        bounds_check_xy(L, d, 3, d_x, 4, d_y);
        bounds_check_xy(L, s, 5, s_x, 6, s_y);
    }
    else if (w < 0 || h < 0) /* not an error but nothing to display */
        return 0;

    w = MIN(w, ABS(x_off));
    h = MIN(h, ABS(y_off));

    /* if(s->data == d->data) need to care about fill direction */
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

    init_rli_iter(&ds, d, d_x, d_y, d_x + w, d_y + h, 1, 1, d_swx, d_swy);

    init_rli_iter(&ss, s, s_x, s_y, s_x + w, s_y + h, 1, 1, s_swx, s_swy);

    if (op == 0xFF && lua_isfunction(L, 11)) /* custom function specified.. */
    {
        rli_trans = custom_transform;
        lf_ref = register_luafunc(L, 11);
        op = lf_ref;
    }
    else
        clr = FB_SCALARPACK((unsigned) luaL_optnumber(L, 11, LCD_BLACK));

    do
    {
        if(!clip)
        {
            luaL_argcheck(L, ss.elem != NULL, 2, ERR_DATA_OVF);
            luaL_argcheck(L, ds.elem != NULL, 1, ERR_DATA_OVF);
        }

        if(!(*rli_trans)(L, &ds, &ss, op, &clr))
            break; /* Custom op can quit early */

    } while(next_rli_iter(&ds) && next_rli_iter(&ss));

    luaL_unref(L, LUA_REGISTRYINDEX, lf_ref); /* de-reference custom function */
    return 0;
}

RLI_LUA rli_clear(lua_State *L)
{
    /* (dst*, [color, x1, y1, x2, y2, clip]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    struct rli_iter_d ds;

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

    init_rli_iter(&ds, a, x1, y1, x2, y2, 1, 1, false, false);

    do
    {
        luaL_argcheck(L, clip || (ds.elem != NULL), 1, ERR_DATA_OVF);

        data_set(ds.elem, ds.x, ds.y, &clr);

    } while(next_rli_iter(&ds));

    return 0;
}
#endif /* RLI_EXTENDED */

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

#ifdef RLI_EXTENDED
    {"copy",       rli_copy},
    {"clear",      rli_clear},
    {"invert",     rli_marshal},
    {"marshal",    rli_marshal},
    {"points",     rli_iterator_factory},
    {"line",       rli_line},
    {"ellipse",    rli_ellipse},
#endif /* RLI_EXTENDED */
    {NULL, NULL}
};

static inline void rli_init(lua_State *L)
{
    /* some devices need x | y coords shifted to match native format */
    /* conversion between packed native formats and individual pixel addressing */
    init_pixelmask(&x_shift, &y_shift, &xy_mask, &pixelmask);

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
static void check_tablevalue(lua_State *L,
                             const char* key,
                             int tablepos,
                             void* res,
                             bool is_unsigned)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    int val = luaL_optint(L, -1, 0);

    if(is_unsigned)
        *(unsigned*)res = (unsigned) val;
    else
        *(int*)res = val;

    lua_pop(L, 1); /* Pop the value off the stack */
}

static inline struct viewport* opt_viewport(lua_State *L,
                                     int narg,
                                     struct viewport* vp,
                                     struct viewport* alt)
{
    if(lua_isnoneornil(L, narg))
        return alt;

    luaL_checktype(L, narg, LUA_TTABLE);

    check_tablevalue(L, "x", narg, &vp->x, false);
    check_tablevalue(L, "y", narg, &vp->y, false);
    check_tablevalue(L, "width", narg, &vp->width, false);
    check_tablevalue(L, "height", narg, &vp->height, false);
#ifdef HAVE_LCD_BITMAP
    check_tablevalue(L, "font", narg, &vp->font, false);
    check_tablevalue(L, "drawmode", narg, &vp->drawmode, false);
#endif
#if LCD_DEPTH > 1
    check_tablevalue(L, "fg_pattern", narg, &vp->fg_pattern, true);
    check_tablevalue(L, "bg_pattern", narg, &vp->bg_pattern, true);
#endif

    return vp;
}

RB_WRAP(set_viewport)
{
    static struct viewport vp;
    int screen = luaL_optint(L, 2, SCREEN_MAIN);
    rb->screens[screen]->set_viewport(opt_viewport(L, 1, &vp, NULL));
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

