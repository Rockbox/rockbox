/* xscreensaver, Copyright (c) 1992, 1996 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __SCREENHACK_USLEEP_H__
#define __SCREENHACK_USLEEP_H__

extern void screenhack_usleep (unsigned long usecs);

#undef usleep
#define usleep(usecs) screenhack_usleep(usecs)

#endif /* __SCREENHACK_USLEEP_H__ */
