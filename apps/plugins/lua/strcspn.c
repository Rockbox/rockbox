#include "rocklibc.h"

size_t strcspn(const char *s, const char *reject)
{
  size_t l=0;
  int a=1,i,al=strlen(reject);

  while((a)&&(*s))
  {
    for(i=0;(a)&&(i<al);i++)
      if (*s==reject[i]) a=0;
    if (a) l++;
    s++;
  }
  return l;
}
