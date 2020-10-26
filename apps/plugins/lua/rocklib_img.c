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

#define lrockimg_c
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "rocklib.h"
#include "rocklib_img.h"

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

#define ROCKLUA_IMAGE LUA_ROCKLIBNAME ".image"

/* mark for RLI to LUA Interface functions (luaState *L) is the only argument */
#define RLI_LUA static int

#ifndef ABS
#define ABS(a)(((a) < 0) ? - (a) :(a))
#endif

struct rocklua_image
{
    int     width;
    int     height;
    int     stride;
    size_t  elems;
    fb_data *data;
    fb_data dummy[1][1];
};

/* holds iterator data for rlimages */
struct rli_iter_d
{
    int x , y;
    int x1, y1;
    int x2, y2;
    int dx, dy;
    fb_data *elem;
    struct rocklua_image *img;
};

/* __tostring information enums */
enum rli_info {RLI_INFO_ALL = 0, RLI_INFO_TYPE, RLI_INFO_WIDTH,
               RLI_INFO_HEIGHT, RLI_INFO_ELEMS, RLI_INFO_BYTES,
               RLI_INFO_DEPTH, RLI_INFO_FORMAT, RLI_INFO_ADDRESS};

#ifdef HAVE_LCD_COLOR

static inline fb_data invert_color(fb_data rgb)
{
    uint8_t r = 0xFFU - FB_UNPACK_RED(rgb);
    uint8_t g = 0xFFU - FB_UNPACK_GREEN(rgb);
    uint8_t b = 0xFFU - FB_UNPACK_BLUE(rgb);

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
static inline fb_data lua_to_fbscalar(lua_State *L, int narg)
{
    lua_Integer luaint = lua_tointeger(L, narg);
    fb_data val = FB_SCALARPACK((unsigned) luaint);
    return val;
}

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
    int narg;

    if(x > img->width || x < 1)
        narg = nargx;
    else if(y <= img->height && y > 0)
        return; /* note -- return if no error */
    else
        narg = nargy;

    luaL_argerror(L, narg, ERR_IDX_RANGE);
} /* bounds_check_xy */

static struct rocklua_image* rli_checktype(lua_State *L, int arg)
{
#if 0
    return (struct rocklua_image*) luaL_checkudata(L, arg, ROCKLUA_IMAGE);
#else /* cache result */
    static struct rocklua_image* last = NULL;
    void *ud = lua_touserdata(L, arg);

    if(ud != NULL)
    {
        if(ud == last)
            return last;
        else if (lua_getmetatable(L, arg))
        {  /* does it have a metatable? */
            luaL_getmetatable(L, ROCKLUA_IMAGE); /* get correct metatable */
            if (lua_rawequal(L, -1, -2))
            {  /* does it have the correct mt? */
                lua_pop(L, 2);  /* remove both metatables */
                last = (struct rocklua_image*) ud;
                return last;
            }
        }
    }

    luaL_typerror(L, arg, ROCKLUA_IMAGE);  /* else error */
    return NULL;  /* to avoid warnings */
#endif
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
    img->stride = STRIDE_MAIN(w_native, h_native);
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

    a->data = &a->dummy[0][0]; /* ref to beginning of alloc'd img data */

    return a->data;
} /* rli_alloc */

static inline fb_data data_set(fb_data *elem, int x, int y, fb_data *val)
{
    fb_data old_val;
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
    else
        old_val = FB_SCALARPACK(0);

    return old_val;
} /* data_set */

static inline fb_data data_get(fb_data *elem, int x, int y)
{
    return data_set(elem, x, y, NULL);
} /* data_get */

static inline fb_data* rli_get_element(struct rocklua_image* img, int x, int y)
{
    int stride      = img->stride;
    size_t elements = img->elems;
    fb_data *data   = img->data;

    pixel_to_native(x, y, &x, &y);

#if defined(LCD_STRIDEFORMAT) && LCD_STRIDEFORMAT == VERTICAL_STRIDE
    /* column major address */
    size_t data_address = (stride * (x - 1)) + (y - 1);

    /* y needs bound between 0 and stride otherwise overflow to prev/next x */
    if(y <= 0 || y > stride || data_address >= elements)
        return NULL; /* data overflow */
#else
    /* row major address */
    size_t data_address = (stride * (y - 1)) + (x - 1);

    /* x needs bound between 0 and stride otherwise overflow to prev/next y */
    if(x <= 0 || x > stride || data_address >= elements)
        return NULL; /* data overflow */
#endif

    return &data[data_address]; /* return element address */
} /* rli_get_element */

