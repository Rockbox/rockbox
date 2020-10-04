#include "rocklibc.h"

#undef strpbrk
/* custom strbbrk allowing you to define search len of s*/
const char *strpbrk_n(const char *s, int smax, const char *accept) {
  register int i;
  register const char *a = accept;
  for (i=0; __likely(s[i] && i < smax); i++, a=accept)
    while(__likely(a++[0]))
      if ((s[i] == a[0]))
        return &s[i];
  return NULL;
}

inline char *strpbrk(const char *s, const char *accept) {
  return (char*)strpbrk_n(s, INT_MAX, accept);
}
