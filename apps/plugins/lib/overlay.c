/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Overlay loader
 *
 * Copyright (C) 2006 Jens Arnold
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
#include "overlay.h"

/* load and run a plugin linked as an overlay. 

 arguments:
   parameter = plugin parameter, passed on to the overlay
   filename  = overlay file name, absolute path as usual
   name      = overlay display name
   
 result:
   return value from the overlay
   
   As long as a large plugin to be overlayed doesn't use the audiobuffer
   itself, no adjustments in the plugin source code are necessary to make
   it work as an overlay, it only needs an adapted linker script.
   
   If the overlayed plugin *does* use the audiobuffer itself, it needs 
   to make sure not to overwrite itself.
   
   The linker script for the overlay should use a base address towards the
   end of the audiobuffer, just low enough to make the overlay fit. */

enum plugin_status run_overlay(const void* parameter,
                               unsigned char *filename, unsigned char *name)
{
    size_t audiobuf_size;
    unsigned char *audiobuf;
    void *handle;
    struct plugin_header *p_hdr;
    struct lc_header     *hdr;
    enum plugin_status retval = PLUGIN_ERROR;

    audiobuf = rb->plugin_get_audio_buffer(&audiobuf_size);
    if (!audiobuf)
    {
        rb->splash(2*HZ, "Can't obtain memory");
        goto error;
    }

    handle = rb->lc_open(filename, audiobuf, audiobuf_size);
    if (!handle)
    {
        rb->splashf(2*HZ, "Can't open %s", filename);
        goto error;
    }

    p_hdr = rb->lc_get_header(handle);
    if (!p_hdr)
    {
        rb->splash(2*HZ, "Can't get header");
        goto error_close;
    }
    else
        hdr = &p_hdr->lc_hdr;

    if (hdr->magic != PLUGIN_MAGIC || hdr->target_id != TARGET_ID)
    {
        rb->splashf(2*HZ, "%s overlay: Incompatible model.", name);
        goto error_close;
    }

    
    if (hdr->api_version > PLUGIN_API_VERSION
        || hdr->api_version < PLUGIN_MIN_API_VERSION)
    {
        rb->splashf(2*HZ, "%s overlay: Incompatible version.", name);
        goto error_close;
    }

    *(p_hdr->api) = rb;
    retval = p_hdr->entry_point(parameter);
    /* fall through */
error_close:
    rb->lc_close(handle);
error:
    return retval;
}