/* Lua to C Interface for pixel set and get functions */
static int rli_setget(lua_State *L, bool is_get, int narg_clip)
{
    /*(set) (dst*, [x1, y1, clr, clip]) */
    /*(get) (dst*, [x1, y1, clip]) */
    struct rocklua_image *a = rli_checktype(L, 1);
    int x = lua_tointeger(L, 2);
    int y = lua_tointeger(L, 3);

    fb_data clr; /* Arg 4 is color if set element */

    fb_data *element = rli_get_element(a, x, y);

    if(!element)
    {
        if(!lua_toboolean(L, narg_clip)) /* Error if !clip */
            bounds_check_xy(L, a, 2, x, 3, y);

        lua_pushnil(L);
        return 1;
    }

    if(is_get) /* get element */
        lua_pushinteger(L, FB_UNPACK_SCALAR_LCD(data_get(element, x, y)));
    else /* set element */
    {
        clr = lua_to_fbscalar(L, 4);
        lua_pushinteger(L, FB_UNPACK_SCALAR_LCD(data_set(element, x, y, &clr)));
    }

    /* returns old value */
    return 1;
} /* rli_setget */

#ifdef RLI_EXTENDED
static bool rli_iter_init(struct rli_iter_d *d,
                          struct rocklua_image *img,
                          int x1, int y1,
                          int x2, int y2,
                          int dx, int dy,
                          bool swx, bool swy)
{

    swap_int((swx), &x1, &x2);
    swap_int((swy), &y1, &y2);

    /* stepping in the correct x direction ? */
    if((dx ^ (x2 - x1)) < 0)
        dx = -dx;

    /* stepping in the correct y direction ? */
    if((dy ^ (y2 - y1)) < 0)
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
} /* rli_iter_init */

static struct rli_iter_d * rli_iter_create(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    int x1    = luaL_optint(L, 2, 1);
    int y1    = luaL_optint(L, 3, 1);
    int x2    = luaL_optint(L, 4, a->width);
    int y2    = luaL_optint(L, 5, a->height);
    int dx    = luaL_optint(L, 6, 1);
    int dy    = luaL_optint(L, 7, 1);
    bool clip = lua_toboolean(L, 8);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    struct rli_iter_d *ds;
    /* create new iter + pushed onto stack */
    ds = (struct rli_iter_d *) lua_newuserdata(L, sizeof(struct rli_iter_d));

    rli_iter_init(ds, a, x1, y1, x2, y2, dx, dy, false, false);

    return ds;
}

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

static void d_line(struct rocklua_image *img,
                   int x1, int y1,
                   int x2, int y2,
                   fb_data *clr)
{
    /* NOTE! clr passed as pointer */
    /* Bresenham midpoint line algorithm */
    fb_data *element;

    int r_a = x2 - x1; /* range of x direction called 'a' for now */
    int r_b = y2 - y1; /* range of y direction called 'b' for now */

    int s_a = (r_a > 0) - (r_a < 0); /* step of a direction -1, 0, 1 */
    int s_b = (r_b > 0) - (r_b < 0); /* step of b direction -1, 0, 1 */

    int d_err;
    int numpixels;

    int *a1 = &x1; /* pointer to the x var 'a' */
    int *b1 = &y1; /* pointer to the y var 'b' */

    r_a = ABS(r_a);/* instead of negative range we switch step */
    r_b = ABS(r_b);/* instead of negative range we switch step */

    if(r_b > r_a) /*if rangeY('b') > rangeX('a') swap their identities */
    {
        a1 = &y1; /* pointer to the y var 'a' */
        b1 = &x1; /* pointer to the x var 'b' */
        swap_int((true), &r_a, &r_b);
        swap_int((true), &s_a, &s_b);
    }

    d_err = ((r_b << 1) - r_a) >> 1; /* preload err of 1 step (px centered) */

    /* add 1 extra point to make the whole line */
    numpixels = r_a + 1;

    r_a -= r_b; /* pre-subtract 'a' - 'b' */

    for(; numpixels > 0; numpixels--)
    {
        element = rli_get_element(img, x1, y1);
        data_set(element, x1, y1, clr);

        if(d_err >= 0) /* 0 is our target midpoint(exact point on the line) */
        {
            *b1 += s_b; /* whichever axis is in 'b' stepped(-1 or +1) */
            d_err -= r_a;
        }
        else
            d_err += r_b; /* only add 'b' when d_err < 0 */

        *a1 += s_a; /* whichever axis is in 'a' stepped(-1 or +1) */
    }

} /* d_line */

/* ellipse worker function */
static void d_ellipse_elements(struct rocklua_image * img,
                               int x1, int y1,
                               int x2, int y2,
                               fb_data *clr,
                               fb_data *fillclr)
{
    fb_data *element;

    if(fillclr)
    {
        d_line(img, x1, y1, x2, y1, fillclr); /* I. II.*/
        d_line(img, x1, y2, x2, y2, fillclr); /* III.IV.*/
    }

    element = rli_get_element(img, x2, y1);
    data_set(element, x2, y1, clr); /*   I. Quadrant +x +y */

    element = rli_get_element(img, x1, y1);
    data_set(element, x1, y1, clr); /*  II. Quadrant -x +y */

    element = rli_get_element(img, x1, y2);
    data_set(element, x1, y2, clr); /* III. Quadrant -x -y */

    element = rli_get_element(img, x2, y2);
    data_set(element, x2, y2, clr); /*  IV. Quadrant +x -y */

} /* d_ellipse_elements */

