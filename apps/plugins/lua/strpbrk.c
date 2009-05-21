#include "rocklibc.h"

char *strpbrk(const char *s, const char *accept) {
  register int i,l=strlen(accept);
  for (; *s; s++)
    for (i=0; i<l; i++)
      if (*s == accept[i])
	return (char*)s;
  return 0;
}
