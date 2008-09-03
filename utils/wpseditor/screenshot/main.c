/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include "gd.h"
#include "gdfonts.h"
#include "api.h"

#define DEBUGF1 _debug
#define DEBUGF2 if(verbose) _debug

#define getFont() gdFontGetSmall()

static struct trackstate mp3data =
{
(char*)"Test title",
(char*)"Test artist",
(char*)"Test album",
(char*)"Test genre",
(char*)"Test disc",
(char*)"Test track",
(char*)"Test year",
(char*)"Test composer",
(char*)"Test comment",
(char*)"Test album artist",
(char*)"Test grouping",
1, /* int discnum */
1, /* int tracknum */
1, /* int version */
1, /* int layer */
2008, /* int year */

100, /* int length */
70 /* int elapsed */
};

static struct wpsstate wpsdata = {-20, -1, -1, 70, API_STATUS_FASTFORWARD};
              /* volume, fontheight, fontwidth, battery_level, audio_status */

static struct proxy_api api;
static bool verbose = false;
static int (*wps_init)(const char* buff, struct proxy_api *api, bool isfile);
static int (*wps_display)();
static int (*wps_refresh)();
static gdImagePtr framebuffer;
static gdImagePtr backdrop;

extern gdImagePtr gdImageCreateFromBmp(FILE * inFile);
extern char *get_current_dir_name (void) __THROW;