static void d_ellipse(struct rocklua_image *img,
                          int x1, int y1,
                          int x2, int y2,
                          fb_data *clr,
                          fb_data *fillclr)
{
    /* NOTE! clr and fillclr passed as pointers */
    /* Rasterizing algorithm derivative of work by Alois Zingl */
#if (LCD_WIDTH > 1024 || LCD_HEIGHT > 1024) && defined(INT64_MAX)
    /* Prevents overflow on large screens */
    int64_t dx, dy, err, e1;
#else
    int32_t dx, dy, err, e1;
#endif
    /* if called with swapped points .. exchange them */
    swap_int((x1 > x2), &x1, &x2);
    swap_int((y1 > y2), &y1, &y2);

    int a = x2 - x1; /* diameter */
    int b = y2 - y1; /* diameter */

    if(a == 0 || b == 0)
        return; /* not an error but nothing to display */

    int b1 = (b & 1);
    b = b - (1 - b1);

    int a2 = (a * a);
    int b2 = (b * b);

    dx = ((1 - a) * b2) >> 1; /* error increment */
    dy = (b1 * a2) >> 1;      /* error increment */

    err = dx + dy + b1 * a2; /* error of 1.step */

    y1 += (b + 1) >> 1;
    y2 = y1 - b1;

    do
    {
        d_ellipse_elements(img, x1, y1, x2, y2, clr, fillclr);

        e1 = err;

        if(e1 <= (dy >> 1)) /* target midpoint - y step */
        {
            y1++;
            y2--;
            dy  += a2;
            err += dy;
        }

        if(e1 >= (dx >> 1) || err > (dy >> 1)) /* target midpoint - x step */
        {
            x1++;
            x2--;
            dx  += b2;
            err += dx;
        }

    } while(x1 <= x2);

    if (fillclr && x1 - x2 <= 2)
        fillclr = clr;

    while (y1 - y2 < b) /* early stop of flat ellipse a=1 finish tip */
    {
        d_ellipse_elements(img, x1, y1, x2, y2, clr, fillclr);

        y1++;
        y2--;
    }

} /* d_ellipse */

/* Lua to C Interface for line and ellipse */
static int rli_line_ellipse(lua_State *L, bool is_ellipse, int narg_clip)
{
    struct rocklua_image *a = rli_checktype(L, 1);

    int x1 = luaL_checkint(L, 2);
    int y1 = luaL_checkint(L, 3);
    int x2 = luaL_optint(L, 4, x1);
    int y2 = luaL_optint(L, 5, y1);

    fb_data clr = lua_to_fbscalar(L, 6);

    fb_data fillclr; /* fill color is index 7 if is_ellipse */
    fb_data *p_fillclr = NULL;

    bool clip = lua_toboolean(L, narg_clip);

    if(!clip)
    {
        bounds_check_xy(L, a, 2, x1, 3, y1);
        bounds_check_xy(L, a, 4, x2, 5, y2);
    }

    if(is_ellipse)
    {
        if(lua_type(L, 7) == LUA_TNUMBER)
        {
            fillclr = lua_to_fbscalar(L, 7);
            p_fillclr = &fillclr;
        }

        d_ellipse(a, x1, y1, x2, y2, &clr, p_fillclr);
    }
    else
        d_line(a, x1, y1, x2, y2, &clr);

    return 0;
} /* rli_line_ellipse */

static inline int rli_pushpixel(lua_State *L, fb_data color, int x, int y)
{
    lua_pushinteger(L, FB_UNPACK_SCALAR_LCD(color));
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 3;
}

/* User defined pixel manipulations through rli_copy, rli_marshal */
static void custom_transform(lua_State *L,
                            struct rli_iter_d *ds,
                            struct rli_iter_d *ss,
                            int op,
                            fb_data *color)
{
    (void) color;
    (void) op;
    fb_data dst;
    fb_data src;

    int params;
    bool done = true;

    if (true)/*(lua_isfunction(L, -1))*/
    {
        lua_pushvalue(L, -1); /* make a copy of the lua function */

        dst = data_get(ds->elem, ds->x, ds->y);
        params = rli_pushpixel(L, dst, ds->x, ds->y);

        if(ss) /* Allows src to be omitted */
        {
            src = data_get(ss->elem, ss->x, ss->y);
            params += rli_pushpixel(L, src, ss->x, ss->y);
        }

        lua_call(L, params, 2); /* call custom function w/ n-params & 2 ret */

        if(lua_type(L, -2) == LUA_TNUMBER)
        {
            done = false;
            dst = lua_to_fbscalar(L, -2);
            data_set(ds->elem, ds->x, ds->y, &dst);
        }

        if(ss && (lua_type(L, -1) == LUA_TNUMBER))
        {
            done = false;
            src = lua_to_fbscalar(L, -1);
            data_set(ss->elem, ss->x, ss->y, &src);
        }

        lua_pop(L, 2);
    }

    if(done) /* signal iter to stop */
    {
        ds->dx = 0;
        ds->dy = 0;
    }
} /* custom_transform */

