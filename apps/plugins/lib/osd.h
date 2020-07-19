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

/* Callback implemented by user. Paramters are OSD vp-relative coordinates */
typedef void (* osd_draw_cb_fn_t)(int x, int y, int width, int height);

/* Initialize the OSD, set its backbuffer, update callback and enable it if
 * the call succeeded. */
enum osd_init_flags
{
    OSD_INIT_MAJOR_WIDTH = 0x0,  /* Width guides buffer dims (default) */
    OSD_INIT_MAJOR_HEIGHT = 0x1, /* Height guides buffer dims */
    OSD_INIT_MINOR_MIN = 0x2,    /* Treat minor axis dim as a minimum */
    OSD_INIT_MINOR_MAX = 0x4,    /* Treat minor axis dim as a maximum */
    /* To get exact minor size, combine min/max flags */
};
bool osd_init(unsigned flags, void *backbuf, size_t backbuf_size,
              osd_draw_cb_fn_t draw_cb, int *width,
              int *height, size_t *bufused);

/* Destroy the OSD, rendering it disabled */
void osd_destroy(void);

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

/* Redraw part of the OSD (OSD viewport-relative coordinates) returning true
   if any screen update occurred */
bool osd_update_rect(int x, int y, int width, int height);

/* Set a new screen location and size (screen coordinates) */
bool osd_update_pos(int x, int y, int width, int height);

/* Call periodically to have the OSD timeout and hide itself */
void osd_monitor_timeout(void);

/* Set the OSD timeout value. 'timeout' <= 0 == never timeout */
void osd_set_timeout(long timeout);

/* Use the OSD viewport context */
struct viewport * osd_get_viewport(void);

/* Get the maximum buffer dimensions calculated by osd_init() */
void osd_get_max_dims(int *maxwidth, int *maxheight);

/* Is the OSD enabled? */
bool osd_enabled(void);

/** Functions that must be used in lieu of regular LCD functions for this
 ** to work.
 **
 ** To be efficient, as much drawing as possible should be combined between
 ** *prepare and *update.
 **
 ** osd_lcd_update_prepare();
 ** <draw stuff using lcd_* routines>
 ** osd_lcd_update[_rect]();
 **
 ** TODO: Make it work seamlessly with greylib and regular LCD functions.
 **/

/* Prepare LCD frambuffer for regular drawing - call before any other LCD
   function */
void osd_lcd_update_prepare(void);

/* Update the whole screen and restore OSD if it is visible */
void osd_lcd_update(void);

/* Update a part of the screen and restore OSD if it is visible */
void osd_lcd_update_rect(int x, int y, int width, int height);

#if LCD_DEPTH < 4
/* Like other functions but for greylib surface (requires GREY_BUFFERED) */
bool osd_grey_init(unsigned flags, void *backbuf, size_t backbuf_size,
                   osd_draw_cb_fn_t draw_cb, int *width,
                   int *height, size_t *bufused);
void osd_grey_destroy(void);
bool osd_grey_show(unsigned flags);
bool osd_grey_update(void);
bool osd_grey_update_rect(int x, int y, int width, int height);
bool osd_grey_update_pos(int x, int y, int width, int height);
void osd_grey_monitor_timeout(void);
void osd_grey_set_timeout(long timeout);
struct viewport * osd_grey_get_viewport(void);
void osd_grey_get_max_dims(int *maxwidth, int *maxheight);
bool osd_grey_enabled(void);
void osd_grey_lcd_update_prepare(void);
void osd_grey_lcd_update(void);
void osd_grey_lcd_update_rect(int x, int y, int width, int height);
#endif /* LCD_DEPTH < 4 */

/* MYLCD-style helper defines to compile with different graphics libs */
#ifdef __GREY_H__
#define myosd_(fn)                  osd_grey_##fn
#else
#define myosd_(fn)                  osd_##fn
#endif

#define myosd_init                  myosd_(init)
#define myosd_destroy               myosd_(destroy)
#define myosd_show                  myosd_(show)
#define myosd_update                myosd_(update)
#define myosd_update_rect           myosd_(update_rect)
#define myosd_update_pos            myosd_(update_pos)
#define myosd_monitor_timeout       myosd_(monitor_timeout)
#define myosd_set_timeout           myosd_(set_timeout)
#define myosd_get_viewport          myosd_(get_viewport)
#define myosd_get_max_dims          myosd_(get_max_dims)
#define myosd_enabled               myosd_(enabled)
#define myosd_lcd_update_prepare    myosd_(lcd_update_prepare)
#define myosd_lcd_update            myosd_(lcd_update)
#define myosd_lcd_update_rect       myosd_(lcd_update_rect)

#endif /* OSD_H */
