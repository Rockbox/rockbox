/*
 * time.h
 * 
 * Struct declaration for dealing with time.
 */

#ifndef _TIME_H_
#define _TIME_H_

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

#if defined(SIMULATOR) && !defined(_TIME_T_DEFINED)
/* for non-win32 simulators */
typedef long time_t;

/* this define below is used by the mingw headers to prevent duplicate
   typedefs */
#define _TIME_T_DEFINED
time_t time(time_t *t);
struct tm *localtime(const time_t *timep);

#endif /* SIMULATOR */

#endif /* _TIME_H_ */

