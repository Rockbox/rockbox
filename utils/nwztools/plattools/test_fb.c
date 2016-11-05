/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include "nwz_lib.h"
#include "nwz_plattools.h"
#include <linux/fb.h>
#include <stdint.h>
#include <sys/mman.h>

static struct fb_fix_screeninfo finfo;
static struct fb_var_screeninfo vinfo;
static uint8_t *framebuffer;

static inline uint32_t read32(uint8_t *p)
{
    return *p | *(p + 1) << 8 | *(p + 2) << 16 | *(p + 3) << 24;
}

static inline void write32(uint8_t *p, uint32_t val)
{
    *p++ = val & 0xff; val >>= 8;
    *p++ = val & 0xff; val >>= 8;
    *p++ = val & 0xff; val >>= 8;
    *p++ = val;
}

/* assume lsb and little-endian */
static inline void put_pix_mask(uint8_t *location, int offset, int len, uint8_t pix)
{
    /* adjust pixel */
    pix >>= 8 - len;
    uint32_t mask = ((1 << len) - 1) << offset;
    uint32_t val = read32(location);
    val = ((val) & ~mask) | pix << offset;
    write32(location, val);
}

static inline void put_pix(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    x += vinfo.xoffset;
    y += vinfo.yoffset;
    uint8_t *location = framebuffer + x * (vinfo.bits_per_pixel / 8) + y * finfo.line_length;
    put_pix_mask(location, vinfo.red.offset, vinfo.red.length, r);
    put_pix_mask(location, vinfo.green.offset, vinfo.green.length, g);
    put_pix_mask(location, vinfo.blue.offset, vinfo.blue.length, b);
}

static void dump_fb(FILE *out, const char *path)
{
    fprintf(out, "%s:\n", path);
    int fd = open(path, O_RDWR);
    if(fd < 0)
    {
        fprintf(out, "  cannot open");
        return;
    }
    /* get fixed info */
    if(ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        fprintf(out, "  ioctl failed (fix info)");
        close(fd);
        return;
    }
    fprintf(out, "  identification: %s\n", finfo.id);
    fprintf(out, "  type: %d\n", finfo.type);
    fprintf(out, "  visual: %d\n", finfo.visual);
    fprintf(out, "  accel: %d\n", finfo.accel);
    fprintf(out, "  line length: %d\n", finfo.line_length);
    fprintf(out, "  mem length: %d\n", finfo.smem_len);
    /* get variable info */
    if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        close(fd);
        fprintf(out, "  ioctl failed (var info)");
        return;
    }
    fprintf(out, "  xres: %d\n", vinfo.xres);
    fprintf(out, "  yres: %d\n", vinfo.yres);
    fprintf(out, "  xoff: %d\n", vinfo.xoffset);
    fprintf(out, "  yoff: %d\n", vinfo.yoffset);
    fprintf(out, "  bbp: %d\n", vinfo.bits_per_pixel);
    fprintf(out, "  red: %d-%d\n", vinfo.red.offset + vinfo.red.length - 1, vinfo.red.offset);
    fprintf(out, "  green: %d-%d\n", vinfo.green.offset + vinfo.green.length - 1, vinfo.green.offset);
    fprintf(out, "  blue: %d-%d\n", vinfo.blue.offset + vinfo.blue.length - 1, vinfo.blue.offset);
    /* get mode info */
    struct nwz_fb_image_info mode_info;
    nwz_fb_set_standard_mode(fd);
    if(ioctl(fd, NWZ_FB_GET_MODE, &mode_info) < 0)
    {
        close(fd);
        fprintf(out, "  ioctl failed (get mode)\n");
        return;
    }
    fprintf(out, "  tc_enable: %d\n", mode_info.tc_enable);
    fprintf(out, "  t_color: %d\n", mode_info.t_color);
    fprintf(out, "  alpha: %d\n", mode_info.alpha);
    fprintf(out, "  rot: %d\n", mode_info.rot);
    fprintf(out, "  page: %d\n", mode_info.page);
    fprintf(out, "  update: %d\n", mode_info.update);
    /* mmap device (but avoid TV) */
    if(vinfo.xres != 720)
    {
        long screensize = vinfo.yres_virtual * finfo.line_length;
        framebuffer = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);
        if(framebuffer == 0)
        {
            close(fd);
            fprintf(out, "  mmap failed");
            return;
        }
        for(int y = 0; y < 10; y++)
            for(int x = 0; x < 10; x++)
            {
                put_pix(x, y, 0xff, 0, 0);
                put_pix(x + 10, y, 0, 0xff, 0);
                put_pix(x + 20, y, 0, 0, 0xff);
            }
    }
    sleep(3);
    close(fd);
}

int NWZ_TOOL_MAIN(test_fb)(int argc, char **argv)
{
    FILE *f = fopen("/contents/fb.txt", "w");
    if(!f)
        f = stdout;
    dump_fb(f, "/dev/fb/0");
    if(f)
        fclose(f);
    return 0;
}
