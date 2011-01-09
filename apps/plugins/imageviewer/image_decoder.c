/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * load image decoder.
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

#include "plugin.h"
#include "imageviewer.h"
#include "image_decoder.h"

static const char *decoder_names[MAX_IMAGE_TYPES] = {
    "bmp",
    "jpeg",
    "png",
#ifdef HAVE_LCD_COLOR
    "ppm"
#endif
};

/* check file type by extention */
enum image_type get_image_type(const char *name)
{
    static const struct {
        char *ext;
        enum image_type type;
    } ext_list[] = {
        { ".bmp",   IMAGE_BMP  },
        { ".jpg",   IMAGE_JPEG },
        { ".jpe",   IMAGE_JPEG },
        { ".jpeg",  IMAGE_JPEG },
        { ".png",   IMAGE_PNG  },
#ifdef HAVE_LCD_COLOR
        { ".ppm",   IMAGE_PPM  },
#endif
    };

    const char *ext = rb->strrchr(name, '.');
    int i;
    if (!ext)
        return IMAGE_UNKNOWN;

    for (i = 0; i < (int)ARRAYLEN(ext_list); i++)
    {
        if (!rb->strcasecmp(ext, ext_list[i].ext))
            return ext_list[i].type;
    }
    return IMAGE_UNKNOWN;
}

static void *decoder_handle = NULL;
const struct image_decoder *load_decoder(struct loader_info *loader_info)
{
    const char *name;
    char filename[MAX_PATH];
    struct imgdec_header *hdr;
    struct lc_header     *lc_hdr;

    if (loader_info->type < 0 || loader_info->type >= MAX_IMAGE_TYPES)
    {
        rb->splashf(2*HZ, "Unknown type: %d", loader_info->type);
        goto error;
    }

    release_decoder();

    name = decoder_names[loader_info->type];
    rb->snprintf(filename, MAX_PATH, VIEWERS_DIR "/%s.ovl", name);

    /* load decoder to the buffer. */
    decoder_handle = rb->lc_open(filename, loader_info->buffer, loader_info->size);
    if (!decoder_handle)
    {
        rb->splashf(2*HZ, "Can't open %s", filename);
        goto error;
    }

    hdr = rb->lc_get_header(decoder_handle);
    if (!hdr)
    {
        rb->splash(2*HZ, "Can't get header");
        goto error_close;
    }
    lc_hdr = &hdr->lc_hdr;

    if (lc_hdr->magic != PLUGIN_MAGIC || lc_hdr->target_id != TARGET_ID)
    {
        rb->splashf(2*HZ, "%s decoder: Incompatible model.", name);
        goto error_close;
    }

    if (lc_hdr->api_version != IMGDEC_API_VERSION)
    {
        rb->splashf(2*HZ, "%s decoder: Incompatible version.", name);
        goto error_close;
    }

    *(hdr->api) = rb;
    *(hdr->img_api) = loader_info->iv;

    /* set remaining buffer size to loader_info. decoder will
     * be loaded to the end of the buffer, so fix size only. */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    loader_info->size = lc_hdr->load_addr - loader_info->buffer;
#endif

    return hdr->decoder;

error_close:
    release_decoder();
error:
    return NULL;
}

void release_decoder(void)
{
    if (decoder_handle != NULL)
    {
        rb->lc_close(decoder_handle);
        decoder_handle = NULL;
    }
}
