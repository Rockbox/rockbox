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

/* Beauty is only skin deep, unless you've got an alpha channel. */


#include "utils.h"
#include "alpha.h"
#include "visual.h"
#include "hsv.h"
#include "yarandom.h"
#include "resources.h"

#include <X11/Xutil.h>

#ifndef countof
# define countof(x) (sizeof(*(x))/sizeof((x)))
#endif


/* I don't believe this fucking language doesn't have builtin exponentiation.
   I further can't believe that the fucking ^ character means fucking XOR!! */
static int 
i_exp (int i, int j)
{
  int k = 1;
  while (j--) k *= i;
  return k;
}


static void
merge_colors (int argc, XColor **argv, XColor *into_color, int mask,
	      Bool additive_p)
{
  int j;
  *into_color = *argv [0];
  into_color->pixel |= mask;

  for (j = 1; j < argc; j++)
    {
# define SHORT_INC(x,y) (x = ((((x)+(y)) > 0xFFFF) ? 0xFFFF : ((x)+(y))))
# define SHORT_DEC(x,y) (x = ((((x)-(y)) < 0)      ? 0      : ((x)-(y))))
      if (additive_p)
	{
	  SHORT_INC (into_color->red,   argv[j]->red);
	  SHORT_INC (into_color->green, argv[j]->green);
	  SHORT_INC (into_color->blue,  argv[j]->blue);
	}
      else
	{
	  SHORT_DEC (into_color->red,   argv[j]->red);
	  SHORT_DEC (into_color->green, argv[j]->green);
	  SHORT_DEC (into_color->blue,  argv[j]->blue);
	}
# undef SHORT_INC
# undef SHORT_DEC
    }
}

static void
permute_colors (XColor *pcolors, XColor *colors,
		int count,
		unsigned long *plane_masks,
		Bool additive_p)
{
  int out = 0;
  int max = i_exp (2, count);
  if (count > 31) abort ();
  for (out = 1; out < max; out++)
    {
      XColor *argv [32];
      int this_mask = 0;
      int argc = 0;
      int bit;
      for (bit = 0; bit < 32; bit++)
	if (out & (1<<bit))
	  {
	    argv [argc++] = &pcolors [bit];
	    this_mask |= plane_masks [bit];
	  }
      merge_colors (argc, argv, &colors [out-1], this_mask, additive_p);
    }
}


static int
allocate_color_planes (Display *dpy, Colormap cmap,
		       int nplanes, unsigned long *plane_masks,
		       unsigned long *base_pixel_ret)
{
  while (nplanes > 1 &&
	 !XAllocColorCells (dpy, cmap, False, plane_masks, nplanes,
			    base_pixel_ret, 1))
    nplanes--;

  return nplanes;
}
		       

static void
initialize_transparency_colormap (Display *dpy, Colormap cmap,
				  int nplanes,
				  unsigned long base_pixel,
				  unsigned long *plane_masks,
				  XColor *colors,
				  Bool additive_p)
{
  int i;
  int total_colors = i_exp (2, nplanes);
  XColor *all_colors = (XColor *) calloc (total_colors, sizeof (XColor));

  for (i = 0; i < nplanes; i++)
    colors[i].pixel = base_pixel | plane_masks [i];
  permute_colors (colors, all_colors, nplanes, plane_masks, additive_p);

  /* clone the default background of the window into our "base" pixel */
  all_colors [total_colors - 1].pixel =
    get_pixel_resource ("background", "Background", dpy, cmap);
  XQueryColor (dpy, cmap, &all_colors [total_colors - 1]);
  all_colors [total_colors - 1].pixel = base_pixel;

  for (i = 0; i < total_colors; i++)
    all_colors[i].flags = DoRed|DoGreen|DoBlue;
  XStoreColors (dpy, cmap, all_colors, total_colors);
  XFree ((XPointer) all_colors);
}


Bool
allocate_alpha_colors (Screen *screen, Visual *visual, Colormap cmap,
		       int *nplanesP, Bool additive_p,
		       unsigned long **plane_masks,
		       unsigned long *base_pixelP)
{
  Display *dpy = DisplayOfScreen (screen);
  XColor *colors;
  int nplanes = *nplanesP;
  int i;

  if (!has_writable_cells (screen, visual))
    cmap = 0;

  if (!cmap)            /* A TrueColor visual, or similar. */
    {
      int depth = visual_depth (screen, visual);
      unsigned long masks;
      XVisualInfo vi_in, *vi_out;

      /* Find out which bits the R, G, and B components actually occupy
         on this visual. */
      vi_in.screen = screen_number (screen);
      vi_in.visualid = XVisualIDFromVisual (visual);
      vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
                               &vi_in, &i);
      if (! vi_out) abort ();
      masks = vi_out[0].red_mask | vi_out[0].green_mask | vi_out[0].blue_mask;
      XFree ((char *) vi_out);

      if (nplanes > depth)
        nplanes = depth;
      *nplanesP = nplanes;
      *base_pixelP = 0;
      *plane_masks = (unsigned long *) calloc(sizeof(unsigned long), nplanes);

      /* Pick the planar values randomly, but constrain them to fall within
         the bit positions of the R, G, and B fields. */
      for (i = 0; i < nplanes; i++)
        (*plane_masks)[i] = random() & masks;

    }
  else                  /* A PseudoColor visual, or similar. */
    {
      if (nplanes > 31) nplanes = 31;
      *plane_masks = (unsigned long *) malloc(sizeof(unsigned long) * nplanes);

      nplanes = allocate_color_planes (dpy, cmap, nplanes, *plane_masks,
				   base_pixelP);
      *nplanesP = nplanes;

      if (nplanes <= 1)
        {
          free(*plane_masks);
          *plane_masks = 0;
          return False;
        }

      colors = (XColor *) calloc (nplanes, sizeof (XColor));
      for (i = 0; i < nplanes; i++)
        {
          /* pick the base colors. If we are in subtractive mode, pick higher
             intensities. */
          hsv_to_rgb (random () % 360,
                      frand (1.0),
                      frand (0.5) + (additive_p ? 0.2 : 0.5),
                      &colors[i].red,
                      &colors[i].green,
                      &colors[i].blue);
        }
      initialize_transparency_colormap (dpy, cmap, nplanes,
                                        *base_pixelP, *plane_masks, colors,
                                        additive_p);
      XFree ((XPointer) colors);
    }
  return True;
}
