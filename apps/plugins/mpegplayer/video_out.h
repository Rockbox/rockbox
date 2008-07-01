/*
 * video_out.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 * libmpeg2 sync history:
 * 2008-07-01 - CVS revision 1.22
 */

#ifndef VIDEO_OUT_H
#define VIDEO_OUT_H

#if LCD_WIDTH >= LCD_HEIGHT
#define SCREEN_WIDTH LCD_WIDTH
#define SCREEN_HEIGHT LCD_HEIGHT
#define LCD_LANDSCAPE
#else /* Assume the screen is rotated on portrait LCDs */
#define SCREEN_WIDTH LCD_HEIGHT
#define SCREEN_HEIGHT LCD_WIDTH
#define LCD_PORTRAIT
#endif

/* Structure to hold width and height values */
struct vo_ext
{
    int w, h;
};

/* Structure that defines a rectangle by its edges */
struct vo_rect
{
    int l, t, r, b;
};

void vo_draw_frame (uint8_t * const * buf);
bool vo_draw_frame_thumb (uint8_t * const * buf,
                          const struct vo_rect *rc);
bool vo_init (void);
bool vo_show (bool show);
bool vo_is_visible(void);
void vo_setup (const mpeg2_sequence_t * sequence);
void vo_set_clip_rect(const struct vo_rect *rc);
void vo_dimensions(struct vo_ext *sz);
void vo_cleanup (void);

#if NUM_CORES > 1
void vo_lock(void);
void vo_unlock(void);
#else
static inline void vo_lock(void) {}
static inline void vo_unlock(void) {}
#endif

/* Sets all coordinates of a vo_rect to 0 */
void vo_rect_clear(struct vo_rect *rc);
/* Returns true if left >= right or top >= bottom */
bool vo_rect_empty(const struct vo_rect *rc);
/* Initializes a vo_rect using upper-left corner and extents */
void vo_rect_set_ext(struct vo_rect *rc, int x, int y,
                     int width, int height);
/* Query if two rectangles intersect
 * If either are empty returns false */
bool vo_rects_intersect(const struct vo_rect *rc1,
                        const struct vo_rect *rc2);

/* Intersect two rectangles
 * Resulting rectangle is placed in rc_dst.
 * rc_dst is set to empty if they don't intersect.
 * Empty source rectangles do not intersect any rectangle.
 * rc_dst may be the same structure as rc1 or rc2.
 * Returns true if the resulting rectangle is not empty. */
bool vo_rect_intersect(struct vo_rect *rc_dst,
                       const struct vo_rect *rc1,
                       const struct vo_rect *rc2);

bool vo_rect_union(struct vo_rect *rc_dst,
                   const struct vo_rect *rc1,
                   const struct vo_rect *rc2);

void vo_rect_offset(struct vo_rect *rc, int dx, int dy);

#endif /* VIDEO_OUT_H */
