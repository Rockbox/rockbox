#include "plugin.h"

char *strstr(const char *haystack, const char *needle) {
  size_t nl=rb->strlen(needle);
  size_t hl=rb->strlen(haystack);
  int i;
  if (!nl) goto found;
  if (nl>hl) return 0;
  for (i=hl-nl+1;LIKELY(i); --i) {
    if (*haystack==*needle && !rb->memcmp(haystack,needle,nl))
found:
      return (char*)haystack;
    ++haystack;
  }
  return 0;
}