/* Pre defined pixel manipulations through rli_copy */
static void blit_transform(lua_State *L,
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
#if 0
        /* src unneeded */
        case 30:  { dst = clr;          break; }/* copyC  */
        case 31:  { dst = clr | dst;    break; }/* DorC   */
        case 32:  { dst = clr ^ dst;    break; }/* DxorC  */
        case 33:  { dst = ~(clr | dst); break; }/* nDorC  */
        case 34:  { dst = (~clr) | dst; break; }/* DornC  */
        case 35:  { dst = clr & dst;    break; }/* DandC  */
        case 36:  { dst = clr & (~dst); break; }/* nDandC */
        case 37:  { dst = ~clr;         break; }/* notC   */
#endif

    }/*switch op*/
    fb_data val = FB_SCALARPACK(dst);
    data_set(ds->elem, ds->x, ds->y, &val);
} /* blit_transform */

static void invert_transform(lua_State *L,
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
} /* invert_transform */

static void clear_transform(lua_State *L,
                            struct rli_iter_d *ds,
                            struct rli_iter_d *ss,
                            int op,
                            fb_data *color)
{
    (void) L;
    (void) op;
    (void) ss;

    data_set(ds->elem, ds->x, ds->y, color);
} /* clear_transform */

#endif /* RLI_EXTENDED */

/* RLI to LUA Interface functions *********************************************/
RLI_LUA rli_new(lua_State *L)
{   /* [width, height] */
    int width  = luaL_optint(L, 1, LCD_WIDTH);
    int height = luaL_optint(L, 2, LCD_HEIGHT);

    luaL_argcheck(L, width > 0 && height > 0, (width <= 0) ? 1 : 2, ERR_IDX_RANGE);

    rli_alloc(L, width, height);

    return 1;
}

RLI_LUA rli_set(lua_State *L)
{
    /*(set) (dst*, [x1, y1, clr, clip]) */
    return rli_setget(L, false, 5);
}

RLI_LUA rli_get(lua_State *L)
{
    /*(get) (dst*, [x1, y1, clip]) */
    return rli_setget(L, true, 4);
}

RLI_LUA rli_equal(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    struct rocklua_image *b = rli_checktype(L, 2);
    lua_pushboolean(L, a->data == b->data);
    return 1;
}

RLI_LUA rli_height(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushinteger(L, a->height);
    return 1;
}

RLI_LUA rli_width(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushinteger(L, a->width);
    return 1;
}

RLI_LUA rli_size(lua_State *L)
{
    struct rocklua_image *a = rli_checktype(L, 1);
    lua_pushinteger(L, a->elems);
    return 1;
}

RLI_LUA rli_raw(lua_State *L)
{
    /*val = (img*, index, [new_val]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    size_t i = (unsigned) lua_tointeger(L, 2);

    fb_data val;

    luaL_argcheck(L, i > 0 && i <= (a->elems), 2, ERR_IDX_RANGE);

    lua_pushinteger(L, FB_UNPACK_SCALAR_LCD(a->data[i-1]));

    if(lua_type(L, 3) == LUA_TNUMBER)
    {
        val = lua_to_fbscalar(L, 3);
        a->data[i-1] = val;
    }

    return 1;
}

RLI_LUA rli_tostring(lua_State *L)
{
    /* (img, [infoitem]) */
    struct rocklua_image *a = rli_checktype(L, 1);

    int item     = lua_tointeger(L, 2);
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
        case RLI_INFO_TYPE:    { lua_pushfstring(L, ROCKLUA_IMAGE);   break; }
        case RLI_INFO_WIDTH:   { lua_pushinteger(L, a->width);        break; }
        case RLI_INFO_HEIGHT:  { lua_pushinteger(L, a->height);       break; }
        case RLI_INFO_ELEMS:   { lua_pushinteger(L, a->elems);        break; }
        case RLI_INFO_BYTES:   { lua_pushinteger(L, bytes);           break; }
        case RLI_INFO_DEPTH:   { lua_pushinteger(L, LCD_DEPTH );      break; }
        case RLI_INFO_FORMAT:  { lua_pushinteger(L, LCD_PIXELFORMAT); break; }
        case RLI_INFO_ADDRESS: { lua_pushfstring(L, "%p", a->data);   break; }
    }

    /* lua_pushstring(L, lua_tostring(L, -1)); */
    lua_tostring(L, -1); /* converts item at index to string */

    return 1;
}

