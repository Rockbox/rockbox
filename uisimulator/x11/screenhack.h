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

#ifndef __SCREENHACK_H__
#define __SCREENHACK_H__

#include <stdlib.h>
#include <stdbool.h>

#include "config-x11.h"

#ifdef __hpux
 /* Which of the ten billion standards does values.h belong to?
    What systems always have it? */
# include <values.h>
#endif

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xos.h>
#include <X11/Intrinsic.h>

#include "resources.h"
#include "visual.h"

extern Bool mono_p;
extern char *progname;
extern char *progclass;
extern XrmDatabase db;
extern XrmOptionDescRec options [];
extern char *defaults [];
extern XtAppContext app;
extern Display* dpy;
extern Window window;

extern void screenhack();
extern int screenhack_handle_event(XEvent*, bool *);
extern int screenhack_handle_events(bool *);
extern void screen_redraw();
extern void screen_resized();

#endif /* __SCREENHACK_H__ */
