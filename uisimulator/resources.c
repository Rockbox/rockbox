/* xscreensaver, Copyright (c) 1992, 1997, 1998
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

#include "utils.h"
#include "resources.h"
#include <X11/Xresource.h>


/* Resource functions.  Assumes: */

extern char *progname;
extern char *progclass;
extern XrmDatabase db;

static unsigned int get_time_resource (char *res_name, char *res_class,
				       Bool sec_p);

#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif

char *
get_string_resource (char *res_name, char *res_class)
{
  XrmValue value;
  char	*type;
  char full_name [1024], full_class [1024];
  strcpy (full_name, progname);
  strcat (full_name, ".");
  strcat (full_name, res_name);
  strcpy (full_class, progclass);
  strcat (full_class, ".");
  strcat (full_class, res_class);
  if (XrmGetResource (db, full_name, full_class, &type, &value))
    {
      char *str = (char *) malloc (value.size + 1);
      strncpy (str, (char *) value.addr, value.size);
      str [value.size] = 0;
      return str;
    }
  return 0;
}

Bool 
get_boolean_resource (char *res_name, char *res_class)
{
  char *tmp, buf [100];
  char *s = get_string_resource (res_name, res_class);
  char *os = s;
  if (! s) return 0;
  for (tmp = buf; *s; s++)
    *tmp++ = isupper (*s) ? _tolower (*s) : *s;
  *tmp = 0;
  free (os);

  while (*buf &&
	 (buf[strlen(buf)-1] == ' ' ||
	  buf[strlen(buf)-1] == '\t'))
    buf[strlen(buf)-1] = 0;

  if (!strcmp (buf, "on") || !strcmp (buf, "true") || !strcmp (buf, "yes"))
    return 1;
  if (!strcmp (buf,"off") || !strcmp (buf, "false") || !strcmp (buf,"no"))
    return 0;
  fprintf (stderr, "%s: %s must be boolean, not %s.\n",
	   progname, res_name, buf);
  return 0;
}

int 
get_integer_resource (char *res_name, char *res_class)
{
  int val;
  char c, *s = get_string_resource (res_name, res_class);
  char *ss = s;
  if (!s) return 0;

  while (*ss && *ss <= ' ') ss++;			/* skip whitespace */

  if (ss[0] == '0' && (ss[1] == 'x' || ss[1] == 'X'))	/* 0x: parse as hex */
    {
      if (1 == sscanf (ss+2, "%x %c", &val, &c))
	{
	  free (s);
	  return val;
	}
    }
  else							/* else parse as dec */
    {
      if (1 == sscanf (ss, "%d %c", &val, &c))
	{
	  free (s);
	  return val;
	}
    }

  fprintf (stderr, "%s: %s must be an integer, not %s.\n",
	   progname, res_name, s);
  free (s);
  return 0;
}

double
get_float_resource (char *res_name, char *res_class)
{
  double val;
  char c, *s = get_string_resource (res_name, res_class);
  if (! s) return 0.0;
  if (1 == sscanf (s, " %lf %c", &val, &c))
    {
      free (s);
      return val;
    }
  fprintf (stderr, "%s: %s must be a float, not %s.\n",
	   progname, res_name, s);
  free (s);
  return 0.0;
}


unsigned int
get_pixel_resource (char *res_name, char *res_class,
		    Display *dpy, Colormap cmap)
{
  XColor color;
  char *s = get_string_resource (res_name, res_class);
  char *s2;
  if (!s) goto DEFAULT;

  for (s2 = s + strlen(s) - 1; s2 > s; s2--)
    if (*s2 == ' ' || *s2 == '\t')
      *s2 = 0;
    else
      break;

  if (! XParseColor (dpy, cmap, s, &color))
    {
      fprintf (stderr, "%s: can't parse color %s\n", progname, s);
      goto DEFAULT;
    }
  if (! XAllocColor (dpy, cmap, &color))
    {
      fprintf (stderr, "%s: couldn't allocate color %s\n", progname, s);
      goto DEFAULT;
    }
  free (s);
  return color.pixel;
 DEFAULT:
  if (s) free (s);
  return ((strlen(res_class) >= 10 &&
	   !strcmp ("Background", res_class + strlen(res_class) - 10))
	  ? BlackPixel (dpy, DefaultScreen (dpy))
	  : WhitePixel (dpy, DefaultScreen (dpy)));
}


int
parse_time (const char *string, Bool seconds_default_p, Bool silent_p)
{
  unsigned int h, m, s;
  char c;
  if (3 == sscanf (string,   " %u : %2u : %2u %c", &h, &m, &s, &c))
    ;
  else if (2 == sscanf (string, " : %2u : %2u %c", &m, &s, &c) ||
	   2 == sscanf (string,    " %u : %2u %c", &m, &s, &c))
    h = 0;
  else if (1 == sscanf (string,       " : %2u %c", &s, &c))
    h = m = 0;
  else if (1 == sscanf (string,          " %u %c",
			(seconds_default_p ? &s : &m), &c))
    {
      h = 0;
      if (seconds_default_p) m = 0;
      else s = 0;
    }
  else
    {
      if (! silent_p)
	fprintf (stderr, "%s: invalid time interval specification \"%s\".\n",
		 progname, string);
      return -1;
    }
  if (s >= 60 && (h != 0 || m != 0))
    {
      if (! silent_p)
	fprintf (stderr, "%s: seconds > 59 in \"%s\".\n", progname, string);
      return -1;
    }
  if (m >= 60 && h > 0)
    {
      if (! silent_p)
	fprintf (stderr, "%s: minutes > 59 in \"%s\".\n", progname, string);
      return -1;
    }
  return ((h * 60 * 60) + (m * 60) + s);
}

static unsigned int 
get_time_resource (char *res_name, char *res_class, Bool sec_p)
{
  int val;
  char *s = get_string_resource (res_name, res_class);
  if (!s) return 0;
  val = parse_time (s, sec_p, False);
  free (s);
  return (val < 0 ? 0 : val);
}

unsigned int 
get_seconds_resource (char *res_name, char *res_class)
{
  return get_time_resource (res_name, res_class, True);
}

unsigned int 
get_minutes_resource (char *res_name, char *res_class)
{
  return get_time_resource (res_name, res_class, False);
}