#ifdef RLI_EXTENDED
RLI_LUA rli_ellipse(lua_State *L)
{
    /* (dst*, x1, y1, x2, y2, [clr, fillclr, clip]) */
    /* line and ellipse share the same init function */
    return rli_line_ellipse(L, true, 8);
}

RLI_LUA rli_line(lua_State *L)
{
    /* (dst*, x1, y1, [x2, y2, clr, clip]) */
    /* line and ellipse share the same init function */
    return rli_line_ellipse(L, false, 7);
}

RLI_LUA rli_iterator(lua_State *L) {
    /* see rli_iterator_factory */
    int params = 0;
    struct rli_iter_d *ds;
    ds = (struct rli_iter_d *) lua_touserdata(L, lua_upvalueindex(1));

    if(ds->dx != 0 || ds->dy != 0)
    {
        params = rli_pushpixel(L, data_get(ds->elem, ds->x, ds->y), ds->x, ds->y);

        next_rli_iter(ds); /* load next element */
    }
    return params; /* nothing left to do */
}

RLI_LUA rli_iterator_factory(lua_State *L)
{
    /* (points) (img*, [x1, y1, x2, y2, dx, dy, clip]) */
    /* (indices 1-8 are used by rli_iter_create) */

    /* create new iter + pushed onto stack */
    rli_iter_create(L);

    /* returns the iter function with embedded iter data(up values) */
    lua_pushcclosure(L, &rli_iterator, 1);

    return 1;
}

RLI_LUA rli_marshal(lua_State *L) /* also invert, clear */
{
    /* (marshal/invert/clear) (img*, [x1, y1, x2, y2, dx, dy, clip, function]) */
    /* (indices 1-8 are used by rli_iter_create) */
    fb_data clr;

    void (*rli_trans)(lua_State *, struct rli_iter_d *, struct rli_iter_d *, int, fb_data *);
    int ltype = lua_type (L, 9);

    /* create new iter + pushed onto stack */
    struct rli_iter_d *ds = rli_iter_create(L);

    if (ltype == LUA_TNUMBER)
    {
        clr = lua_to_fbscalar(L, 9);
        rli_trans = clear_transform;
    }
    else if(ltype == LUA_TFUNCTION) /* custom function */
    {
        rli_trans = custom_transform;
        lua_pushvalue(L, 9); /* ensure lua function on top of stack */
    }
    else
        rli_trans = invert_transform; /* default transformation */

    do
    {
        (*rli_trans)(L, ds, NULL, 0, &clr);
    } while(next_rli_iter(ds));

    return 0;
}

RLI_LUA rli_copy(lua_State *L)
{
    /* (dst*, src*, [d_x, d_y, s_x, s_y, x_off, y_off, clip, [op, funct/clr]]) */
    struct rocklua_image *dst = rli_checktype(L, 1); /* dst */
    struct rocklua_image *src = rli_checktype(L, 2); /* src */

    struct rli_iter_d ds; /* dst */
    struct rli_iter_d ss; /* src */

    /* copy whole image if possible */
    if(src->elems == dst->elems && src->width == dst->width && lua_gettop(L) < 3)
    {
        rb->memcpy(dst->data, src->data, dst->elems * sizeof(fb_data));
        return 0;
    }

    int d_x = luaL_optint(L, 3, 1);
    int d_y = luaL_optint(L, 4, 1);
    int s_x = luaL_optint(L, 5, 1);
    int s_y = luaL_optint(L, 6, 1);

    int w = MIN(dst->width - d_x, src->width - s_x);
    int h = MIN(dst->height - d_y, src->height - s_y);

    int x_off = luaL_optint(L, 7, w);
    int y_off = luaL_optint(L, 8, h);

    bool clip = lua_toboolean(L, 9);
    int op; /* 10 is operation for blit */
    fb_data clr; /* 11 is custom function | color */

    bool d_swx = (x_off < 0); /* dest swap */
    bool d_swy = (y_off < 0);
    bool s_swx = false; /* src swap */
    bool s_swy = false;

    void (*rli_trans)(lua_State *, struct rli_iter_d *, struct rli_iter_d *, int, fb_data *);

    if(!clip) /* Out of bounds is not allowed */
    {
        bounds_check_xy(L, dst, 3, d_x, 4, d_y);
        bounds_check_xy(L, src, 5, s_x, 6, s_y);
        w = MIN(w, ABS(x_off));
        h = MIN(h, ABS(y_off));
        bounds_check_xy(L, dst, 7, d_x + w, 8, d_y + h);
        bounds_check_xy(L, src, 7, s_x + w, 8, s_y + h);
    }
    else
    {
        if (w < 0 || h < 0) /* not an error but nothing to display */
            return 0;
        w = MIN(w, ABS(x_off));
        h = MIN(h, ABS(y_off));
    }

    /* if src->data == dst->data need to care about fill direction */
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

    rli_iter_init(&ds, dst, d_x, d_y, d_x + w, d_y + h, 1, 1, d_swx, d_swy);

    rli_iter_init(&ss, src, s_x, s_y, s_x + w, s_y + h, 1, 1, s_swx, s_swy);

    if (lua_type(L, 11) == LUA_TFUNCTION) /* custom function supplied.. */
    {
        rli_trans = custom_transform;
        lua_settop(L, 11); /* ensure lua function on top of stack */
        clr = FB_SCALARPACK(0);
        op = 0;
    }
    else
    {
        rli_trans = blit_transform; /* default transformation */
        clr = lua_to_fbscalar(L, 11);
        op = lua_tointeger(L, 10);
    }

    do
    {
        (*rli_trans)(L, &ds, &ss, op, &clr);
    } while(next_rli_iter(&ds) && next_rli_iter(&ss));

    return 0;
}

