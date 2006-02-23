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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef SIMULATOR
#include "plugin.h"

/* load and run a plugin linked as an overlay. 

 arguments:
   rb        = pointer to plugin api, also passed on to the overlay
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

enum plugin_status run_overlay(struct plugin_api* rb, void* parameter,
                               unsigned char *filename, unsigned char *name)
{
    int fd, readsize;
    int audiobuf_size;
    unsigned char *audiobuf;
    static struct plugin_header header;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(2*HZ, true, "Can't open %s", filename);
        return PLUGIN_ERROR;
    }
    readsize = rb->read(fd, &header, sizeof(header));
    rb->close(fd);
    /* Close for now. Less code than doing it in all error checks.
     * Would need to seek back anyway. */

    if (readsize != sizeof(header))
    {
        rb->splash(2*HZ, true, "Reading %s overlay failed.", name);
        return PLUGIN_ERROR;
    }
    if (header.magic != PLUGIN_MAGIC || header.target_id != TARGET_ID)
    {
        rb->splash(2*HZ, true, "%s overlay: Incompatible model.", name);
        return PLUGIN_ERROR;
    }
    if (header.api_version != PLUGIN_API_VERSION) 
    {
        rb->splash(2*HZ, true, "%s overlay: Incompatible version.", name);
        return PLUGIN_ERROR;
    }

    audiobuf = rb->plugin_get_audio_buffer(&audiobuf_size);
    if (header.load_addr < audiobuf ||
        header.end_addr > audiobuf + audiobuf_size)
    {
        rb->splash(2*HZ, true, "%s overlay doesn't fit into memory.", name);
        return PLUGIN_ERROR;
    }
    rb->memset(header.load_addr, 0, header.end_addr - header.load_addr);

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(2*HZ, true, "Can't open %s", filename);
        return PLUGIN_ERROR;
    }
    readsize = rb->read(fd, header.load_addr, header.end_addr - header.load_addr);
    rb->close(fd);

    if (readsize < 0)
    {
        rb->splash(2*HZ, true, "Reading %s overlay failed.", name);
        return PLUGIN_ERROR;
    }
    return header.entry_point(rb, parameter);
}
#endif
