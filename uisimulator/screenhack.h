/* xscreensaver, Copyright (c) 1992-1997 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* Found in Don Hopkins' .plan file:
 *
 *   The color situation is a total flying circus.  The X approach to
 *   device independence is to treat everything like a MicroVax framebuffer
 *   on acid.  A truely portable X application is required to act like the
 *   persistent customer in the Monty Python ``Cheese Shop'' sketch.  Even
 *   the simplest applications must answer many difficult questions, like:
 *
 *   WHAT IS YOUR DISPLAY?
 *       display = XOpenDisplay("unix:0");
 *   WHAT IS YOUR ROOT?
 *       root = RootWindow(display, DefaultScreen(display));
 *   AND WHAT IS YOUR WINDOW?
 *       win = XCreateSimpleWindow(display, root, 0, 0, 256, 256, 1,
 *                                 BlackPixel(display, DefaultScreen(display)),
 *                                 WhitePixel(display, DefaultScreen(display)))
 *   OH ALL RIGHT, YOU CAN GO ON.
 *
 *   WHAT IS YOUR DISPLAY?
 *         display = XOpenDisplay("unix:0");
 *   WHAT IS YOUR COLORMAP?
 *         cmap = DefaultColormap(display, DefaultScreen(display));
 *   AND WHAT IS YOUR FAVORITE COLOR?
 *         favorite_color = 0; / * Black. * /
 *         / * Whoops! No, I mean: * /
 *         favorite_color = BlackPixel(display, DefaultScreen(display));
 *         / * AAAYYYYEEEEE!! (client dumps core & falls into the chasm) * /
 *
 *   WHAT IS YOUR DISPLAY?
 *         display = XOpenDisplay("unix:0");
 *   WHAT IS YOUR VISUAL?
 *         struct XVisualInfo vinfo;
 *         if (XMatchVisualInfo(display, DefaultScreen(display),
 *                              8, PseudoColor, &vinfo) != 0)
 *            visual = vinfo.visual;
 *   AND WHAT IS THE NET SPEED VELOCITY OF AN XConfigureWindow REQUEST?
 *         / * Is that a SubStructureRedirectMask or a ResizeRedirectMask? * /
 *   WHAT?! HOW AM I SUPPOSED TO KNOW THAT?
 *   AAAAUUUGGGHHH!!!! (server dumps core & falls into the chasm)
 */

#ifndef __SCREENHACK_H__
#define __SCREENHACK_H__

#include <stdlib.h>

#include "config.h"

#ifdef __hpux
 /* Which of the ten billion standards does values.h belong to?
    What systems always have it? */
# include <values.h>
#endif

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xos.h>

/* M_PI ought to have been defined in math.h, but... */
#ifndef M_PI
# define M_PI 3.1415926535
#endif

#ifndef M_PI_2
# define M_PI_2 1.5707963267
#endif

#include "yarandom.h"
#include "usleep.h"
#include "resources.h"
#include "hsv.h"
#include "colors.h"
#include "visual.h"

extern Bool mono_p;
extern char *progname;
extern char *progclass;
extern XrmDatabase db;
extern XrmOptionDescRec options [];
extern char *defaults [];

extern void screenhack (Display*,Window);
extern void screenhack_handle_event (Display*, XEvent*);
extern void screenhack_handle_events (Display*);
extern void screen_redraw();
extern void screen_resized();

#endif /* __SCREENHACK_H__ */
