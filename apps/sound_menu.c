/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdbool.h>
#include "menu.h"
#include "mpeg.h"
#include "settings.h"

static void volume(void)
{
    set_int("Volume","%", &global_settings.volume, mpeg_volume, 2, 0, 100);
}

static void bass(void)
{
    set_int("Bass","%", &global_settings.bass, mpeg_bass, 2, 0, 100);
};

static void treble(void)
{
    set_int("Treble","%", &global_settings.treble, mpeg_treble, 2, 0, 100);
}

void sound_menu(void)
{
    int m;
    struct menu_items items[] = {
        { "Volume", volume },
        { "Bass",   bass },
        { "Treble", treble }
    };
    
    m=menu_init( items, sizeof items / sizeof(struct menu_items) );
    menu_run(m);
    menu_exit(m);
}
