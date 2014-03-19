#include "rocklibc.h"

#undef strspn

size_t strspn(const char *s, const char *accept)
{
  size_t count;
  for (count = 0; *s != 0; s++, count++) {
    const char *ac;
    for (ac = accept; *ac != 0; ac++) {
      if (*ac == *s)
        break;
    }
    if (*ac == 0) /* no acceptable char found */
      break;
  }
  return count;
}

