/* xscreensaver, Copyright (c) 1993, 1994, 1995, 1996, 1997, 1998, 1999
 *  by Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* This file contains some code for intelligently picking the best visual
   (where "best" is biased in the direction of either: high color counts;
   or: having writable color cells...)
 */

#include "utils.h"
#include "resources.h"  /* for get_string_resource() */
#include "visual.h"

#include <X11/Xutil.h>

extern char *progname;


#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif


static Visual *pick_best_visual (Screen *, Bool, Bool);
static Visual *pick_mono_visual (Screen *);
static Visual *pick_best_visual_of_class (Screen *, int);
static Visual *pick_best_gl_visual (Screen *);
static Visual *id_to_visual (Screen *, int);
static Visual *id_to_visual (Screen *screen, int id);


#define DEFAULT_VISUAL	-1
#define BEST_VISUAL	-2
#define MONO_VISUAL	-3
#define GRAY_VISUAL	-4
#define COLOR_VISUAL	-5
#define GL_VISUAL	-6
#define SPECIFIC_VISUAL	-7

Visual *
get_visual (Screen *screen, const char *string, Bool prefer_writable_cells,
	    Bool verbose_p)
{
  char *v = (string ? strdup(string) : 0);
  char c, *tmp;
  int vclass;
  unsigned long id;
  Visual *result = 0;

  if (v)
    for (tmp = v; *tmp; tmp++)
      if (isupper (*tmp)) *tmp = _tolower (*tmp);

  if (!v || !*v)				  vclass = BEST_VISUAL;
  else if (!strcmp (v, "default"))		  vclass = DEFAULT_VISUAL;
  else if (!strcmp (v, "best")) 		  vclass = BEST_VISUAL;
  else if (!strcmp (v, "mono")) 		  vclass = MONO_VISUAL;
  else if (!strcmp (v, "monochrome")) 		  vclass = MONO_VISUAL;
  else if (!strcmp (v, "gray")) 		  vclass = GRAY_VISUAL;
  else if (!strcmp (v, "grey")) 		  vclass = GRAY_VISUAL;
  else if (!strcmp (v, "color")) 		  vclass = COLOR_VISUAL;
  else if (!strcmp (v, "gl"))	 		  vclass = GL_VISUAL;
  else if (!strcmp (v, "staticgray"))	 	  vclass = StaticGray;
  else if (!strcmp (v, "staticcolor"))		  vclass = StaticColor;
  else if (!strcmp (v, "truecolor"))		  vclass = TrueColor;
  else if (!strcmp (v, "grayscale"))		  vclass = GrayScale;
  else if (!strcmp (v, "greyscale"))		  vclass = GrayScale;
  else if (!strcmp (v, "pseudocolor"))		  vclass = PseudoColor;
  else if (!strcmp (v, "directcolor"))		  vclass = DirectColor;
  else if (1 == sscanf (v, " %ld %c", &id, &c))	  vclass = SPECIFIC_VISUAL;
  else if (1 == sscanf (v, " 0x%lx %c", &id, &c)) vclass = SPECIFIC_VISUAL;
  else
    {
      fprintf (stderr, "%s: unrecognized visual \"%s\".\n", progname, v);
      vclass = DEFAULT_VISUAL;
    }

  if (vclass == DEFAULT_VISUAL)
    result = DefaultVisualOfScreen (screen);
  else if (vclass == BEST_VISUAL)
    result = pick_best_visual (screen, prefer_writable_cells, False);
  else if (vclass == MONO_VISUAL)
    {
      result = pick_mono_visual (screen);
      if (!result && verbose_p)
	fprintf (stderr, "%s: no monochrome visuals.\n", progname);
    }
  else if (vclass == GRAY_VISUAL)
    {
      if (prefer_writable_cells)
	result = pick_best_visual_of_class (screen, GrayScale);
      if (!result)
	result = pick_best_visual_of_class (screen, StaticGray);
      if (!result)
	result = pick_best_visual_of_class (screen, GrayScale);
      if (!result && verbose_p)
	fprintf (stderr, "%s: no GrayScale or StaticGray visuals.\n",
		 progname);
    }
  else if (vclass == COLOR_VISUAL)
    {
      int class;
      /* First see if the default visual will do. */
      result = DefaultVisualOfScreen (screen);
      class = visual_class(screen, result);
      if (class != TrueColor &&
	  class != PseudoColor &&
	  class != DirectColor &&
	  class != StaticColor)
	result = 0;
      if (result && visual_depth(screen, result) <= 1)
	result = 0;

      /* Else, find the best non-default color visual */
      if (!result)
	result = pick_best_visual (screen, prefer_writable_cells, True);

      if (!result && verbose_p)
	fprintf (stderr, "%s: no color visuals.\n", progname);
    }
  else if (vclass == GL_VISUAL)
    {
      Visual *visual = pick_best_gl_visual (screen);
      if (visual)
	result = visual;
      else if (verbose_p)
	fprintf (stderr, "%s: no visual suitable for GL.\n", progname);
    }
  else if (vclass == SPECIFIC_VISUAL)
    {
      result = id_to_visual (screen, id);
      if (!result && verbose_p)
	fprintf (stderr, "%s: no visual with id 0x%x.\n", progname,
		 (unsigned int) id);
    }
  else
    {
      Visual *visual = pick_best_visual_of_class (screen, vclass);
      if (visual)
	result = visual;
      else if (verbose_p)
	fprintf (stderr, "%s: no visual of class %s.\n", progname, v);
    }

  if (v) free (v);
  return result;
}

