/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Jens Arnold
 *
 * Overlay loader for rockboy on Archos
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

#if MEM <= 8 && !defined(SIMULATOR)

PLUGIN_HEADER

#define OVL_NAME "/.rockbox/viewers/rockboy.ovl"
#define OVL_DISPLAYNAME "RockBoy"

struct plugin_api* rb;
unsigned char *audiobuf;
int audiobuf_size;

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int fd, readsize;
    struct plugin_header header;

    rb = api;

    fd = rb->open(OVL_NAME, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(2*HZ, true, "Can't open " OVL_NAME);
        return PLUGIN_ERROR;
    }
    readsize = rb->read(fd, &header, sizeof(header));
    rb->close(fd);
    /* Close for now. Less code than doing it in all error checks.
     * Would need to seek back anyway. */

    if (readsize != sizeof(header))
    {
        rb->splash(2*HZ, true, "Reading" OVL_DISPLAYNAME " overlay failed.");
        return PLUGIN_ERROR;
    }
    if (header.magic != PLUGIN_MAGIC || header.target_id != TARGET_ID)
    {
        rb->splash(2*HZ, true, OVL_DISPLAYNAME 
                               " overlay: Incompatible model.");
        return PLUGIN_ERROR;
    }
    if (header.api_version != PLUGIN_API_VERSION) 
    {
        rb->splash(2*HZ, true, OVL_DISPLAYNAME
                               " overlay: Incompatible version.");
        return PLUGIN_ERROR;
    }

    audiobuf = rb->plugin_get_audio_buffer(&audiobuf_size);
    if (header.load_addr < audiobuf ||
        header.end_addr > audiobuf + audiobuf_size)
    {
        rb->splash(2*HZ, true, OVL_DISPLAYNAME 
                               " overlay doesn't fit into memory.");
        return PLUGIN_ERROR;
    }
    rb->memset(header.load_addr, 0, header.end_addr - header.load_addr);

    fd = rb->open(OVL_NAME, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(2*HZ, true, "Can't open " OVL_NAME);
        return PLUGIN_ERROR;
    }
    readsize = rb->read(fd, header.load_addr, header.end_addr - header.load_addr);
    rb->close(fd);

    if (readsize < 0)
    {
        rb->splash(2*HZ, true, "Reading" OVL_DISPLAYNAME " overlay failed.");
        return PLUGIN_ERROR;
    }
    return header.entry_point(api, parameter);
}
#endif