int _debug(const char* fmt,...)
{
#if 0 /* Doesn't want to compile ?? */
    struct va_list ap;
    
    va_start(ap, fmt);
    
    fprintf(stdout, "[DBG]  ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    
    va_end(ap);

    return 0;
#else
    return -1;
#endif
}

void _putsxy(int x, int y, const unsigned char *str)
{
    struct viewport_api avp;
    int black = gdImageColorAllocate(framebuffer, 0, 0, 0);
    
    api.get_current_vp(&avp);
    
    gdImageString(framebuffer, getFont(), x + avp.x, y + avp.y - avp.fontheight, (unsigned char*)str, black);
}

void _transparent_bitmap_part(const void *src, int src_x, int src_y,
                                        int stride, int x, int y, int width, int height)
{
    FILE *_image;
    gdImagePtr image;
    int pink;
    
    DEBUGF2("transparent_bitmap_part(const void *src=%s, int src_x=%d, int src_y=%d, int stride=%d, int x=%d, int y=%d, int width=%d, int height=%d", (char*)src, src_x, src_y, stride, x, y, width, height);
    
    _image = fopen(src, "rb");
    if(_image == NULL)
        return;
    
    image = gdImageCreateFromBmp(_image);
    fclose(_image);
    
    pink = gdTrueColor(255, 0, 255); 
    gdImageColorTransparent(image, pink);
    
    gdImageCopy(framebuffer, image, x, y, src_x, src_y, width, height);
    
    gdImageDestroy(image);
}

void _bitmap_part(const void *src, int src_x, int src_y,
                  int stride, int x, int y, int width, int height)
{
    FILE *_image;
    gdImagePtr image;
    
    DEBUGF2("bitmap_part(const void *src=%s, int src_x=%d, int src_y=%d, int stride=%d, int x=%d, int y=%d, int width=%d, int height=%d", (char*)src, src_x, src_y, stride, x, y, width, height);
    
    _image = fopen(src, "rb");
    if(_image == NULL)
        return;
    
    image = gdImageCreateFromBmp(_image);
    fclose(_image);
    
    gdImageCopy(framebuffer, image, x, y, src_x, src_y, width, height);
    
    gdImageDestroy(image);
}

void _drawpixel(int x, int y)
{
    int black = gdImageColorAllocate(framebuffer, 0, 0, 0);
    gdImageSetPixel(framebuffer, x, y, black);
}

void _fillrect(int x, int y, int width, int height)
{
    /* Don't draw this as backdrop is used */
#if 0
    int black = gdImageColorAllocate(framebuffer, 0, 0, 0);
    gdImageFilledRectangle(framebuffer, x, y, x+width, y+height, black);
#endif
}

void _hline(int x1, int x2, int y)
{
    int black = gdImageColorAllocate(framebuffer, 0, 0, 0); 
    gdImageLine(framebuffer, x1, y, x2, y, black);
}

void _vline(int x, int y1, int y2)
{
    int black = gdImageColorAllocate(framebuffer, 0, 0, 0); 
    gdImageLine(framebuffer, x, y1, x, y2, black);
}

void _clear_viewport(int x, int y, int w, int h, int color)
{
    if(backdrop == NULL)
        return;
    
    gdImageCopy(framebuffer, backdrop, x, y, x, y, w, h);
}

static bool _load_wps_backdrop(char* filename)
{
    FILE *image;
    if(backdrop != NULL)
        gdImageDestroy(backdrop);
    
    DEBUGF2("load backdrop: %s", filename);
    
    image = fopen(filename, "rb");
    if(image == NULL)
        return false;
    
    backdrop = gdImageCreateFromBmp(image);
    fclose(image);
    
    return true;
}

int _read_bmp_file(const char* filename, int *width, int *height)
{
    FILE *_image;
    gdImagePtr image;
    
    DEBUGF2("load backdrop: %s", filename);
    
    _image = fopen(filename, "rb");
    if(_image == NULL)
        return 0;
    
    image = gdImageCreateFromBmp(_image);
    fclose(_image);
    
    *width = image->sx;
    *height = image->sy;
    
    gdImageDestroy(image);
    
    return 1;
}

static void _drawBackdrop()
{
    if(backdrop == NULL)
        return;
    
    gdImageCopy(framebuffer, backdrop, 0, 0, 0, 0, backdrop->sx, backdrop->sy);
}

static int screenshot(char *model, char *wps, char *png)
{
    char lib[255];
    void *handle;
    FILE *out, *in;
    
    in = fopen(wps, "rb");
    if(in == NULL)
    {
        fprintf(stderr, "[ERR]  Cannot open WPS: %s\n", wps);
        return -1;
    }
    fclose(in);
    
    out = fopen(png, "wb");
    if(out == NULL)
    {
        fprintf(stderr, "[ERR]  Cannot open PNG: %s\n", png);
        return -2;
    }
    
    snprintf(lib, 255, "%s/libwps_%s.so", (char*)get_current_dir_name(), (char*)model);
    handle = dlopen(lib, RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "[ERR]  Cannot open library: %s\n", dlerror());
        fclose(out);
        return -3;
    }
    
    wps_init = dlsym(handle, "wps_init");
    wps_display = dlsym(handle, "wps_display");
    wps_refresh = dlsym(handle, "wps_refresh");
    
    if (!wps_init || !wps_display || !wps_refresh)
    {
        fprintf(stderr, "[ERR]  Failed to resolve funcs!");
        dlclose(handle);
        fclose(out);
        return -4;
    }

    memset(&api, 0, sizeof(struct proxy_api));
    
    if(verbose)
        api.verbose = 3;
    else
        api.verbose = 0;

    api.putsxy =                     &_putsxy;
    api.transparent_bitmap_part =    &_transparent_bitmap_part;
    api.bitmap_part =                &_bitmap_part;
    api.drawpixel =                  &_drawpixel;
    api.fillrect =                   &_fillrect;
    api.hline =                      &_hline;
    api.vline =                      &_vline;
    api.clear_viewport =             &_clear_viewport;
    api.load_wps_backdrop =          &_load_wps_backdrop;
    api.read_bmp_file =              &_read_bmp_file;
    api.debugf =                     &_debug;
    
    wps_init(wps, &api, true);
    
    framebuffer = gdImageCreateTrueColor(api.getwidth(), api.getheight());

    _drawBackdrop();
    
    if(strcmp(api.get_model_name(), model) != 0)
    {
        fprintf(stderr, "[ERR]  Model name doesn't match the one supplied by the library\n");
        fprintf(stderr, "       %s <-> %s\n", model, api.get_model_name());
        dlclose(handle);
        fclose(out);
        gdImageDestroy(framebuffer);
        gdImageDestroy(backdrop);
        wps_init = NULL;
        wps_display = NULL;
        wps_refresh = NULL;
        return -5;
    }
    fprintf(stdout, "[INFO] Model: %s\n", api.get_model_name());
    
    wpsdata.fontheight = getFont()->h;
    wpsdata.fontwidth = getFont()->w;
    api.set_wpsstate(wpsdata);
    api.set_trackstate(mp3data);
    api.set_next_trackstate(mp3data);
    
    _drawBackdrop();
    wps_refresh();
    gdImagePng(framebuffer, out);
    
    fprintf(stdout, "[INFO] Image written\n");

    dlclose(handle);
    fclose(out);
    gdImageDestroy(framebuffer);
    gdImageDestroy(backdrop);
    
    wps_init = NULL;
    wps_display = NULL;
    wps_refresh = NULL;
    
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "Usage: screenshot [-V] <MODEL> <WPS_FILE> <OUT_PNG>\n");
    fprintf(stderr, "Example: screenshot h10_5gb iCatcher.wps out.png\n");
}

int main(int argc, char ** argv)
{  
    if(argc < 4)
    {
        usage();
        return -1;
    }
    
    if(strcmp(argv[1], "-V") == 0)
    {
        verbose = true;
        return screenshot(argv[2], argv[3], argv[4]);
    }
    else
    {
        verbose = false;
        return screenshot(argv[1], argv[2], argv[3]);
    }
    
    return 0;
}
