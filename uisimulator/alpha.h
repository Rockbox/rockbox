/* xscreensaver, Copyright (c) 1992, 1995, 1996, 1997
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

#ifndef __XSCREENSAVER_ALPHA_H__
#define __XSCREENSAVER_ALPHA_H__

extern Bool allocate_alpha_colors (Screen *screen, Visual *visual,
                                   Colormap cmap,
                                   int *nplanesP, Bool additive_p,
                                   unsigned long **plane_masks,
                                   unsigned long *base_pixelP);

#endif /* __XSCREENSAVER_ALPHA_H__ */
