/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Floating on-screen display
*
* Copyright (C) 2012 Michael Sevakis
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
#ifndef OSD_H
#define OSD_H

/* At this time: assumes use of the default viewport for normal drawing */

/* Callback implemented by user */
typedef void (* osd_draw_cb_fn_t)(int x, int y, int width, int height);

/* Initialized the OSD and set its backbuffer */
bool osd_init(void *backbuf, size_t backbuf_size,
              osd_draw_cb_fn_t draw_cb);

enum
{
    OSD_HIDE      = 0x0, /* Hide from view */
    OSD_SHOW      = 0x1, /* Bring into view, updating if needed */
    OSD_UPDATENOW = 0x2, /* Force update even if nothing changed */
};

/* Show/Hide the OSD on screen returning previous status */
bool osd_show(unsigned flags);

/* Redraw the entire OSD returning true if screen update occurred */
bool osd_update(void);

/* Redraw part of the OSD (viewport-relative coordinates) returning true
   if screen update occurred */
bool osd_update_rect(int x, int y, int width, int height);

/* Set a new screen location and size (screen coordinates) */
bool osd_update_pos(int x, int y, int width, int height);

/* Call periodically to have the OSD timeout and hide itself */
void osd_monitor_timeout(void);

/* Set the OSD timeout value. <= 0 = never timeout */
void osd_set_timeout(long timeout);

/* Use the OSD viewport context */
struct viewport * osd_get_viewport(void);

/* Get the maximum dimensions calculated by osd_init() */
void osd_get_max_dims(int *maxwidth, int *maxheight);

/* Is the OSD enabled? */
bool osd_enabled(void);

/** Functions that must be used in lieu of regular LCD functions for this
 *  to work **/

/* Prepare LCD frambuffer for regular drawing - call before any other LCD
   function */
void osd_prepare_draw(void);

/* Update the whole screen */
void osd_lcd_update(void);

/* Update a part of the screen */
void osd_lcd_update_rect(int x, int y, int width, int height);

#endif /* OSD_H */
