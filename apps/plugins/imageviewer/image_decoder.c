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
    "jpeg", // Default decoder for jpeg: Use jpeg for old decoder, jpegp for new
    "png",
#ifdef HAVE_LCD_COLOR
    "ppm",
#endif
    "gif",
    "jpegp",
};

/* Check file type by magic number or file extension
 *
 * If the file contains magic number, use it to determine image type.
 * Otherwise use file extension to determine image type.
 * If the file contains magic number and file extension is not correct,
 * informs user that something is wrong.
 */
enum image_type get_image_type(const char *name, bool quiet)
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
        { ".gif",   IMAGE_GIF  },
    };
    static const struct {
        char *magic;    /* magic number */
        int length;     /* length of the magic number */
        enum image_type type;
    } magic_list[] = {
        { "BM", 2, IMAGE_BMP },
        { "\xff\xd8\xff\xe0", 4, IMAGE_JPEG },
        { "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", 8, IMAGE_PNG },
#ifdef HAVE_LCD_COLOR
        { "P3", 2, IMAGE_PPM },
        { "P6", 2, IMAGE_PPM },
#endif
        { "GIF87a", 6, IMAGE_GIF },
        { "GIF89a", 6, IMAGE_GIF },
    };

    enum image_type type = IMAGE_UNKNOWN;
    const char *ext = rb->strrchr(name, '.');
    int i, fd;
    char buf[12];

    /* check file extention */
    if (ext)
    {
        for (i = 0; i < (int)ARRAYLEN(ext_list); i++)
        {
            if (!rb->strcasecmp(ext, ext_list[i].ext))
            {
                type = ext_list[i].type;
                break;
            }
        }
    }

    /* check magic value in the file */
    fd = rb->open(name, O_RDONLY);
    if (fd >= 0)
    {
        rb->memset(buf, 0, sizeof buf);
        rb->read(fd, buf, sizeof buf);
        rb->close(fd);
        for (i = 0; i < (int)ARRAYLEN(magic_list); i++)
        {
            if (!rb->memcmp(buf, magic_list[i].magic, magic_list[i].length))
            {
                if (!quiet && type != magic_list[i].type)
                {
                    /* file extension is wrong. */
                    rb->splashf(HZ*1, "Note: File extension is not correct");
                }
                type = magic_list[i].type;
                break;
            }
        }
    }
    return type;
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

    if (lc_hdr->api_version != IMGDEC_API_VERSION ||
        hdr->img_api_size > sizeof(struct imgdec_api) ||
        hdr->plugin_api_version != PLUGIN_API_VERSION ||
        hdr->plugin_api_size > sizeof(struct plugin_api))
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