Visual *
get_visual_resource (Screen *screen, char *name, char *class,
		     Bool prefer_writable_cells)
{
  char *string = get_string_resource (name, class);
  Visual *v = get_visual (screen, string, prefer_writable_cells, True);
  if (string)
    free(string);
  if (v)
    return v;
  else
    return DefaultVisualOfScreen (screen);
}


static Visual *
pick_best_visual (Screen *screen, Bool prefer_writable_cells, Bool color_only)
{
  Visual *visual;

  if (!prefer_writable_cells)
    {
      /* If we don't prefer writable cells, then the "best" visual is the one
	 on which we can allocate the largest range and number of colors.

	 Therefore, a TrueColor visual which is at least 16 bits deep is best.
	 (The assumption here being that a TrueColor of less than 16 bits is
	 really just a PseudoColor visual with a pre-allocated color cube.)

	 The next best thing is a PseudoColor visual of any type.  After that
	 come the non-colormappable visuals, and non-color visuals.
       */
      if ((visual = pick_best_visual_of_class (screen, TrueColor)) &&
	  visual_depth (screen, visual) >= 16)
	return visual;
    }

#define TRY_CLASS(CLASS) \
  if ((visual = pick_best_visual_of_class (screen, CLASS)) && \
      (!color_only || visual_depth(screen, visual) > 1)) \
    return visual
  TRY_CLASS(PseudoColor);
  TRY_CLASS(TrueColor);
  TRY_CLASS(DirectColor);
  TRY_CLASS(StaticColor);
  if (!color_only)
    {
      TRY_CLASS(GrayScale);
      TRY_CLASS(StaticGray);
    }
#undef TRY_CLASS

  visual = DefaultVisualOfScreen (screen);
  if (!color_only || visual_depth(screen, visual) > 1)
    return visual;
  else
    return 0;
}

static Visual *
pick_mono_visual (Screen *screen)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count;

  vi_in.depth = 1;
  vi_in.screen = screen_number (screen);
  vi_out = XGetVisualInfo (dpy, (VisualDepthMask | VisualScreenMask),
			   &vi_in, &out_count);
  if (vi_out)
    {
      Visual *v = (out_count > 0 ? vi_out [0].visual : 0);
      if (v && vi_out[0].depth != 1)
	v = 0;
      XFree ((char *) vi_out);
      return v;
    }
  else
    return 0;
}


