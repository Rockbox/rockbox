/* xscreensaver, Copyright (c) 1992, 1996, 1997
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#else  /* !HAVE_CONFIG_H */
# ifndef NO_SELECT
#  define HAVE_SELECT
# endif
#endif /* !HAVE_CONFIG_H */

#ifdef __STDC__
# include <stdlib.h>
#endif

#if defined(VMS)
# include <descrip.h>
# include <stdio.h>
# include <lib$routines.h>
#elif defined(HAVE_SELECT)
# include <sys/time.h>		/* for struct timeval */
#endif


#ifdef __SCREENHACK_USLEEP_H__
ERROR, do not include that here
#endif

void
screenhack_usleep (unsigned long usecs)
{
# if defined(VMS)
  float seconds = ((float) usecs)/1000000.0;
  unsigned long int statvms = lib$wait(&seconds);

#elif defined(HAVE_SELECT)
  /* usleep() doesn't exist everywhere, and select() is faster anyway. */
  struct timeval tv;
  tv.tv_sec  = usecs / 1000000L;
  tv.tv_usec = usecs % 1000000L;
  (void) select (0, 0, 0, 0, &tv);

#else /* !VMS && !HAVE_SELECT */
  /* If you don't have select() or usleep(), I guess you lose...
     Maybe you have napms() instead?  Let me know. */
  usleep (usecs);

#endif /* !VMS && !HAVE_SELECT */
}
