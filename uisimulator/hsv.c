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

/* This file contains some utility routines for randomly picking the colors
   to hack the screen with.
 */

#include "utils.h"
#include "hsv.h"

void
hsv_to_rgb (int h, double s, double v,
	    unsigned short *r, unsigned short *g, unsigned short *b)
{
  double H, S, V, R, G, B;
  double p1, p2, p3;
  double f;
  int i;

  if (s < 0) s = 0;
  if (v < 0) v = 0;
  if (s > 1) s = 1;
  if (v > 1) v = 1;

  S = s; V = v;
  H = (h % 360) / 60.0;
  i = H;
  f = H - i;
  p1 = V * (1 - S);
  p2 = V * (1 - (S * f));
  p3 = V * (1 - (S * (1 - f)));
  if	  (i == 0) { R = V;  G = p3; B = p1; }
  else if (i == 1) { R = p2; G = V;  B = p1; }
  else if (i == 2) { R = p1; G = V;  B = p3; }
  else if (i == 3) { R = p1; G = p2; B = V;  }
  else if (i == 4) { R = p3; G = p1; B = V;  }
  else		   { R = V;  G = p1; B = p2; }
  *r = R * 65535;
  *g = G * 65535;
  *b = B * 65535;
}

void
rgb_to_hsv (unsigned short r, unsigned short g, unsigned short b,
	    int *h, double *s, double *v)
{
  double R, G, B, H, S, V;
  double cmax, cmin;
  double cmm;
  int imax;
  R = ((double) r) / 65535.0;
  G = ((double) g) / 65535.0;
  B = ((double) b) / 65535.0;
  cmax = R; cmin = G; imax = 1;
  if  ( cmax < G ) { cmax = G; cmin = R; imax = 2; }
  if  ( cmax < B ) { cmax = B; imax = 3; }
  if  ( cmin > B ) { cmin = B; }
  cmm = cmax - cmin;
  V = cmax;
  if (cmm == 0)
    S = H = 0;
  else
    {
      S = cmm / cmax;
      if       (imax == 1)    H =       (G - B) / cmm;
      else  if (imax == 2)    H = 2.0 + (B - R) / cmm;
      else /*if (imax == 3)*/ H = 4.0 + (R - G) / cmm;
      if (H < 0) H += 6.0;
    }
  *h = (H * 60.0);
  *s = S;
  *v = V;
}
