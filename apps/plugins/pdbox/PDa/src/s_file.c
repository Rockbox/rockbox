/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*
 * this file contains file-handling routines.
 */

#include "m_pd.h"
#include "s_stuff.h"
#include <sys/types.h>
#include <sys/stat.h>

    /* LATER delete this? -- replaced by find_via_path() in s_path.c */
int sys_isreadablefile(const char *s)
{
    struct stat statbuf;
    int mode;
    if (stat(s, &statbuf) < 0) return (0);
#ifdef UNIX
    mode = statbuf.st_mode;
    if (S_ISDIR(mode)) return (0);
#endif
    return (1);
}

    /* change '/' characters to the system's native file separator */
void sys_bashfilename(const char *from, char *to)
{
    char c;
    while (c = *from++)
    {
#ifdef MSW
    	if (c == '/') c = '\\';
#endif
    	*to++ = c;
    }
    *to = 0;
}


    /* change the system's native file separator to '/' characters  */
void sys_unbashfilename(const char *from, char *to)
{
    char c;
    while (c = *from++)
    {
#ifdef MSW
    	if (c == '\\') c = '/';
#endif
    	*to++ = c;
    }
    *to = 0;
}

