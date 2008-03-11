/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PLAYBACK_CONTROL_H__
#define __PLAYBACK_CONTROL_H__

/* Use these if your menu uses the new menu api. 
   REMEBER to call playback_control_init(rb) before rb->do_menu()... */
extern const struct menu_item_ex playback_control_menu;
void playback_control_init(struct plugin_api* newapi);

/* Use this if your menu still uses the old menu api */
bool playback_control(struct plugin_api* api);

#endif /* __PLAYBACK_CONTROL_H__ */
