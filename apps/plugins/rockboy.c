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

#define OVL_NAME "/.rockbox/viewers/rockboy.ovl"
#define OVL_DISPLAYNAME "RockBoy"

struct plugin_api* rb;
unsigned char *audiobuf;
int audiobuf_size;

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int fh, readsize;
    struct {
        unsigned long magic;
        unsigned char *start_addr;
        unsigned char *end_addr;
        enum plugin_status(*entry_point)(struct plugin_api*, void*);
    } header;

    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);
    rb = api;

    fh = rb->open(OVL_NAME, O_RDONLY);
    if (fh < 0)
    {
        rb->splash(2*HZ, true, "Couldn't open " OVL_DISPLAYNAME " overlay.");
        return PLUGIN_ERROR;
    }
    readsize = rb->read(fh, &header, sizeof(header));
    if (readsize != sizeof(header) || header.magic != 0x524f564c)
    {
        rb->close(fh);
        rb->splash(2*HZ, true, OVL_NAME " is not a valid Rockbox overlay.");
        return PLUGIN_ERROR;
    }
    
    audiobuf = rb->plugin_get_audio_buffer(&audiobuf_size);
    if (header.start_addr < audiobuf ||
        header.end_addr > audiobuf + audiobuf_size)
    {
        rb->close(fh);
        rb->splash(2*HZ, true, OVL_DISPLAYNAME 
                   " overlay doesn't fit into memory.");
        return PLUGIN_ERROR;
    }
    rb->memset(header.start_addr, 0, header.end_addr - header.start_addr);

    rb->lseek(fh, 0, SEEK_SET);
    readsize = rb->read(fh, header.start_addr, header.end_addr - header.start_addr);
    rb->close(fh);
    if (readsize <= sizeof(header))
    {
        rb->splash(2*HZ, true, "Error loading " OVL_DISPLAYNAME " overlay.");
        return PLUGIN_ERROR;
    }
    return header.entry_point(api, parameter);
}
#endif