RLI_LUA rli_clear(lua_State *L)
{
    /* (clear) (dst*, [color, x1, y1, x2, y2, clip, dx, dy]) */
    lua_settop(L, 9);

    lua_pushvalue(L, 7); /* clip -- index 8 */
    lua_remove(L, 7);

    lua_pushinteger(L, lua_tointeger(L, 2)); /*color -- index 9*/
    lua_remove(L, 2);

    return rli_marshal(L); /* (img*, [x1, y1, x2, y2, dx, dy, clip, function]) */
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

/*
 * -----------------------------
 *
 *  Rockbox wrappers start here!
 *
 * -----------------------------
 */

#define RB_WRAP(func) static int rock_##func(lua_State UNUSED_ATTR *L)

#if defined NB_SCREENS && (NB_SCREENS > 1)
#define RB_SCREEN_STRUCT(luastate, narg) \
        rb->screens[get_screen(luastate, narg)]
#define RB_SCREENS(luastate, narg, func, ...) \
        rb->screens[get_screen(luastate, narg)]->func(__VA_ARGS__)

static int get_screen(lua_State *L, int narg)
{
    int screen = luaL_optint(L, narg, SCREEN_MAIN);

    if(screen < SCREEN_MAIN)
        screen = SCREEN_MAIN;
    else if(screen > NB_SCREENS)
        screen = NB_SCREENS;

    return screen;
}
#else /* only SCREEN_MAIN exists */
#define get_screen(a,b) (SCREEN_MAIN)
#define RB_SCREEN_STRUCT(luastate, narg) \
        rb->screens[SCREEN_MAIN]
#define RB_SCREENS(luastate, narg, func, ...) \
        rb->screens[SCREEN_MAIN]->func(__VA_ARGS__)
#endif

RB_WRAP(lcd_update)
{
    RB_SCREENS(L, 1, update);
    return 0;
}

RB_WRAP(lcd_clear_display)
{
    RB_SCREENS(L, 1, clear_display);
    return 0;
}

RB_WRAP(lcd_set_drawmode)
{
    int mode = (int) luaL_checkint(L, 1);
    RB_SCREENS(L, 2, set_drawmode, mode);
    return 0;
}

/* helper function for lcd_puts functions */
static const unsigned char * lcd_putshelper(lua_State *L, int *x, int *y)
{
    *x = (int) luaL_checkint(L, 1);
    *y = (int) luaL_checkint(L, 2);
    return luaL_checkstring(L, 3);
}

RB_WRAP(lcd_putsxy)
{
    int x, y;
    const unsigned char *string = lcd_putshelper(L, &x, &y);
    RB_SCREENS(L, 4, putsxy, x, y, string);
    return 0;
}

RB_WRAP(lcd_puts)
{
    int x, y;
    const unsigned char * string = lcd_putshelper(L, &x, &y);
    RB_SCREENS(L, 4, puts, x, y, string);
    return 0;
}

RB_WRAP(lcd_puts_scroll)
{
    int x, y;
    const unsigned char * string = lcd_putshelper(L, &x, &y);
    bool result = RB_SCREENS(L, 4, puts_scroll, x, y, string);
    lua_pushboolean(L, result);
    return 1;
}

RB_WRAP(lcd_scroll_stop)
{
    RB_SCREENS(L, 1, scroll_stop);
    return 0;
}

/* Helper function for opt_viewport */
static int check_tablevalue(lua_State *L, const char* key, int tablepos)
{
    lua_getfield(L, tablepos, key); /* Find table[key] */

    int val = lua_tointeger(L, -1);

    lua_pop(L, 1); /* Pop the value off the stack */
    return val;
}

static inline struct viewport* opt_viewport(lua_State *L,
                                     int narg,
                                     struct viewport* vp,
                                     struct viewport* alt)
{
    if(lua_isnoneornil(L, narg))
        return alt;

    luaL_checktype(L, narg, LUA_TTABLE);

    vp->x = check_tablevalue(L, "x", narg);
    vp->y = check_tablevalue(L, "y", narg);
    vp->width = check_tablevalue(L, "width", narg);
    vp->height = check_tablevalue(L, "height", narg);
    vp->font = check_tablevalue(L, "font", narg);
    vp->drawmode = check_tablevalue(L, "drawmode", narg);
#if LCD_DEPTH > 1
    vp->fg_pattern = (unsigned int) check_tablevalue(L, "fg_pattern", narg);
    vp->bg_pattern = (unsigned int) check_tablevalue(L, "bg_pattern", narg);
#endif

    return vp;
}

RB_WRAP(set_viewport)
{
    static struct viewport vp;
    RB_SCREENS(L, 2, set_viewport, opt_viewport(L, 1, &vp, NULL));
    return 0;
}

RB_WRAP(clear_viewport)
{
    RB_SCREENS(L, 1, clear_viewport);
    return 0;
}

RB_WRAP(font_getstringsize)
{
    const unsigned char* str = luaL_checkstring(L, 1);
    int fontnumber = lua_tointeger(L, 2);
    int w, h, result;

    if (fontnumber == FONT_UI)
        fontnumber = rb->global_status->font_id[SCREEN_MAIN];
    else
        fontnumber = FONT_SYSFIXED;

    if lua_isnil(L, 2)
        result = RB_SCREENS(L, 3, getstringsize, str, &w, &h);
    else
        result = rb->font_getstringsize(str, &w, &h, fontnumber);

    lua_pushinteger(L, result);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);

    return 3;
}

