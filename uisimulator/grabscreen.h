/* xscreensaver, Copyright (c) 1992, 1993, 1994, 1997
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __GRABSCREEN_H__
#define __GRABSCREEN_H__

/* This will write a snapshot of the screen image into the given window.
   Beware that the colormap of the window may also be changed (to match
   the bits that were drawn.)
 */
extern void grab_screen_image (Screen *, Window);

/* Whether one should use GCSubwindowMode when drawing on this window
   (assuming a screen image has been grabbed onto it.)  Yes, this is a
   total kludge. */
extern Bool use_subwindow_mode_p(Screen *screen, Window window);

#endif /* __GRABSCREEN_H__ */
