/* This file contains compatibility routines for systems without Xmu.
 * You would be better served by installing Xmu on your machine or
 * yelling at your vendor to ship it.
 */

#ifndef __XMU_H__
#define __XMU_H__

#include <X11/Xlib.h>
#include <stdio.h>

int XmuPrintDefaultErrorMessage (Display *dpy, XErrorEvent *event, FILE *fp);

#endif /* __XMU_H__ */
