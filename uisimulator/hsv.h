/* xscreensaver, Copyright (c) 1992, 1997 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __HSV_H__
#define __HSV_H__

/* Converts between RGB and HSV color spaces.
   R, G, and B are in the range 0 - 65535;
   H is in the range 0 - 360;
   S and V are in the range 0.0 - 1.0.
 */
extern void hsv_to_rgb (int h, double s, double v,
			unsigned short *r,
			unsigned short *g,
			unsigned short *b);
extern void rgb_to_hsv (unsigned short r, unsigned short g, unsigned short b,
			int *h, double *s, double *v);

#endif /* __HSV_H__ */