static Visual *
pick_best_visual_of_class (Screen *screen, int visual_class)
{
  /* The best visual of a class is the one which on which we can allocate
     the largest range and number of colors, which means the one with the
     greatest depth and number of cells.

     (But actually, for XDaliClock, all visuals of the same class are
     probably equivalent - either we have writable cells or we don't.)
   */
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count;

  vi_in.class = visual_class;
  vi_in.screen = screen_number (screen);
  vi_out = XGetVisualInfo (dpy, (VisualClassMask | VisualScreenMask),
			   &vi_in, &out_count);
  if (vi_out)
    {
      /* choose the 'best' one, if multiple */
      int i, best;
      Visual *visual;
/*      for (i = 0, best = 0; i < out_count; i++) */
      for (i = out_count-1, best = i; i >= 0; i--) /* go backwards */
	/* It's better if it's deeper, or if it's the same depth with
	   more cells (does that ever happen?  Well, it could...) */
	if ((vi_out [i].depth > vi_out [best].depth) ||
	    ((vi_out [i].depth == vi_out [best].depth) &&
	     (vi_out [i].colormap_size > vi_out [best].colormap_size)))
	  best = i;
      visual = (best < out_count ? vi_out [best].visual : 0);
      XFree ((char *) vi_out);
      return visual;
    }
  else
    return 0;
}

static Visual *
pick_best_gl_visual (Screen *screen)
{
  /* The best visual for GL is a TrueColor visual that is half as deep as
     the screen.  If such a thing doesn't exist, then TrueColor is best.
     Failing that, the deepest available color visual is best.

     Compare this function to get_gl_visual() in visual-gl.c.
     This function tries to find the best GL visual using Xlib calls,
     whereas that function does the same thing using GLX calls.
   */
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count;
  Visual *result = 0;

  int ndepths = 0;
  int *depths = XListDepths (dpy, screen_number (screen), &ndepths);
  int screen_depth = depths[ndepths];
  XFree (depths);

  vi_in.class = TrueColor;
  vi_in.screen = screen_number (screen);
  vi_in.depth = screen_depth / 2;
  vi_out = XGetVisualInfo (dpy, (VisualClassMask | VisualScreenMask |
                                 VisualDepthMask),
			   &vi_in, &out_count);
  if (out_count > 0)
    result = vi_out[0].visual;

  if (vi_out)
    XFree ((char *) vi_out);

  if (!result && screen_depth > 24)
    {
      /* If it's a 32-deep screen and we didn't find a depth-16 visual,
         see if there's a depth-12 visual. */
      vi_in.class = TrueColor;
      vi_in.screen = screen_number (screen);
      vi_in.depth = 12;
      vi_out = XGetVisualInfo (dpy, (VisualClassMask | VisualScreenMask |
                                     VisualDepthMask),
                               &vi_in, &out_count);
      if (out_count > 0)
        result = vi_out[0].visual;
    }

  if (!result)
    /* No half-depth TrueColor?  Ok, try for any TrueColor (the deepest.) */
    result = pick_best_visual_of_class (screen, TrueColor);

  if (!result)
    /* No TrueColor?  Ok, try for anything. */
    result = pick_best_visual (screen, False, False);

  return result;
}


static Visual *
id_to_visual (Screen *screen, int id)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = id;
  vi_out = XGetVisualInfo (dpy, (VisualScreenMask | VisualIDMask),
			   &vi_in, &out_count);
  if (vi_out)
    {
      Visual *v = vi_out[0].visual;
      XFree ((char *) vi_out);
      return v;
    }
  return 0;
}

int
visual_depth (Screen *screen, Visual *visual)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count, d;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
			   &vi_in, &out_count);
  if (! vi_out) abort ();
  d = vi_out [0].depth;
  XFree ((char *) vi_out);
  return d;
}


#if 0
/* You very probably don't want to be using this.
   Pixmap depth doesn't refer to the depths of pixmaps, but rather, to
   the depth of protocol-level on-the-wire pixmap data, that is, XImages.
   To get this info, you should be looking at XImage->bits_per_pixel
   instead.  (And allocating the data for your XImage structures by
   multiplying ximage->bytes_per_line by ximage->height.)
 */
int
visual_pixmap_depth (Screen *screen, Visual *visual)
{
  Display *dpy = DisplayOfScreen (screen);
  int vdepth = visual_depth (screen, visual);
  int pdepth = vdepth;
  int i, pfvc = 0;
  XPixmapFormatValues *pfv = XListPixmapFormats (dpy, &pfvc);

  /* Return the first matching depth in the pixmap formats.  If there are no
     matching pixmap formats (which shouldn't be able to happen at all) then
     return the visual depth instead. */
  for (i = 0; i < pfvc; i++)
    if (pfv[i].depth == vdepth)
      {
	pdepth = pfv[i].bits_per_pixel;
	break;
      }
  if (pfv)
    XFree (pfv);
  return pdepth;
}
#endif /* 0 */


