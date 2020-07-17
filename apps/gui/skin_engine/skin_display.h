/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002-2007 Björn Stenberg
 * Copyright (C) 2007-2008 Nicolas Pennequin
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
#include "config.h"
#include <stdio.h>
#include "wps_internals.h"
#include "skin_engine.h"
#include "statusbar-skinned.h"

#ifndef _SKIN_DISPLAY_H_
#define _SKIN_DISPLAY_H_


void draw_progressbar(struct gui_wps *gwps, int line, struct progressbar *pb);
void draw_playlist_viewer_list(struct gui_wps *gwps, struct playlistviewer *viewer);
/* clears the area where the image was shown */
void clear_image_pos(struct gui_wps *gwps, struct gui_img *img);
void wps_display_images(struct gui_wps *gwps, struct viewport* vp);


void skin_render_viewport(struct skin_element* viewport, struct gui_wps *gwps,
                        struct skin_viewport* skin_viewport, unsigned long refresh_type);


/* Evaluate the conditional that is at *token_index and return whether a skip
   has ocurred. *token_index is updated with the new position.
*/
int evaluate_conditional(struct gui_wps *gwps, int offset,
                         struct conditional *conditional, int num_options);
/* Display a line appropriately according to its alignment format.
   format_align contains the text, separated between left, center and right.
   line is the index of the line on the screen.
   scroll indicates whether the line is a scrolling one or not.
*/
void write_line(struct screen *display, struct align_pos *format_align,
                int line, bool scroll, struct line_desc *line_desc);
void draw_peakmeters(struct gui_wps *gwps, int line_number,
                     struct viewport *viewport);
#endif
