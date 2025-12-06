/* The following file is (with slight modifications for Rockbox) from dietlibc
* version 0.31 which is licensed under the GPL version 2: */

#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <gcc_extensions.h>

#define ABS_LONG_MIN LONG_MAX
long int strtol(const char *nptr, char **endptr, int base)
{
  int neg=0;
  unsigned long int v;
  const char*orig=nptr;

  while(UNLIKELY(isspace(*nptr))) nptr++;

  if (*nptr == '-' && isalnum(nptr[1])) { neg=-1; ++nptr; }
  v=strtoul(nptr,endptr,base);
  if (endptr && *endptr==nptr) *endptr=(char *)orig;
  if (UNLIKELY(v>=ABS_LONG_MIN)) {
    if (v==ABS_LONG_MIN && neg) {
      errno=0;
      return v;
    }
    errno=ERANGE;
    return (neg?LONG_MIN:LONG_MAX);
  }
  return (neg?-v:v);
}