int
visual_class (Screen *screen, Visual *visual)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count, c;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
			   &vi_in, &out_count);
  if (! vi_out) abort ();
  c = vi_out [0].class;
  XFree ((char *) vi_out);
  return c;
}

Bool
has_writable_cells (Screen *screen, Visual *visual)
{
  switch (visual_class (screen, visual))
    {
    case GrayScale:	/* Mappable grays. */
    case PseudoColor:	/* Mappable colors. */
      return True;
    case StaticGray:	/* Fixed grays. */
    case TrueColor:	/* Fixed colors. */
    case StaticColor:	/* (What's the difference again?) */
    case DirectColor:	/* DirectColor visuals are like TrueColor, but have
			   three colormaps - one for each component of RGB.
			   Screw it. */
      return False;
    default:
      abort();
      return False;
    }
}

void
describe_visual (FILE *f, Screen *screen, Visual *visual, Bool private_cmap_p)
{
  char n[10];
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, (VisualScreenMask | VisualIDMask),
			   &vi_in, &out_count);
  if (! vi_out) abort ();
  if (private_cmap_p)
    sprintf(n, "%3d", vi_out->colormap_size);
  else
    strcpy(n, "default");

  fprintf (f, "0x%02x (%s depth: %2d, cmap: %s)\n",
	   (unsigned int) vi_out->visualid,
	   (vi_out->class == StaticGray  ? "StaticGray, " :
	    vi_out->class == StaticColor ? "StaticColor," :
	    vi_out->class == TrueColor   ? "TrueColor,  " :
	    vi_out->class == GrayScale   ? "GrayScale,  " :
	    vi_out->class == PseudoColor ? "PseudoColor," :
	    vi_out->class == DirectColor ? "DirectColor," :
					   "UNKNOWN:    "),
	   vi_out->depth, n);
  XFree ((char *) vi_out);
}

int
screen_number (Screen *screen)
{
  Display *dpy = DisplayOfScreen (screen);
  int i;
  for (i = 0; i < ScreenCount (dpy); i++)
    if (ScreenOfDisplay (dpy, i) == screen)
      return i;
  abort ();
  return 0;
}

int
visual_cells (Screen *screen, Visual *visual)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  int out_count, c;
  vi_in.screen = screen_number (screen);
  vi_in.visualid = XVisualIDFromVisual (visual);
  vi_out = XGetVisualInfo (dpy, VisualScreenMask|VisualIDMask,
			   &vi_in, &out_count);
  if (! vi_out) abort ();
  c = vi_out [0].colormap_size;
  XFree ((char *) vi_out);
  return c;
}

Visual *
find_similar_visual(Screen *screen, Visual *old_visual)
{
  Display *dpy = DisplayOfScreen (screen);
  XVisualInfo vi_in, *vi_out;
  Visual *result = 0;
  int out_count;

  vi_in.screen = screen_number (screen);
  vi_in.class  = visual_class (screen, old_visual);
  vi_in.depth  = visual_depth (screen, old_visual);

  /* Look for a visual of the same class and depth.
   */
  vi_out = XGetVisualInfo (dpy, (VisualScreenMask | VisualClassMask |
				 VisualDepthMask),
			   &vi_in, &out_count);
  if (vi_out && out_count > 0)
    result = vi_out[0].visual;
  if (vi_out) XFree (vi_out);
  vi_out = 0;

  /* Failing that, look for a visual of the same class.
   */
  if (!result)
    {
      vi_out = XGetVisualInfo (dpy, (VisualScreenMask | VisualClassMask),
			       &vi_in, &out_count);
      if (vi_out && out_count > 0)
	result = vi_out[0].visual;
      if (vi_out) XFree (vi_out);
      vi_out = 0;
    }

  /* Failing that, return the default visual. */
  if (!result)
    result = DefaultVisualOfScreen (screen);

  return result;
}
