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

#ifndef __XSCREENSAVER_RESOURCES_H__
#define __XSCREENSAVER_RESOURCES_H__
extern char *get_string_resource (char*,char*);
extern Bool get_boolean_resource (char*,char*);
extern int get_integer_resource (char*,char*);
extern double get_float_resource (char*,char*);
extern unsigned int get_pixel_resource (char*,char*,Display*,Colormap);
extern unsigned int get_minutes_resource (char*,char*);
extern unsigned int get_seconds_resource (char*,char*);
extern int parse_time (const char *string, Bool seconds_default_p,
                       Bool silent_p);
#endif /* __XSCREENSAVER_RESOURCES_H__ */
