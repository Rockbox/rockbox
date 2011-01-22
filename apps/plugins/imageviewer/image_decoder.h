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

#ifndef _IMAGE_DECODER_H
#define _IMAGE_DECODER_H

#include "imageviewer.h"

enum image_type {
    IMAGE_UNKNOWN = -1,
    IMAGE_BMP = 0,
    IMAGE_JPEG,
    IMAGE_PNG,
#ifdef HAVE_LCD_COLOR
    IMAGE_PPM,
#endif
    MAX_IMAGE_TYPES
};

struct loader_info {
    enum image_type type;
    const struct imgdec_api *iv;
    unsigned char* buffer;
    size_t size;
};

/* Check file type by magic number or file extension */
enum image_type get_image_type(const char *name, bool quiet);
/* Load image decoder */
const struct image_decoder *load_decoder(struct loader_info *loader_info);
/* Release the loaded decoder */
void release_decoder(void);

#endif /* _IMAGE_DECODER_H */