RB_WRAP(lcd_framebuffer)
{
    int screen = get_screen(L, 1);
    static struct viewport vp;
    rb->viewport_set_fullscreen(&vp, screen);
    rli_wrap(L, vp.buffer->data,
                RB_SCREEN_STRUCT(L, 1)->lcdwidth,
                RB_SCREEN_STRUCT(L, 1)->lcdheight);
    return 1;
}

RB_WRAP(lcd_setfont)
{
    int font = (int) luaL_checkint(L, 1);
    RB_SCREENS(L, 2, setfont, font);
    return 0;
}

/* helper function for lcd_xxx_bitmap/rect functions */
static void checkint_arr(lua_State *L, int *val, int narg, int elems)
{
    /* fills passed array of integers with lua integers from stack */
    for (int i = 0; i < elems; i++)
        val[i] = luaL_checkint(L, narg + i);
}

RB_WRAP(gui_scrollbar_draw)
{
    enum {x = 0, y, w, h, items, min_shown, max_shown, flags, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    rb->gui_scrollbar_draw(RB_SCREEN_STRUCT(L, 9), v[x], v[y], v[w], v[h],
                    v[items], v[min_shown], v[max_shown], (unsigned) v[flags]);
return 0;
}

RB_WRAP(lcd_mono_bitmap_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {src_x = 0, src_y, stride, x, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);;

    RB_SCREENS(L, 9, mono_bitmap_part, (const unsigned char *)src->data,
                  v[src_x], v[src_y], v[stride], v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_mono_bitmap)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);

    RB_SCREENS(L, 6, mono_bitmap, (const unsigned char *)src->data,
                                                        v[x], v[y], v[w], v[h]);
    return 0;
}

#if LCD_DEPTH > 1
RB_WRAP(lcd_bitmap_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {src_x = 0, src_y, stride, x, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);

    RB_SCREENS(L, 9, bitmap_part, src->data,
               v[src_x], v[src_y], v[stride], v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_bitmap)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);

    RB_SCREENS(L, 6, bitmap, src->data, v[x], v[y], v[w], v[h]);
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

RB_WRAP(lcd_set_foreground)
{
    unsigned foreground = (unsigned) luaL_checkint(L, 1);
    RB_SCREENS(L, 2, set_foreground, foreground);
    return 0;
}

RB_WRAP(lcd_get_foreground)
{
    unsigned result = RB_SCREENS(L, 1, get_foreground);
    lua_pushinteger(L, result);
    return 1;
}

RB_WRAP(lcd_set_background)
{
    unsigned background = (unsigned) luaL_checkint(L, 1);
    RB_SCREENS(L, 2, set_background, background);
    return 0;
}

RB_WRAP(lcd_get_background)
{
    unsigned result = RB_SCREENS(L, 1, get_background);
    lua_pushinteger(L, result);
    return 1;
}
#endif /* LCD_DEPTH > 1 */

#if (LCD_DEPTH < 4) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
RB_WRAP(lcd_blit_grey_phase)
{
    /* note that by and bheight are in 8-pixel units! */
    unsigned char * values = (unsigned char *) luaL_checkstring(L, 1);
    unsigned char * phases = (unsigned char *) luaL_checkstring(L, 2);
    enum {bx = 0, by, bw, bh, stride, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 3, eCNT);

    rb->lcd_blit_grey_phase(values, phases, v[bx], v[by], v[bw], v[bh], v[stride]);
    return 0;
}

