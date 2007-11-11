/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _ALBUMART_H_
#define _ALBUMART_H_

#ifdef HAVE_ALBUMART

#include <stdbool.h>
#include "id3.h"
#include "gwps.h"

/* Look for albumart bitmap in the same dir as the track and in its parent dir.
 * Stores the found filename in the buf parameter.
 * Returns true if a bitmap was found, false otherwise */
bool find_albumart(const struct mp3entry *id3, char *buf, int buflen);

/* Draw the album art bitmap from the given handle ID onto the given WPS. */
void draw_album_art(struct gui_wps *gwps, int handle_id);

#endif /* HAVE_ALBUMART */

#endif /* _ALBUMART_H_ */
