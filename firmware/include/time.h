/*
 * time.h
 * 
 * Struct declaration for dealing with time.
 */

#ifndef _TIME_H_
#define _TIME_H_

#ifdef WPSEDITOR
#include <sys/types.h>
#include <time.h>
#endif

struct tm
{
  int	tm_sec;
  int	tm_min;
  int	tm_hour;
  int	tm_mday;
  int	tm_mon;
  int	tm_year;
  int	tm_wday;
  int	tm_yday;
  int	tm_isdst;
};

#if !defined(_TIME_T_DEFINED) && !defined(_TIME_T_DECLARED)
typedef long time_t;

/* this define below is used by the mingw headers to prevent duplicate
   typedefs */
#define _TIME_T_DEFINED
#define _TIME_T_DECLARED
time_t time(time_t *t);
struct tm *localtime(const time_t *timep);

#endif /* SIMULATOR */

#ifdef __PCTOOL__
/* this time.h does not define struct timespec,
   so tell sys/stat.h not to use it */
#undef __USE_MISC  
#endif


#endif /* _TIME_H_ */