RB_WRAP(lcd_blit_mono)
{
    /* note that by and bheight are in 8-pixel units! */
    const unsigned char * data = (const unsigned char *) luaL_checkstring(L, 1);
    enum {x = 0, by, w, bh, stride, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 3, eCNT);

    rb->lcd_blit_mono(data, v[x], v[by], v[w], v[bh], v[stride]);
    return 0;
}
#endif /* LCD_DEPTH < 4 && CONFIG_PLATFORM & PLATFORM_NATIVE */

#if LCD_DEPTH == 16
RB_WRAP(lcd_bitmap_transparent_part)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {src_x = 0, src_y, stride, x, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);

    RB_SCREENS(L, 9, transparent_bitmap_part, src->data,
              v[src_x], v[src_y], v[stride], v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_bitmap_transparent)
{
    struct rocklua_image *src = rli_checktype(L, 1);
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 2, eCNT);

    RB_SCREENS(L, 6, transparent_bitmap, src->data, v[x], v[y], v[w], v[h]);
    return 0;
}
#endif /* LCD_DEPTH == 16 */

RB_WRAP(lcd_update_rect)
{
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 5, update_rect, v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_drawrect)
{
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 5, drawrect, v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_fillrect)
{
    enum {x = 0, y, w, h, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 5, fillrect, v[x], v[y], v[w], v[h]);
    return 0;
}

RB_WRAP(lcd_drawline)
{
    enum {x1 = 0, y1, x2, y2, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 5, drawline, v[x1], v[y1], v[x2], v[y2]);
    return 0;
}

RB_WRAP(lcd_hline)
{
    enum {x1 = 0, x2, y, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 4, hline, v[x1], v[x2], v[y]);
    return 0;
}

RB_WRAP(lcd_vline)
{
    enum {x = 0, y1, y2, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    RB_SCREENS(L, 4, vline, v[x], v[y1], v[y2]);
    return 0;
}

RB_WRAP(lcd_drawpixel)
{
    int x = (int) luaL_checkint(L, 1);
    int y = (int) luaL_checkint(L, 2);
    RB_SCREENS(L, 3, drawpixel, x, y);
    return 0;
}

#ifdef HAVE_LCD_COLOR
RB_WRAP(lcd_rgbpack)
{
    enum {r = 0, g, b, eCNT};
    int v[eCNT];
    checkint_arr(L, v, 1, eCNT);

    int result = LCD_RGBPACK(v[r], v[g], v[b]);
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

#define R(NAME) {#NAME, rock_##NAME}
static const luaL_Reg rocklib_img[] =
{
    /* Graphics */
    R(lcd_update),
    R(lcd_clear_display),
    R(lcd_set_drawmode),
    R(lcd_putsxy),
    R(lcd_puts),
    R(lcd_puts_scroll),
    R(lcd_scroll_stop),
    R(set_viewport),
    R(clear_viewport),
    R(font_getstringsize),
    R(lcd_framebuffer),
    R(lcd_setfont),
    R(gui_scrollbar_draw),
    R(lcd_mono_bitmap_part),
    R(lcd_mono_bitmap),
#if LCD_DEPTH > 1
    R(lcd_get_backdrop),
    R(lcd_bitmap_part),
    R(lcd_bitmap),
    R(lcd_set_foreground),
    R(lcd_get_foreground),
    R(lcd_set_background),
    R(lcd_get_background),
#endif /* LCD_DEPTH > 1 */
#if (LCD_DEPTH < 4) && (CONFIG_PLATFORM & PLATFORM_NATIVE)
    R(lcd_blit_grey_phase),
    R(lcd_blit_mono),
#endif /* LCD_DEPTH < 4 && CONFIG_PLATFORM & PLATFORM_NATIVE */
#if LCD_DEPTH == 16
    R(lcd_bitmap_transparent_part),
    R(lcd_bitmap_transparent),
#endif
    R(lcd_update_rect),
    R(lcd_drawrect),
    R(lcd_fillrect),
    R(lcd_drawline),
    R(lcd_hline),
    R(lcd_vline),
    R(lcd_drawpixel),

#ifdef HAVE_LCD_COLOR
    R(lcd_rgbpack),
    R(lcd_rgbunpack),
#endif
    R(read_bmp_file),
    {"new_image", rli_new},

    {NULL, NULL}
};
#undef  R

LUALIB_API int luaopen_rock_img(lua_State *L)
{
    /* some devices need x | y coords shifted to match native format */
    /* conversion between packed native formats and individual pixel addressing */
    init_pixelmask(&x_shift, &y_shift, &xy_mask, &pixelmask);

    luaL_newmetatable(L, ROCKLUA_IMAGE);

    lua_pushvalue(L, -1);  /* pushes the metatable */
    lua_setfield(L, -2, "__index"); /* metatable.__index = metatable */
    luaL_register(L, NULL, rli_lib); /*add rli_lib to the image metatable*/

    luaL_register(L, LUA_ROCKLIBNAME, rocklib_img);

    return 1;
}

